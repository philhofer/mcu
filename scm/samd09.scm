(module samd09
  (;export
   samd09
   sercom#)

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

(: sercom# (fixnum -> (struct sam-periph)))
(define (sercom# n)
  (case n
    [(0) (sercom  9 #x42000800 2 '(14 13) #\C '(PA14 PA15 PA04 PA05))]
    [(1) (sercom 10 #x42000C00 3 '(15 13) #\C '(PA22 PA23 PA16 PA17))]
    [else (error "no such sercom" n)]))

)
