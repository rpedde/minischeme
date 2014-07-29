# equivalence predicates #

* [ ] eqv?
* [ ] eq?
* [ ] equal?

# numeric #

* [ ] integer?
* [ ] float?

* [ ] exact?
* [ ] inexact?

* [ ] zero? (library)
* [ ] positive? (library)
* [ ] neagative? (library)
* [ ] odd? (library)
* [ ] even? (library)

* [ ] max (library)
* [ ] min (library)

* [X] +
* [ ] *
* [ ] -
* [ ] /

* [ ] abs (library)
* [ ] quotient
* [ ] remainder
* [ ] modulo

* [ ] gcd (library)
* [ ] lcm (library)

* [ ] floor
* [ ] ceiling
* [ ] truncate
* [ ] round

* [ ] exp
* [ ] log
* [ ] sin
* [ ] cos
* [ ] tan
* [ ] asin
* [ ] acos
* [ ] atan
* [ ] sqrt
* [ ] expt

* [ ] number->string
* [ ] string->number

# boolean #

* [X] not
* [ ] boolean?

# pairs and lists #

* [X] pair?
* [X] cons
* [X] car
* [X] cdr
* [X] set-car!
* [X] set-cdr
* [ ] caar/cadr -- cdddar/cddddr (library)
* [X] null?
* [X] list? (library?)
* [ ] list (library)
* [ ] length (library?)
* [ ] append (library)
* [ ] reverse (library)
* [ ] tail-list (library)
* [ ] list-ref (library)
* [ ] memq (library)
* [ ] memv (library)
* [ ] member (library)
* [ ] assq (library)
* [ ] assv (library)
* [ ] assoc (library)

# symbols #

* [X] symbol?
* [ ] symbol->string
* [ ] string->symbol

# chars #

* [ ] char?
* [ ] char=?
* [ ] char<?
* [ ] char>?
* [ ] char<=?
* [ ] char>=?

* [ ] char-ci=? (library)
* [ ] char-ci<? (library)
* [ ] char-ci>? (library)
* [ ] char-ci<=? (library)
* [ ] char-ci>=? (library)

* [ ] char-alphabetic? (library)
* [ ] char-numeric? (library)
* [ ] char-whitespace? (library)
* [ ] char-upper-case? (library)
* [ ] char-lower-case? (library)

* [ ] char->integer
* [ ] integer->char

* [ ] char-upcase (library)
* [ ] char-downcase (library)

# strings #

* [X] string?

* [ ] make-string

* [ ] string (library)
* [ ] string-length
* [ ] string-ref
* [ ] string-set!

* [ ] string=? (library)
* [ ] string-ci=? (library)

* [ ] string<? (library)
* [ ] string>? (library)
* [ ] string<=? (library)
* [ ] string>=? (library)
* [ ] string-ci<? (library)
* [ ] string-ci>? (library)
* [ ] string-ci<=? (library)
* [ ] string-ci>=? (library)

* [ ] substring (library)
* [ ] string-append (library)

* [ ] string->list (library)
* [ ] list->string (library)

* [ ] string-copy (library)
* [ ] string-fill! (library)

# control #

* [ ] procedure?
* [ ] apply
* [ ] map (library)
* [ ] for-each (library)
* [ ] force (library)
* [ ] call-with-current-continuation
* [ ] values
* [ ] call-with-values
* [ ] dynamic-wind

# eval #

* [ ] eval
* [X] scheme-report-environment
* [X] null-environment
* [ ] interaction-environment

# ports #

* [ ] call-with-input-file (library)
* [ ] call-with-output-file (library)
* [ ] input-port?
* [ ] output-port?
* [ ] current-input-port
* [ ] with-input-from-file
* [ ] with-output-to-file
* [ ] open-input-file
* [ ] open-output-file
* [ ] close-input-port
* [ ] close-output-port

# input #

* [ ] read
* [ ] read-char
* [ ] peek-char
* [ ] eof-object?
* [ ] char-ready?

# output #

* [ ] write (machine-readable)
* [ ] display (human readable)
* [ ] newline
* [ ] write-char

# system interface #

* [X] load
* [ ] transcript-on
* [ ] transcript-off
