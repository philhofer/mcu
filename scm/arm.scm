(module
  arm
  (;export
   armv6-m
   armv7-m)

(import
  scheme
  (chicken base)
  (chicken type)
  (chicken io)
  (chicken port)
  srfi-69
  config-base)

; return the assembly line corresponding
; to the given vector table entry; 'claimed'
; indicates whether or not it is used
(: irq-line (fixnum boolean --> string))
(define (irq-line num claimed)
  (if claimed
      #".word #(irq-name num)+1"
      #".word empty_handler+1"))

(: write-boot ((struct mcu) (list-of fixnum) output-port symbol -> undefined))
(define (write-boot mcu irqlist port arch-name)
  (define ht (make-hash-table))
  (for-each (cut hash-table-set! ht <> #t) irqlist)
  (define (claimed? num)
    (hash-table-ref/default ht num #f))
  (define max-irq
    (case arch-name
      [(armv7-m) 256]
      [(armv6-m) 32]
      [else      (error "bad arch name" arch-name)]))
  (define stack-top
    (let ([stk (mcu-memory-range mcu)])
      (+ (car stk) (cdr stk))))
  ; TODO:
  ;  - don't assume we have systick on armv6-m
  (display #<#EOF
.arch #(symbol->string arch-name)
.thumb
.pushsection .vector_table
.word #(hex stack-top)
.word reset_entry+1
.word nmi_entry+1
.word hardfault_entry+1
.word fatal0+1
.word fatal0+1
.word fatal0+1
.word fatal0+1
.word fatal0+1
.word fatal0+1
.word svcall_entry+1
.word fatal0+1
.word fatal0+1
.word pendsv_entry+1
.word systick_entry+1

EOF
port)
  (let loop ([i 0])
    (if (>= i max-irq)
	#t
	(begin
	  (write-line (irq-line i (claimed? i)) port)
	  (loop (+ i 1)))))
  (display #<#EOF
.popsection
fatal0:
    b .

EOF
port))

(define (write-ld brd port)
  (define mem (mcu-memory-range (board-mcu brd)))
  (define flash (mcu-flash-range (board-mcu brd)))
  (unless (eqv? 0 (car flash))
    (error "arm cortex-M requires flash at address 0!"))
  (display #<#EOF
MEMORY {
	RAM(rwx): ORIGIN = #(hex (car mem)), LENGTH = #(hex (cdr mem))
	FLASH(rx): ORIGIN = 0x00000000, LENGTH = #(hex (cdr flash))
}

ENTRY(reset_entry)

SECTIONS {
	.text : AT(0)
	{
		*(.vector_table)
		. = ALIGN(4);
		_stext = .;
		*(.text)
		*(.text*)
		*(.rodata)
		*(.rodata*)
		. = ALIGN(4);
		_etext = .;
	} >FLASH

	_sidata = .;

	.data : AT(_sidata)
	{
		. = ALIGN(4);
		_sdata = .;
		*(.data)
		*(.data*)
		. = ALIGN(4);
		_edata = .;
	} >RAM

	. = ALIGN(4);
	.bss :
	{
		_sbss = .;
		*(.bss)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >RAM
}

EOF
port))

; armv{6,7}m share most of their code, since
; they have (mostly) compatible memory
; and vector-table layouts
(define-syntax define-arch
  (syntax-rules ()
    ((_ name)
     (define name
       (make-arch (quote name) (cut write-boot <> <> <> (quote name)) write-ld)))))

(: armv6-m (struct arch))
(define-arch armv6-m)
(: armv7-m (struct arch))
(define-arch armv7-m)

)
