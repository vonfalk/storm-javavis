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

(define (consume x)
  null)

(define (repeat x)
  (when (> x 0)
    (begin
     (my-reverse (create 1000))
     (repeat (- x 1)))))

;; takes 2.5s
(consume (repeat 100))
(time (consume (repeat 100)))

