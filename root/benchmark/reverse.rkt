#lang racket
(define (create len)
  (if (> len 0)
      (cons len (create (- len 1)))
      null))

(define (my-append a b)
  (if (null? a)
      b
      (cons (first a) (my-append (rest a) b))))

(define (my-reverse a)
  (if (null? a)
      null
      (my-append (my-reverse (rest a)) (cons (first a) null))))

(for ((i (in-range 100)))
     (time (my-reverse (create 3000))))

