(module
  config-base
  (;export
   make-cfunc
   c-struct
   make-cinclude
   make-cdefine
   cfunc
   cstruct
   cpu
   cpu?
   cpu-arch
   cpu-options
   make-cpu
   arch
   make-arch
   board
   board?
   board-mcu
   board-config
   make-board
   gpio
   gpio?
   gpio-name
   gpio-pin
   make-gpio
   irq-name
   irq-cfunc
   extirq-defines
   bus?
   bus-type
   bus-peripheral
   bus-devices
   bus-prop
   bus-name
   i2c-bus
   i2c-bus-flag->string
   usb-bus
   i2c-cdecl
   usb-cdecl
   gpio-cdecl
   mcu
   mcu?
   mcu-cpu
   mcu-flash-range
   mcu-memory-range
   mcu-prop
   make-mcu
   emit-board
   dedup
   hex
   list*)

(import
  scheme
  (chicken base)
  (chicken type)
  (chicken syntax)
  (chicken read-syntax)
  (chicken port)
  (chicken io)
  srfi-1  ;; filter
  srfi-69 ;; hash tables
  srfi-13) ;; string processing

;; script for generating C source files
;; based on s-expression-based hardware
;; descriptions
;;
;; this file defines the top-level types
;; and functions, and just enough hardware
;; so that it knows what other files to
;; evaluate at runtime
;;

; we use cons* so often that using a macro
; here seems worthwhile
;
; in practice, using this over cons* surfaces
; more type warnings, since it tends to make
; some return values transparent to csc
(define-syntax list*
  (syntax-rules ()
    ((_ a b)
     (cons a b))
    ((_ a b c* ...)
     (cons a (list* b c* ...)))))

(define-record arch name write-boot write-ld)
(define-record cpu arch options)
(define-record mcu class-name cpu flash-range memory-range getirq-fn cobj-fn require-fn board-fn props)
(define-record bus type properties peripheral devices)
(define-record board mcu config)
(define-record gpio name pin)

(: i2c-bus (symbol * list --> (struct bus)))
(define (i2c-bus flag peripheral devices)
  (make-bus 'i2c (alist->hash-table `((flag . ,flag))) peripheral devices))

(: i2c-bus-flag->string ((struct bus) --> string))
(define (i2c-bus-flag->string bus)
  (let ([flag (bus-prop bus 'flag)])
    (cond
     [(assq flag '((normal-speed . "I2C_SPEED_NORMAL")
		   (full-speed . "I2C_SPEED_FULL")
		   (fast-mode . "I2C_SPEED_FAST")
		   (high-speed . "I2C_SPEED_HIGH"))) => cdr]
     [else (error "bad i2c flag" flag)])))

(: spi-bus (fixnum * * list --> (struct bus)))
(define (spi-bus freq mode peripheral devices)
  (make-bus 'spi
            (alist->hash-table `((frequency . ,freq) (mode . ,mode)))
            peripheral devices))

(: usb-bus (symbol * --> (struct bus)))
(define (usb-bus class peripheral)
  (make-bus 'usb
            (alist->hash-table `((class . ,class)))
            peripheral
	    '()))

(: bus-prop ((struct bus) symbol -> *))
(define (bus-prop b v)
  (hash-table-ref/default (bus-properties b) v #f))

(: mcu-prop ((struct mcu) * -> *))
(define (mcu-prop m v)
  (hash-table-ref/default (mcu-props m) v #f))

(: irq-name (fixnum --> string))
(define (irq-name num)
  (string-join `("irq" ,(number->string num) "entry") "_"))

(: hex (fixnum --> string))
(define (hex num)
  (string-append "0x" (number->string num 16)))

(: irq-cfunc (fixnum list #!rest string --> (struct cfunc)))
(define (irq-cfunc num . body)
  (make-cfunc (string-append "void " (irq-name num) "(void)") body))

(: extirq-defines ((list fixnum * string) list --> list))
(define (extirq-defines table lst)
  (define (one-defines lst row)
    (let ([name (third row)]
	  [line (first row)])
      (list*
	(make-cdefine #"#{name}_enable()" #"extirq_enable(#line)")
	(make-cdefine #"#{name}_enabled()" #"extirq_enabled(#line)")
	(make-cdefine #"#{name}_disable()" #"extirq_disable(#line)")
	(make-cdefine #"#{name}_triggered()" #"(extirq_triggered(#line))")
	(make-cdefine #"#{name}_clear_enable()" #"extirq_clear_enable(#line)")
	(make-cfunc #"void #{name}_entry(void)" '())
	lst)))
  (cons
    (make-cinclude "extirq.h")
    (foldl one-defines lst table)))

; discrete types for C structures, functions,
; and includes, respectively
;
; cstructs and cfuncs get a declaration in config.h and
; a definition in config.c
;
; cexprs get inserted directly into board_setup()
(define-record cstruct decl body)
(define-record cfunc decl body)
(define-record cinclude name)
(define-record cdefine name value)

(define-type c-obj (or (struct cstruct) (struct cfunc) (struct cinclude)))

(: c-struct (string #!rest string --> (struct cstruct)))
(define (c-struct decl . members)
  (make-cstruct decl members))

(define (string-has-prefix str pre)
  (let ([len (string-length pre)])
    (and (>= (string-length str) len)
	 (string=? (substring str 0 len) pre))))

(: c-write (c-obj port (or port false) -> undefined))
(define (c-write obj hdr body)
  (define (write-cstruct obj)
    (unless (string-has-prefix (cstruct-decl obj) "static")
      (write-line #"extern #(cstruct-decl obj);" hdr))
    (when body
      (write-line #"#(cstruct-decl obj) = {" body)
      (for-each
	(lambda (mem)
	  (write-line #"  #{mem}," body))
	(cstruct-body obj))
      (write-line "};" body)))
  (define (write-cfunc obj)
    (unless (string-has-prefix (cfunc-decl obj) "static")
      (write-line #"extern #(cfunc-decl obj);" hdr))
    (when (and body (> (length (cfunc-body obj)) 0))
	(begin
	  (write-line #"#(cfunc-decl obj) {" body)
	  (for-each (cut write-line <> body) (cfunc-body obj))
	  (write-line "}" body))))
  (define (write-cdefine obj)
    (define prepend "#define")
    (define value-string
      (let ([x (cdefine-value obj)])
	(if (number? x) (hex x) x)))
    (write-line #"#prepend #(cdefine-name obj) #value-string" hdr))
  (define (write-cinclude obj)
    (write-line (string-append "#include <" (cinclude-name obj) ">") hdr))
  (cond
    [(cfunc?   obj)  (write-cfunc obj)]
    [(cstruct? obj)  (write-cstruct obj)]
    [(cinclude? obj) (write-cinclude obj)]
    [(cdefine? obj)  (write-cdefine obj)]
    [else            (error "can't c-write" obj)]))

(define (split-list pred? lst)
  (let loop ([lst lst]
	     [yes '()]
	     [no  '()])
    (if (null? lst)
	(values yes no)
	(let ([head (car lst)]
	      [rest (cdr lst)])
	  (if (pred? head)
	      (loop rest (cons head yes) no)
	      (loop rest yes (cons head no)))))))

;; write-config writes config.h to 'hdr'
;; and config.c to 'body' given a list of
;; c-obj entries
(: write-config ((list-of c-obj) port port -> undefined))
(define (write-config objs hdr body)
  (for-each (cut write-line "/* autogenerated; do not edit! */" <>) (list hdr body))
  (for-each (cut write-line <> hdr)
	    (list
	     "#ifndef __CONFIG_H_"
	     "#define __CONFIG_H_"))
  (let*-values ([(incs objs) (split-list cinclude? objs)]
		[(defs objs) (split-list cdefine? objs)])
    ; write any #defines before including other files,
    ; in case those headers are interested in the config #defines
    (write-line "#include <board.h>" hdr)
    (for-each (cut c-write <> hdr #f) defs)
    (for-each (cut c-write <> hdr #f) incs)
    (write-line "#include <config.h>" body)
    (for-each (cut c-write <> hdr body) objs)
    (write-line "#endif" hdr)))

(: dedup (forall (e) ((list-of e) -> (list-of e))))
(define (dedup lst)
  (define ht (make-hash-table))
  (let loop ([rem lst]
	     [rev '()])
      (if (null? rem)
	  (reverse rev)
	  (if (hash-table-exists? ht (car rem))
	      (loop (cdr rem) rev)
	      (begin
		(hash-table-set! ht (car rem) #t)
		(loop (cdr rem) (cons (car rem) rev)))))))

; convert 'foo -> "foo.o"
(: sym-obj (symbol --> string))
(define (sym-obj sym)
  (string-append (symbol->string sym) ".o"))

(: bus-name ((struct bus) --> string))
(define (bus-name b)
  (or (bus-prop b 'name)
      (string-append "default_" (symbol->string (bus-type b)))))

; default i2c declaration constructor
(: i2c-cdecl ((struct bus) string string list --> list))
(define (i2c-cdecl bus driver-sym ops-sym lst)
  (list*
    (make-cinclude "i2c.h")
    (c-struct #"struct i2c_dev #(bus-name bus)"
	      #".driver = #driver-sym"
	      #".ops = #ops-sym")
    lst))

; default usb declaration constructor
(: usb-cdecl ((struct bus) string list --> list))
(define (usb-cdecl bus driver-sym lst)
  (case (bus-prop bus 'class)
    [(usb-cdc-acm) (list*
		     (make-cinclude "usb.h")
		     (make-cinclude "usb-cdc-acm.h")
		     (c-struct #"struct acm_data #(bus-name bus)_acm_data")
		     (c-struct #"struct usb_dev #(bus-name bus)"
			       #".drv = &#driver-sym"
			       ".class = &usb_cdc_acm"
			       #".classdata = &#(bus-name bus)_acm_data")
		     lst)]
    [else          (error "bad usb class (add it?)" (bus-prop bus 'class))]))

; default gpio declaration constructor
(: gpio-cdecl ((struct gpio) fixnum list --> list))
(define (gpio-cdecl item number lst)
  (list*
    (make-cinclude "gpio.h")
    (c-struct #"const struct gpio #(gpio-name item)"
	      #".number = #number")
    lst))

(: emit-config ((struct board) output-port output-port -> undefined))
(define (emit-config brd hdr body)
  (write-config
    ((mcu-cobj-fn (board-mcu brd)) brd)
    hdr
    body))

(: emit-boot ((struct board) output-port -> undefined))
(define (emit-boot brd port)
  (let* ([mcu    (board-mcu brd)]
	 [getirq (mcu-getirq-fn mcu)]
	 [bvec   (arch-write-boot (cpu-arch (mcu-cpu mcu)))])
    (bvec mcu (getirq brd) port)))

(: emit-boot-file ((struct board) -> undefined))
(define (emit-boot-file brd)
  (call-with-output-file "boot.s" (cut emit-boot brd <>)))

(: emit-config-files ((struct board) -> undefined))
(define (emit-config-files brd)
  (let ([inner (lambda (body) (call-with-output-file "config.h"
						     (cut emit-config brd <> body)))])
    (call-with-output-file "config.c" inner)))

; emit Makefile include file
; by detecting dependencies in
; the board configuration
(: emit-mk ((struct board) output-port -> undefined))
(define (emit-mk brd port)
  (define required-objects
    '("config.o" "boot.o" "idle.o" "arch.o" "start.o" "libc.o"))

  (define (usb-objects obj lst)
    (list* "usb.o" (sym-obj (bus-prop obj 'class)) lst))

  (define (bus-requires bs lst)
    (case (bus-type bs)
      [(i2c) (cons "i2c.o" lst)]
      [(usb) (usb-objects bs lst)]
      [else  lst]))

  (define (default-require obj lst)
    (cond
      [(bus?  obj) (bus-requires obj lst)]
      [else        lst]))

  (define (require-objects lst)
    (define sep " ")
    (write-line #"objects += #(string-join lst sep)" port))
  (write-line "# autogenerated by configure; do not edit!" port)
  (write-line #"CONFIG_ARCH:=#(arch-name (cpu-arch (mcu-cpu (board-mcu brd))))" port)
  (write-line #"CONFIG_MCU_CLASS:=#(mcu-class-name (board-mcu brd))" port)

  ; utility for matching foldl's argument order
  (define (%require fn)
    (lambda (lst item) (fn item lst)))

  ; walk everything using default-require, then mcu-require
  (define default-objs
    (foldl (%require default-require) required-objects (board-config brd)))
  (define all-objs
    ((mcu-require-fn (board-mcu brd)) brd default-objs))
  (require-objects (dedup all-objs)))

(: emit-ld ((struct board) output-port -> undefined))
(define (emit-ld brd port)
  (let* ([arch (cpu-arch (mcu-cpu (board-mcu brd)))]
	 [mkld (arch-write-ld arch)])
    (mkld brd port)))

(: emit-board-hdr ((struct board) output-port -> undefined))
(define (emit-board-hdr brd port)
  (let* ([mcu (board-mcu brd)]
	 [wlk (mcu-board-fn mcu)])
    (write-line "#ifndef __BOARD_H_" port)
    (write-line "#define __BOARD_H_" port)
    (for-each (cut c-write <> port #f) (wlk brd '()))
    (write-line "#endif" port)))

(: emit-board-file ((struct board) -> undefined))
(define (emit-board-file brd)
  (call-with-output-file "board.h" (cut emit-board-hdr brd <>)))

(: emit-mk-file ((struct board) -> undefined))
(define (emit-mk-file brd)
  (call-with-output-file "config.mk" (cut emit-mk brd <>)))

(: emit-ld-file ((struct board) -> undefined))
(define (emit-ld-file brd)
  (call-with-output-file "board.ld" (cut emit-ld brd <>)))

(: emit-board ((struct board) -> undefined))
(define (emit-board brd)
  (for-each (cut <> brd) (list
			   emit-config-files
			   emit-board-file
			   emit-boot-file
			   emit-mk-file
			   emit-ld-file)))

(define (self-test)
  (define (test-eq a b)
    (unless (equal? a b)
            (error a "not equal to" b)))
  (begin
    (test-eq #"foo#(+ 1 3)#'(foo bar baz)#(reverse '(foo bar baz))"
             "foo4(foo bar baz)(baz bar foo)")
    (test-eq (dedup '(a b c d a a))
	     '(a b c d))))


(self-test)

)
