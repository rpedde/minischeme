;
; Create r5-ish environment
;

(define + p-+)
(define null? p-null?)
(define symbol? p-symbol?)
(define atom? p-atom?)
(define cons? p-cons?)
(define list? p-list?)
(define pair? p-pair?)
(define equal? p-equal?)
(define set-cdr! p-set-cdr!)
(define set-car! p-set-car!)
(define length p-length)
(define inspect p-inspect)
(define load p-load)
(define assert p-assert)
(define warn p-warn)
(define not p-not)
(define cons p-cons)
(define car p-car)
(define cdr p-cdr)
(define gensym p-gensym)
(define display p-display)
(define format p-format)

(define plus p-+)
