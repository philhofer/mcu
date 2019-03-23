(module
  feather-m0
  (;export
   feather-m0
   default-usb
   default-i2c
   extint
   pin-labels
   red-led)

;; definitions for Adafruit Feather-M0
;; Arduino-compatible development board

(import
    scheme
    (chicken base)
    (chicken type)
    config-base
    sam
    samd21)

;; default-usb returns a usb bus
;; bound to the given class
(: default-usb (symbol -> (struct bus)))
(define (default-usb class)
  (usb-bus class usb0))

;; red-led is the red LED attached to PA17
(: red-led (struct gpio))
(define red-led (make-gpio 'red_led 'PA17))

;; pin-labels is an alist that maps
;; the Feather M0's pin labels to
;; the pins+port group on the MCU
(define pin-labels
  '((AREF . PA03)
    (A0 . PA02)
    (A1 . PB08)
    (A2 . PB09)
    (A3 . PA04)
    (A4 . PA05)
    (A5 . PB02)
    (D5 . PA15)
    (D6 . PA20)
    (D9 . PA07)
    (D10 . PA18)
    (D11 . PA16)
    (D12 . PA19)
    (D13 . PA17)))

;; default-i2c returns an i2c bus
;; bound to the pins labeled SCL/SDA
;; on the board, with the frequency
;; set to 'freq'
(: default-i2c (fixnum #!rest * -> (struct bus)))
(define (default-i2c freq . devices)
  (i2c-bus freq (sercom-num 3) devices))

(: extint (#!rest list -> (struct sam-periph)))
(define (extint . mapping)
  (eic mapping))

(: feather-m0 ((list-of *) -> undefined))
(define (feather-m0 . config)
  (emit-board (make-board samd21g18 config)))

)
