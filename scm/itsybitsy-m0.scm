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
    (chicken module)
    config-base
    sam
    samd21)

(reexport
 samd21)

(define pin-labels
  ;; note: FLASH-XXX pins are not broken out
  '((A5 . PB02)
    (FLASH-MISO . PB03)
    (A1 . PB08)
    (A2 . PB09)
    (MOSI . PB10)
    (SCK . PB11)
    (FLASH-MOSI . PB22)
    (FLASH-SCK . PB23)
    (A0 . PA02)
    (AREF . PA03)
    (A3 . PA04)
    (A4 . PA05)
    (D8 . PA06)
    (D9 . PA07)
    (D4 . PA08)
    (D3 . PA09)
    (D1 . PA10)
    (D0 . PA11)
    (MISO . PA12)
    (D2 . PA14)
    (D5 . PA15) ; note: 5V pin
    (D11 . PA16)
    (D13 . PA17) ; also red-led
    (D10 . PA18)
    (D12 . PA19)
    (D6 . PA20)
    (D7 . PA21)
    (SDA . PA22)
    (SCL . PA23)
    (D- . PA24)
    (D+ . PA25)
    (FLASH-CS . PA27)
    (SWCLK . PA30)
    (SWDIO . PA31)))

(define (label->pin label)
  (cond
   [(assq label pin-labels) => cdr]
   [else #f]))

(define (labels->pins lst)
  (map label->pin lst))

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
(: default-i2c (symbol #!rest * -> (struct bus)))
(define (default-i2c freq . devices)
  (pins->i2c (labels->pins '(SDA SCL)) freq devices))

#;(define (flash-spi)
  ;; TODO: Winbond 25Q16 flash here...
  (pins->spi (labels->pins '(FLASH-MOSI FLASH-MISO FLASH-SCK)) 16000000))

(: itsybitsy-m0 ((list-of *) -> undefined))
(define (itsybitsy-m0 . config)
  (emit-board (make-board samd21g18 config)))

)
