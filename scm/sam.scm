(module
  sam
  (;export
   sercom
   sercom-pinmuxing
   match-sercom
   sam-eic
   sam-periph
   sam-usb
   sam-pin-num
   samd-m0-mcu
   samd-m4f-mcu)

(import
  scheme
  chicken.base
  chicken.type
  chicken.syntax
  srfi-1
  srfi-69
  chicken.bitwise
  chicken.port
  config-base
  arm)

; record type for common sam peripheral attributes
(define-record sam-periph
	       name      ;; symbol like 'sercom or 'sam-usb
	       irq       ;; fixnum of list of fixnum: irqs
	       addr      ;; base address
	       apb-index ;;
	       gclks     ;; GCLK number (or list of numbers)
	       pinconf)  ;; list of pairs of (symbol . char) for pin muxing

;; match a list against a pinmuxing configuration,
;; which is a vector of lists of pairs
(: pinmux-match ((list-of symbol) vector --> (or list false)))
(define (pinmux-match? lst vec)
  (let loop ([pins lst]
	     [conf '()]
	     [n    0])
    (if (null? pins)
	(reverse conf)
	(and-let* ([item (assq (car pins) (vector-ref vec n))])
	  (loop (cdr pins) (cons item conf) (+ n 1))))))

;; sercom-pinmuxing returns a lambda that matches
;; a single input argument (a list of pins) to pin-match
;; or returns #f
(define (sercom-pinmuxing irq addr apb-index gclks pin-match)
  (lambda (pins)
    (let ([conf (pinmux-match? pins pin-match)])
      (if conf
	  (sercom irq addr apb-index gclks conf)
	  #f))))

;; match-sercom matches a list of pins to
;; a set of sercom pinmuxings
(define (match-sercom pins . muxings)
  (let loop ([lst muxings])
    (cond
     [(null? lst) #f]
     [((car lst) pins) => identity]
     [else (loop (cdr lst))])))

(: sercom (fixnum fixnum fixnum (or (list-of fixnum) fixnum) (list-of pair) --> (struct sam-periph)))
(define (sercom irq addr apb-index gclks pinconf)
  (make-sam-periph 'sercom irq addr apb-index gclks pinconf))

(: sam-eic (fixnum fixnum fixnum (or (list-of fixnum) fixnum) (list-of list) --> (struct sam-periph)))
(define (sam-eic irq addr apb-index gclk mapping)
  (make-sam-periph 'eic irq addr apb-index gclk mapping))

(: sam-usb (fixnum fixnum fixnum (or (list-of fixnum) fixnum) (list-of pair) --> (struct sam-periph)))
(define (sam-usb irq addr apb-index gclk pinconf)
  (make-sam-periph 'sam-usb irq addr apb-index gclk pinconf))

(define-record sam-config
	       eic-mapper     ; proc (pin -> fixnum)
	       max-irqs       ; fixnum, all interrups must be < max-irq
	       port-groups    ; fixnum, number of port groups
	       mandatory-irqs ; list of fixnum, interrupts always configured
	       class)         ; 'samd-small or 'samd-large

(define (alist->cdefines lst)
  (map (lambda (item)
	 (make-cdefine (car item) (cdr item)))
       lst))

; default #defines for samd small boards
(define samd-small-defines
  (alist->cdefines
       `(("PM_BASE"      . #x40000400)
	 ("SYSCTRL_BASE" . #x40000800)
	 ("GCLK_BASE"    . #x40000C00)
	 ("WDT_BASE"     . #x40001000)
	 ("RTC_BASE"     . #x40001400)
	 ("DSU_BASE"     . #x41002000)
	 ("PORT_BASE"    . #x41004400)
	 ("DMAC_BASE"    . #x41004800)
	 ("NVMCTRL_BASE" . #x41004000)
	 ("EVSYS_BASE"   . #x42000400)
	 ("USB_CALIB_TRANSN" . "((read32(0x806024UL)>>13)&0x1f)")
	 ("USB_CALIB_TRANSP" . "((read32(0x806024UL)>>18)&0x1f)")
	 ("USB_CALIB_TRIM" . "((read32(0x806024UL)>>23)&0x7)")
	 ("CPU_HZ"       . 48000000))))

(define samd-large-defines
  (alist->cdefines
    `())) ; TODO

; samd-m0-mcu constructs a SAMDx mcu
; that uses the 'small' driver model
; (armv6-m, cortex-m0+, regular interrupts for peripherals)
;
; includes SAMD(09|10|20|21)
;
(: samd-m0-mcu (fixnum fixnum fixnum (symbol -> fixnum) fixnum --> (struct mcu)))
(define (samd-m0-mcu flash-size mem-size max-irqs eic-map port-groups)
  (define config
    (make-sam-config eic-map
		     max-irqs
		     port-groups
		     '()
		     'samd-small))
  (make-mcu 
    'sam
    (make-cpu armv6-m '(little-endian vtor systick mtb))
    `(0 . ,flash-size)
    `(#x20000000 . ,mem-size)
    (sam-default-irq-fn config)
    (sam-default-walk-fn config)
    (sam-default-require-fn config)
    (sam-default-board-fn config)
    (alist->hash-table
      `((sam-config . ,config)))))

; samd-m4f-mcu constructs a SAMDx mcu
; that uses the 'large' driver model
; (armv7-m, cortex-m4+fp, vectored interrupts)
;
; includes SAM[ED]51
(: samd-m4f-mcu (fixnum fixnum fixnum (symbol -> fixnum) fixnum --> (struct mcu)))
(define (samd-m4f-mcu flash-size mem-size max-irqs eic-map port-groups)
  (define config
    (make-sam-config eic-map
		     max-irqs
		     port-groups
		     '()
		     'samd-large))
  (make-mcu
    'sam
    (make-cpu armv7-m '(little-endian vtor systick mtb fp swo))
    `(0 . ,flash-size)
    `(#x20000000 . ,mem-size)
    (sam-default-irq-fn config)
    (sam-default-walk-fn config)
    (sam-default-require-fn config)
    (alist->hash-table
      `((sam-config . ,config)))))

; validate-periph validates a peripheral
(: validate-periph ((struct sam-config) (struct sam-periph) -> undefined))
(define (validate-periph config periph)
  (unless (sam-periph? periph)
    (error "expected" periph "to be a sam peripheral"))
  (unless (< (sam-periph-irq periph) (sam-config-max-irqs config))
    (error #"sam periph irq #(sam-periph-irq periph) above max #(sam-config-max-irqs config)")))

(: mcu-sam-config ((struct mcu) --> (struct sam-config)))
(define (mcu-sam-config mcu)
  (mcu-prop mcu 'sam-config))

(: periph-files ((struct sam-periph) -> (or string false)))
(define (periph-files p)
  (case (sam-periph-name p)
    [(eic)        "eic.o"]
    [(sercom)     "sercom.o"]
    [(sam-usb)    "sam-usb.o"]
    [else          #f]))

(: sam-peripherals ((struct board) -> (list-of (struct sam-periph))))
(define (sam-peripherals brd)
  (filter sam-periph? (board-config brd)))

(: validate-board ((struct board) -> undefined))
(define (validate-board brd)
  (let ([config (mcu-sam-config (board-mcu brd))]
	[perphs (sam-peripherals brd)])
    (for-each (cut validate-periph config <>) perphs)))

(define (board-sam-class brd)
  (let ([mcu (board-mcu brd)])
    (sam-config-class (mcu-prop mcu 'sam-config))))

;; sam-apb-num yields the APB number
;; of a peripheral given the board and
;; the peripheral
(: sam-apb-num ((struct sam-periph) --> fixnum))
(define (sam-apb-num p)
  ; APBn bus bases are #x4n000000
  (- (arithmetic-shift (sam-periph-addr p) -24) #x40))

(: pinrole-num (char --> fixnum))
(define (pinrole-num ch)
  (let ([v (- (char->integer ch)
	      (char->integer #\A))])
    (assert (>= v 0))
    v))

;; produce a C expression that assigns
;; the given role to the given pin
(: pinmux-expr (char symbol --> string))
(define (pinmux-expr role pin)
  (let ([rawnum (bitwise-and (sam-pin-num pin) #x1f)])
    #"port_pmux_pin(#(sam-pin-port-group pin), #rawnum, #(pinrole-num role));"))

;; default GCLK assignments for peripherals
(: sam-periph-default-clock ((struct sam-periph) --> (or (list-of fixnum) fixnum)))
(define (sam-periph-default-clock p)
  (case (sam-periph-name p)
    [(sercom) '(0 2)]
    [(eic)     2]
    [(usb)     0]
    [else      0]))

(: assign-gclk (fixnum fixnum --> string))
(define (assign-gclk src dst)
  #"gclk_enable_clock(#src, #dst);")

; zip two lists into a single list of
; pairs with 'ks' as keys and 'vs' as values
;
; n.b. this is defined in list-utils, but
; we really don't need to pull in a whole
; package for such a simple function...
(: zip-alist (list list --> list))
(define (zip-alist ks vs)
  (let loop ([out '()]
	     [ks   ks]
	     [vs   vs])
    (if (null? ks)
	out
	(loop (cons (cons (car ks) (car vs)) out)
	      (cdr ks)
	      (cdr vs)))))

; given a peripheral and a list of clock sources,
; emit expressions that perform the requisite
; clock assignments
(: assign-gclk-exprs ((struct sam-periph) list --> list))
(define (assign-gclk-exprs periph lst)
  (let ([gclks (sam-periph-gclks periph)]
	[clks  (sam-periph-default-clock periph)])
    (cond
      [(and (number? gclks) (number? clks))
       (cons (assign-gclk clks gclks) lst)]
      [(and (list? gclks) (list? clks))
       (begin
	 (unless (eqv? (length gclks) (length clks))
	   (error "periph" periph "only consumes" (length gclks) "clocks"))
	 (foldl (lambda (lst cell)
		  (cons (assign-gclk (car cell) (cdr cell)) lst))
		lst
		(zip-alist clks gclks)))]
      [else
       (error "invalid gclk spec on" periph)])))

;; sam-periph-init-exprs
;; produces generic initialization code for
;; a SAM peripheral
(: sam-periph-init-exprs ((struct sam-periph) fixnum (list-of string) --> (list-of string)))
(define (sam-periph-init-exprs p maxpins lst)
  (define (assign-pins p lst)
    (foldl (lambda (lst item)
	     (cons
	      (pinmux-expr (if (eic? p) #\A (cdr item)) (car item))
	      lst))
	   lst
	   (take (sam-periph-pinconf p) maxpins)))
  (cons
    #"powermgr_apb_mask(#(sam-apb-num p), #(sam-periph-apb-index p), true);"
    (assign-gclk-exprs p (assign-pins p lst))))

(: sercom-cdecls ((struct sam-periph) list --> list))
(define (sercom-cdecls p lst)
  (list*
    (c-struct #"static const struct sercom_config sercom_#(sam-periph-addr p)"
	      #".base = #(sam-periph-addr p)"
	      #".irq = #(sam-periph-irq p)")
    (make-cinclude "kernel/sam/sercom.h")
    lst))

(: sercom-i2c-cexprs ((struct bus) list --> list))
(define (sercom-i2c-cexprs bus lst)
  (let* ([sc     (bus-peripheral bus)]
	 [sc-ref #"&sercom_#(sam-periph-addr sc)"]
	 [final  #"return i2c_dev_reset(&#(bus-name bus), #(i2c-bus-flag->string bus));"]
	 [initfn (make-cfunc
		  #"int #(bus-name bus)_init(void)"
		  (sam-periph-init-exprs sc 2 (list final)))])
    ; need IRQ handler, default peripheral initialization,
    ; and the sercom structure for the runtime code
    ; (which must come first, since it is a static declaration)
    (sercom-cdecls
      sc
      (list*
	(irq-cfunc (sam-periph-irq sc)
		   #"sercom_i2c_master_irq(&#(bus-name bus));")
	(make-cinclude "kernel/sam/sercom-i2c-master.h")
	(i2c-cdecl bus
		   sc-ref
		   "&sercom_i2c_bus_ops"
		   (cons initfn lst))))))

(: emit-sam-usb ((struct bus) list --> (list-of (or (struct cfunc) (struct cexpr)))))
(define (sam-usb-cexprs bus lst)
  (let* ([p       (bus-peripheral bus)]
	 [usb-ref #"&#(bus-name bus)"]
	 [final   #"usb_init(#usb-ref);"]
	 [initfn  (make-cfunc
		    #"void #(bus-name bus)_init(void)"
		    (sam-periph-init-exprs p 2 (list final)))])
    (list*
      (c-struct #"static struct work #(bus-name bus)_work"
		#".udata = #usb-ref"
		#".func = (void(*)(void*))sam_usb_irq_bh")
      (irq-cfunc (sam-periph-irq p)
		 #"irq_disable_num(#(sam-periph-irq p));"
		 #"schedule_work(&#(bus-name bus)_work);")
      (make-cinclude "idle.h")
      (make-cinclude "kernel/sam/usb.h")
      (usb-cdecl bus "sam_usb_driver" (cons initfn lst)))))

(define (sam-default-getirq-fn config)
  (define (with-irqs p lst)
    (let ([irqs (sam-periph-irq p)])
      (cond
	[(list? irqs)   (append-reverse irqs lst)]
	[(number? irqs) (cons irqs lst)]
	[else           lst])))
  (lambda (brd)
    (foldl
      (lambda (lst i)
	(cond
	  [(bus? i)           (with-irqs (bus-peripheral i) lst)]
	  [(sam-periph? i)    (with-irqs i lst)]
	  [else               lst]))
      (sam-config-mandatory-irqs config)
      (board-config brd))))

(define (sam-default-require-fn config)
  ; produce the list of mandatory
  ; object files for sam boards
  (define (mandatory lst)
    (list*
      "gclk.o"
      "port.o"
      "nvmctrl.o"
      "sercom.o" ; technically not always necessary, but it is very thin!
      (if (eqv? 'samd-small (sam-config-class config))
	  "sysctrl-small.o"
	  "sysctrl-large.o")
      lst))
  (define (bus-requires bs lst)
    (case (bus-type bs)
      [(i2c) (cons "sercom-i2c-master.o" lst)]
      [(usb) (cons "sam-usb.o" lst)]
      [else  lst]))
  (lambda (brd lst)
    (foldl
      (lambda (lst item)
	(cond
	  [(bus? item) (bus-requires item lst)]
	  [(eic? item) (cons "eic.o" lst)]
	  [else        lst]))
      (mandatory lst)
      (board-config brd))))

(: sam-class? (* symbol --> boolean))
(define (sam-class? p class)
  (and (sam-periph? p)
       (eqv? (sam-periph-name p) class)))

(define (eic? e) (sam-class? e 'eic))
(define (usb? p) (sam-class? p 'sam-usb))

(define (bus-is? b p?)
  (and (bus? b)
       (p? (bus-peripheral b))))

(define (sam-usb-bus? b) (bus-is? b usb?))

(: sam-default-walk-fn ((struct sam-config) -> ((struct board) -> list)))
(define (sam-default-walk-fn config)
  ;; while walking the board config,
  ;; track which pins are claimed and
  ;; error if we see a pin more than once
  (define claimed-pins (make-hash-table))
  (define (claim-pin! pin)
    (assert (symbol? pin))
    (assert (< (sam-pin-port-group pin)
	       (sam-config-port-groups config)))
    (if (hash-table-exists? claimed-pins pin)
	(error "pin" pin "claimed more than once")
	(hash-table-set! claimed-pins pin #t)))

  (: claim-pins! ((struct bus) fixnum -> undefined))
  (define (claim-pins! bus num)
    (let ([pins (map car (sam-periph-pinconf (bus-peripheral bus)))])
      (for-each claim-pin! (take pins num))))

  (: claim-and-then (fixnum ((struct bus) list -> list) (struct bus) list -> list))
  (define (claim-and-then n fn bs lst)
    (let ([sc (bus-peripheral bs)])
      (assert (sam-periph? sc))
      (validate-periph config sc)
      (claim-pins! bs n)
      (fn bs lst)))

  (: walk-bus ((struct bus) list -> list))
  (define (walk-bus item lst)
    (case (bus-type item)
      [(i2c) (claim-and-then 2 sercom-i2c-cexprs item lst)]
      [(usb) (claim-and-then 2 sam-usb-cexprs item lst)]
      [else  lst]))

  (define seen-eic #f)

  (: walk-eic ((struct sam-periph) list -> list))
  (define (walk-eic item lst)
    (when seen-eic
      (error "can't support multiple EIC instances"))
    (set! seen-eic #t)
    (define (test-line row)
      #"if(bits&(1U<<#(car row))){#(third row)_entry();}")
    (define (pin-init row)
      #"eic_configure(#(car row), #(second row));")
    (for-each claim-pin! (map car (sam-periph-pinconf item)))
    (let* ([table  (sam-eic-table (sam-config-eic-mapper config) (sam-periph-pinconf item))]
	   [pinits (foldl (lambda (lst p)
			    (cons (pin-init p) lst))
			  (list
			    ; end of initialization
			    "eic_enable();"
			    #"irq_enable_num(#(sam-periph-irq item));")
			  table)]
	   [inits  (sam-periph-init-exprs item (length (sam-periph-pinconf item)) pinits)])
      (list*
	; TODO: handle vectored EIC interrupts, where each
	; EIC line gets a different interrupt handler
	(make-cinclude "kernel/sam/eic.h")
	(make-cfunc "void extint_init(void)" inits) 
	(make-cfunc "void eic_vec(u32 bits)" (map test-line table))
	(irq-cfunc (sam-periph-irq item) #"eic_irq_entry();")
	(extirq-defines table lst))))

  (: walk-gpio ((struct gpio) -> list))
  (define (walk-gpio item lst)
    (let* ([pin (gpio-pin item)]
	   [num (sam-pin-num pin)])
      (claim-pin! pin)
      (gpio-cdecl item num lst)))

  (lambda (brd)
    (foldl
      (lambda (lst item)
	(cond
	  [(bus? item)  (walk-bus  item lst)]
	  [(eic? item)  (walk-eic  item lst)]
	  [(gpio? item) (walk-gpio item lst)]
	  [else         lst]))
      (list
	(make-cinclude "kernel/sam/gclk.h")
	(make-cinclude "kernel/sam/powermgr.h")
	(make-cinclude "kernel/sam/port.h"))
      (board-config brd))))

(define (sam-default-irq-fn config)
  (define claimed-irqs (make-hash-table))
  (define (claim-irq! irq)
    (assert (fixnum? irq))
    (assert (< irq (sam-config-max-irqs config)))
    (if (hash-table-exists? claimed-irqs irq)
	(error "irq" irq "claimed more than once")
	(hash-table-set! claimed-irqs irq #t)))

  (define (claim-irqs! p lst)
    (let ([irqs (sam-periph-irq p)])
      (cond
	[(list? irqs) (begin
			(for-each claim-irq! irqs)
			(append-reverse irqs lst))]
	[(number? irqs) (cons irqs lst)]
	[else           lst])))

  (lambda (brd)
    (foldl
      (lambda (lst item)
	(cond
	  [(bus? item)        (claim-irqs! (bus-peripheral item) lst)]
	  [(sam-periph? item) (claim-irqs! item lst)]
	  [else               lst]))
      '()
      (board-config brd))))

(define (sam-default-board-fn config)
  (define tail
    (case (sam-config-class config)
      [(samd-small) samd-small-defines]
      [(samd-large) samd-large-defines]
      [else         (error "bad board class" (sam-config-class config))]))
  (define (usb-bus-defines b lst)
    (let ([p (bus-peripheral b)])
      (list*
	(make-cdefine "USB_IRQ_NUM" (sam-periph-irq p))
	(make-cdefine "USB_BASE" (sam-periph-addr p))
	lst)))
  (lambda (brd lst)
    (let ([periphs (board-config brd)])
      (foldl
	(lambda (lst p)
	  (cond
	    [(eic? p)         (cons (make-cdefine "EIC_BASE" (sam-periph-addr p)) lst)]
	    [(sam-usb-bus? p) (usb-bus-defines p lst)]
	    [else             lst]))
	tail
	periphs))))
  

(: sam-pin-port-group (symbol -> fixnum))
(define (sam-pin-port-group sym)
  (define A (char->integer #\A))
  (let ([g (- (char->integer (string-ref (symbol->string sym) 1)) A)])
    (if (< g 0)
	(error "invalid SAM pin" sym)
	g)))

;; convert a sam pin number like 'PA08 to an integer
;;
;; our conversion rule is (32*group)+num
(: sam-pin-num (symbol --> fixnum))
(define (sam-pin-num sym)
  (let* ([str  (symbol->string sym)]
	 [c0   (string-ref str 0)]
	 [grp  (sam-pin-port-group sym)]
	 [tail (string->number (substring str 2) 10)])
    (unless (and (equal? c0 #\P) (< tail 32))
      (error "invalid SAM pin" sym))
    (+ (* 32 grp) tail)))

;; eic sense to number mapping
(: sense->number (symbol -> fixnum))
(define (sense->number s)
  (case s
    [(rise) 1]
    [(fall) 2]
    [(both) 3]
    [(high) 4]
    [(low)  5]
    [else    (error "bad EIC sense" s)]))

(: sam-eic-table ((symbol -> fixnum) (list-of (list symbol symbol string)) ->
				     (list-of (list fixnum fixnum string))))
(define (sam-eic-table pmap irqconf)
  (define (list->pinsetting x)
    (list (pmap (first x)) (sense->number (second x)) (third x)))
  (define (check-dup muxing)
    (let ([h (make-hash-table)])
      (for-each (lambda (p)
		  (if (hash-table-exists? h (car p))
		      (error "overlapping EIC interrupts on" (car p))
		      (hash-table-set! h (car p) #t)))
		muxing)))
  (let ([muxing (map list->pinsetting irqconf)])
    (check-dup muxing)
    muxing))

(define (self-test)
  (define (check-equal? a b)
    (unless (equal? a b)
      (error "not equal:" a b)))
  (check-equal? (sam-pin-port-group 'PA08) 0)
  (check-equal? (sam-pin-port-group 'PC31) 2)
  (check-equal? (sam-pin-num 'PA31) 31)
  (check-equal? (sam-pin-num 'PB31) 63)
  (define (eic-map pin)
    (bitwise-and #xf (sam-pin-num pin)))
  (check-equal? (sam-eic-table eic-map
			       '((PA01 rise "do_one_irq()")
				 (PB05 high "do_another()")))
		'((1 1 "do_one_irq()")
		  (5 4 "do_another()"))))

(self-test)
)
