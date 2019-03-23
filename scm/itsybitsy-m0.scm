(module
  itsybitsy-m0
  (;export
   itsybitsy-m0
   default-usb
   default-i2c
   extint
   red-led)

;; definitions for Adafruit ItsyBitsy-M0

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

(: itsybitsy-m0 ((list-of *) -> undefined))
(define (itsybitsy-m0 . config)
  (emit-board (make-board samd21g18 config)))

)
