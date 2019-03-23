(module samd21
  (;export
   samd21g18
   sercom-num
   eic
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

(: sercom# (fixnum --> (struct sam-periph)))
(define (sercom-num n)
  (case n
    [(0) (sercom  9 #x42000800 2 '(14 13) #\C '(PA08 PA09 PA10 PA11))]
    [(1) (sercom 10 #x42000C00 3 '(15 13) #\C '(PA16 PA17 PA18 PA19))]
    [(2) (sercom 11 #x42001000 4 '(16 13) #\C '(PA12 PA13 PA14 PA15))]
    [(3) (sercom 12 #x42001400 5 '(17 13) #\C '(PA22 PA23 PA24 PA24))]
    [(4) (sercom 13 #x42001800 6 '(18 13) #\C '(PB12 PB13 PB14 PB15))]
    [(5) (sercom 14 #x42001C00 7 '(19 13) #\D '(PB02 PB03 PB00 PB01))]
    [else (error "no such sercom" n)]))

(: usb0 (struct sam-periph))
(define usb0
  (sam-usb 7 #x41005000 5 6 #\G '(PA24 PA25)))

(: eic ((list-of list) --> (struct sam-periph)))
(define (eic mapping)
  (sam-eic 4 #x40001800 6 5 #\A mapping))

)
