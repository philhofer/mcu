(module samd21
  (;export
   samd21g18
   sercom-at
   pins->i2c
   extint
   usb0)

(import
  scheme
  (chicken base)
  (chicken bitwise)
  (chicken type)
  srfi-69
  config-base
  sam)

(define (eic-mapper p)
  (bitwise-and #xf (sam-pin-num p)))

(: samd21g18 (struct mcu))
(define samd21g18
  (samd-m0-mcu
    (* 256 1024) ; 256kB flash
    (* 32 1024)  ; 32kB RAM
    28           ; 28 interrupts
    eic-mapper
    2))          ; 2 port groups

;; pins->i2c takes a list of pins,
;; an i2c speed flag, and a list of devices,
;; and creates an I2C bus out of them by
;; matching against the sercom peripherals
(define (pins->i2c pins flag devices)
  (define (i2c-pin? p)
    (memq p '(PA08 PA09 PA12 PA13 PA16 PA17 PA22 PA23 PB12 PB13 PB16 PB17 PB30 PB31)))
  (begin
    (for-each (lambda (p) (or (i2c-pin? p) (error "not an I2C pin" p))) pins)
    (cond
     [(sercom-at pins) => (lambda (s) (i2c-bus flag s devices))]
     [else (error "no sercom at pins" pins)])))

(: sercom-at ((list-of symbol) --> (struct sam-periph)))
(define (sercom-at pins)
  (match-sercom
    pins
    (sercom-pinmuxing  9 #x42000800 2 '(#x14 #x13)
		       #(((PA08 . #\C) (PA04 . #\D))
			 ((PA09 . #\C) (PA05 . #\D))
			 ((PA10 . #\C) (PA06 . #\D))
			 ((PA11 . #\C) (PA07 . #\D))))
    (sercom-pinmuxing 10 #x42000C00 3 '(#x15 #x13)
		      #(((PA16 . #\C) (PA00 . #\D))
			((PA17 . #\C) (PA01 . #\D))
			((PA18 . #\C) (PA30 . #\D))
			((PA19 . #\C) (PA31 . #\D))))
    (sercom-pinmuxing 11 #x42001000 4 '(#x16 #x13)
		      #(((PA12 . #\C) (PA08 . #\D))
			((PA13 . #\C) (PA09 . #\D))
			((PA14 . #\C) (PA10 . #\D))
			((PA15 . #\C) (PA11 . #\D))))
    (sercom-pinmuxing 12 #x42001400 5 '(#x17 #x13)
		      #(((PA22 . #\C) (PA16 . #\D))
			((PA23 . #\C) (PA17 . #\D))
			((PA24 . #\C) (PA18 . #\D))
			((PA24 . #\C) (PA19 . #\D))))
    (sercom-pinmuxing 13 #x42001800 6 '(#x18 #x13)
		      #(((PB12 . #\C) (PB08 . #\D))
			((PB13 . #\C) (PB09 . #\D))
			((PB14 . #\C) (PB10 . #\D))
			((PB15 . #\C) (PB11 . #\D))))
    (sercom-pinmuxing 14 #x42001C00 7 '(#x19 #x13)
		      #(((PB16 . #\C) (PB02 . #\D) (PB30 . #\D) (PA22 . #\D))
			((PB17 . #\C) (PB03 . #\D) (PB31 . #\D) (PA23 . #\D))
			((PA20 . #\C) (PB00 . #\D) (PB22 . #\D) (PA24 . #\D))
			((PA21 . #\C) (PB01 . #\D) (PB23 . #\D) (PA25 . #\D))))))

(: usb0 (struct sam-periph))
(define usb0
  (sam-usb 7 #x41005000 5 6 '((PA24 . #\G)
			      (PA25 . #\G))))

(: extint (#!rest list --> (struct sam-periph)))
(define (extint . mapping)
  (sam-eic 4 #x40001800 6 5 mapping))

)
