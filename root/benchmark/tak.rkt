#lang racket
(define (tak x y z)
  (if (< y x)
      (tak
       (tak (- x 1) y z)
       (tak (- y 1) z x)
       (tak (- z 1) x y)
       )
      z))

(define (runTak)
  (runTakInt 0))

(define (runTakInt at)
  (if (< at 12)
      (begin
       (display (string-append "Tak " (number->string at) "\n"))
       (display (string-append "=> " (number->string (tak (* 2 at) at 0)) "\n"))
       (runTakInt (+ 1 at)))
      0))

(for ((i (in-range 100)))
     (time (runTak)))

