(module
  seesaw
  (;export
   seesaw
   extint)

;; definitions for Adafruit "Seesaw" SAMD09 breakout

(import
    scheme
    (chicken base)
    (chicken type)
    config-base
    sam
    samd09)

(: extint (#!rest list -> (struct sam-periph)))
(define (extint . mapping)
  (sam-eic mapping))

(: seesaw ((list-of *) -> undefined))
(define (seesaw . config)
  (emit-board (make-board samd09 config)))

)
