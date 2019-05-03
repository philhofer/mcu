(module samd09
  (;export
   samd09
   pins->i2c
   extint
   i2c-clusters)

(import
  scheme
  (chicken base)
  (chicken type)
  srfi-69
  config-base
  sam)

(: eic-mapper (symbol -> fixnum))
(define (eic-mapper p)
  ; see SAMD09 datasheet, page 10
  (case p
    [(PA16)           0]
    [(PA15 PA17)      1]
    [(PA02 PA30)      2]
    [(PA03 PA11 PA31) 3]
    [(PA04 PA24)      4]
    [(PA05 PA25)      5]
    [(PA06 PA08 PA22) 6]
    [(PA07 PA23 PA27) 7]
    [else (error "pin" p "has no EIC line")]))

(: samd09 (struct mcu))
(define samd09
  (samd-m0-mcu
    (* 16 1024)  ; 16kB flash
    (*  4 1024)  ; 4kB ram
    eic-mapper
    20
    1))

(define (extint . mapping)
  (sam-eic 4 #x40001800 6 5 mapping)) 

;; pins->i2c accepts a list of pins as (SDA SCL)
;; and a speed configuration flag and returns the
;; corresponding bus, e.g.
;;
;;   (pins->i2c '(PA14 PA15) 'normal-speed)
;;
(: pins->i2c ((list-of symbol) symbol #!rest * -> (struct bus)))
(define (pins->i2c pins flag . devices)
  (begin
    ;; only some pins are capable of I2C
    (let* ([i2c-capable? (lambda (p) (memq p '(PA14 PA15 PA22 PA23)))]
	   [checkpin     (lambda (p) (or (i2c-capable? p) (error "pin not i2c-capable" p)))])
      (for-each checkpin pins))
    (i2c-bus flag
	     (or (sercom-at pins) (error "no sercom with pins" pins))
	     devices)))

(define i2c-clusters '((PA14 PA15)
		       (PA22 PA23)))

#;(: pins->spi ((list-of symbol) fixnum symbol #!rest * -> (struct bus)))
#;(define (pins->spi pins freq mode . devices)
  (spi-bus freq mode
	   (or (sercom-at pins) (error "no sercom with pins" pins))
	   devices))

(: sercom-at ((list-of symbol) --> (struct sam-periph)))
(define (sercom-at pins)
  (match-sercom
    pins
    (sercom-pinmuxing 9 #x42000800 2 '(14 13)
			#(((PA06 . #\C) (PA14 . #\C) (PA04 . #\D))
			  ((PA07 . #\C) (PA15 . #\C) (PA05 . #\D))
			  ((PA10 . #\C) (PA04 . #\C) (PA06 . #\D) (PA08 . #\D))
			  ((PA11 . #\C) (PA05 . #\C) (PA07 . #\D) (PA09 . #\D))))
    (sercom-pinmuxing 10 #x42000C00 3 '(15 13)
			#(((PA22 . #\C) (PA30 . #\C))
			  ((PA23 . #\C) (PA31 . #\C))
			  ((PA16 . #\C) (PA08 . #\C) (PA24 . #\C) (PA30 . #\D))
			  ((PA17 . #\C) (PA09 . #\C) (PA25 . #\C) (PA31 . #\D))))))

(assert (pins->i2c '(PA14 PA15) 'normal-speed))
(assert (pins->i2c '(PA22 PA23) 'full-speed))

#;(: sercom-num (fixnum -> (struct sam-periph)))
#;(define (sercom-num n)
  (case n
    [(0) (sercom  9 #x42000800 2 '(14 13) #\C '(PA14 PA15 PA04 PA05))]
    [(1) (sercom 10 #x42000C00 3 '(15 13) #\C '(PA22 PA23 PA16 PA17))]
    [else (error "no such sercom" n)]))

)
