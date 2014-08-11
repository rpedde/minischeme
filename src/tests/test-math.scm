;; + - * /

; test behavior of special cases -- 0 and 1 arg
(define test-math-fn-+-arity (lambda () (assert (equal? (+) 0))))
(define test-math-fn-+-arity2 (lambda () (assert (equal? (+ 1) 1))))
(define test-math-fn-*-arity (lambda () (assert (equal? (*) 1))))
(define test-math-fn-*-arity2 (lambda () (assert (equal? (* 2) 2))))



;; modulo and remainder remainder
(define test-math-mr-i-m-pnpd (lambda () (assert (equal? 1 (modulo 13 4)))))
(define test-math-mr-i-r-pnpd (lambda () (assert (equal? 1 (remainder 13 4)))))
(define test-math-mr-i-m-nnpd (lambda () (assert (equal? 3 (modulo -13 4)))))
(define test-math-mr-i-r-nnpd (lambda () (assert (equal? -1 (remainder -13 4)))))
(define test-math-mr-i-m-pnnd (lambda () (assert (equal? -3 (modulo 13 -4)))))
(define test-math-mr-i-r-pnnd (lambda () (assert (equal? 1 (remainder 13 -4)))))
(define test-math-mr-i-m-nnnd (lambda () (assert (equal? -1 (modulo -13 -4)))))
(define test-math-mr-i-r-nnnd (lambda () (assert (equal? -1 (remainder -13 -4)))))

;; trunc/round/ceiling/floor
(define test-math-floor-fn (lambda () (assert (equal? -5 (floor -4.3)))))
(define test-math-floor-fp (lambda () (assert (equal?  3 (floor 3.5)))))
(define test-math-ceiling-fn (lambda () (assert (equal? -4 (ceiling -4.3)))))
(define test-math-ceiling-fp (lambda () (assert (equal? 4 (ceiling 3.5)))))
(define test-math-truncate-fn (lambda () (assert (equal? -4 (truncate -4.3)))))
(define test-math-truncate-fp (lambda () (assert (equal? 3 (truncate 3.5)))))
(define test-math-round-fn (lambda () (assert (equal? -4 (round -4.3)))))
(define test-math-round-fp (lambda () (assert (equal? 4 (round 3.5)))))
(define test-math-round-r (lambda () (assert (equal? 4 (round 7/2)))))
(define test-math-round-i (lambda () (assert (equal? 7 (round 7)))))
