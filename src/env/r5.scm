;
; Create r5-ish environment
;

(define #exec-success 0)
(define #exec-arity 1)
(define #exec-type 2)
(define #exec-lookup 3)
(define #exec-internal 4)
(define #exec-syntax 5)
(define #exec-system 6)
(define #exec-raise 7)
(define #exec-warn 8)
(define #exec-div 9)

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

;; caar...cddddr
(define caar (lambda (x) (car (car (x)))))
(define cadr (lambda (x) (car (cdr (x)))))
(define cdar (lambda (x) (cdr (car (x)))))
(define cddr (lambda (x) (cdr (cdr (x)))))

(define caaar (lambda (x) (car (car (car x)))))
(define caadr (lambda (x) (car (car (cdr x)))))
(define cadar (lambda (x) (car (cdr (car x)))))
(define caddr (lambda (x) (car (cdr (cdr x)))))
(define cdaar (lambda (x) (cdr (car (car x)))))
(define cdadr (lambda (x) (cdr (car (cdr x)))))
(define cddar (lambda (x) (cdr (cdr (car x)))))
(define cdddr (lambda (x) (cdr (cdr (cdr x)))))

(define caaaar (lambda (x) (car (car (car (car x))))))
(define caaadr (lambda (x) (car (car (car (cdr x))))))
(define caadar (lambda (x) (car (car (cdr (car x))))))
(define caaddr (lambda (x) (car (car (cdr (cdr x))))))
(define cadaar (lambda (x) (car (cdr (car (car x))))))
(define cadadr (lambda (x) (car (cdr (car (cdr x))))))
(define caddar (lambda (x) (car (cdr (cdr (car x))))))
(define cadddr (lambda (x) (car (cdr (cdr (cdr x))))))
(define cdaaar (lambda (x) (cdr (car (car (car x))))))
(define cdaadr (lambda (x) (cdr (car (car (cdr x))))))
(define cdadar (lambda (x) (cdr (car (cdr (car x))))))
(define cdaddr (lambda (x) (cdr (car (cdr (cdr x))))))
(define cddaar (lambda (x) (cdr (cdr (car (car x))))))
(define cddadr (lambda (x) (cdr (cdr (car (cdr x))))))
(define cdddar (lambda (x) (cdr (cdr (cdr (car x))))))
(define cddddr (lambda (x) (cdr (cdr (cdr (cdr x))))))


;; ports
(define input-port? p-input-port?)
(define output-port? p-output-port?)
(define open-input-file p-open-input-file)
(define open-output-file p-open-output-file)
(define close-input-port p-close-input-port)
(define close-output-port p-close-output-port)
(define read-char p-read-char)
(define peek-char p-peek-char)
(define toktest p-toktest)

;; char
(define char? p-char?)
(define char=? p-char=?)
(define char<? p-char<?)
(define char>? p-char>?)
(define char<=? p-char<=?)
(define char>=? p-char>=?)
(define char->integer p-char->integer)

;; math
(define integer? p-integer?)
(define rational? p-rational?)
(define float? p-float?)
(define real? p-float?)
(define exact? p-exact?)
(define inexact? p-inexact?)
(define > p->)
(define < p-<)
(define >= p->=)
(define <= p-<=)
(define = p-=)
(define + p-+)
(define - p--)
(define * p-*)
(define / p-/)
;; (define quotient p-quotient)
;; (define remainder p-remainder)
;; (define modulo p-modulo)
;; (define floor p-floor)
;; (define ceiling p-ceiling)
;; (define truncate p-truncate)
;; (define round p-round)
