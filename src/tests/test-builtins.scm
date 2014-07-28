(define test-equal1 (lambda () (assert (equal? 1 1))))

(define test-car1 (lambda () (assert (equal? 1 (car '(1 2 3))))))
(define test-car2 (lambda () (assert (equal? 1 (car '(1 . 2))))))
(define test-car3 (lambda () (assert (equal? '(a) (car '((a) b c d))))))

(define test-cdr1 (lambda () (assert (equal? '(2 3) (cdr '(1 2 3))))))
(define test-cdr2 (lambda () (assert (equal? 2 (cdr '(1 . 2))))))
(define test-cdr3 (lambda () (assert (equal? '(b c d) (cdr '((a) b c d))))))

(define test-cons1 (lambda () (assert (equal? '(1 2 3) (cons 1 '(2 3))))))
(define test-cons2 (lambda () (assert (equal? '(1 . 2) (cons 1 2)))))
(define test-cons3 (lambda () (assert (equal? '(() . 0) (cons '() 0)))))
(define test-cons4 (lambda () (assert (equal? '(0) (cons 0 '())))))
(define test-cons5 (lambda () (assert (equal? '(()) (cons '() '())))))
(define test-cons6
  (lambda ()
    (assert
     (equal? '((a) b c d)
             (cons '(a) '(b c d))))))
(define test-cons7
  (lambda ()
    (assert (equal? '("a" b c)
                    (cons "a" '(b c))))))
(define test-cons8
  (lambda ()
    (assert (equal? '((a b) . c)
                    (cons '(a b) c)))))

(define test-pair?1
  (lambda ()
    (assert (equal? #t (pair? '(a . b))))))
(define test-pair?2
  (lambda ()
    (assert (equal? #t (pair? '(a b c))))))
(define test-pair?3
  (lambda ()
    (assert (equal? #f (pair? '())))))
;; ; we don't have vectors yet
;; (define test-pair?4
;;   (lambda ()
;;     (assert (equal? #f (pair? '#(a b))))))

(define test-lambda-value-return
  (lambda ()
    (assert ((lambda () #t)))))
(define test-lambda-arg-return
  (lambda ()
    (assert ((lambda (x) x) #t))))
(define test-lambda-closure-overwritten
  (lambda ()
    (let ((x #f))
      (assert ((lambda (x) x) #t)))))
(define test-lambda-closure-used
  (lambda ()
    (let ((x 1))
      (assert (equal? 2 ((lambda () (+ x 1))))))))
(define test-lambda-null-body
  (lambda ()
    ; the following should error
    (warn (not (equal? () ((lambda () ())))))))
;; ; no varargs support yet
;; (define test-lambda-varargs
;;   (lambda ()
;;     (assert
;;      (equal? '(2 3 4)
;; 	     ((lambda (a . b) b) 1 2 3 4)))))
