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
  (if (< at 13)
      (begin
       (display (string-append "Tac " (number->string at) "\n"))
       (display (string-append "=> " (number->string (tak at 0 (- 0 at))) "\n"))
       (runTakInt (+ 1 at)))
      0))

(runTak)
