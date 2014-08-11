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
