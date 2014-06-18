(define test-equal1 (lambda () (assert (equal? 1 1))))

(define test-car1 (lambda () (assert (equal? 1 (car '(1 2 3))))))
(define test-car2 (lambda () (assert (equal? 1 (car '(1 . 2))))))

(define test-cdr1 (lambda () (assert (equal? '(2 3) (cdr '(1 2 3))))))
(define test-cdr2 (lambda () (assert (equal? 2 (cdr '(1 . 2))))))

(define test-cons1 (lambda () (assert (equal? '(1 2 3) (cons '1 '(2 3))))))
(define test-cons2 (lambda () (assert (equal? '(1 . 2) (cons '1 '2)))))
