/*
 * Simple lisp interpreter
 *
 * Copyright (C) 2014 Ron Pedde (ron@pedde.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __LISP_TYPES_H__
#define __LISP_TYPES_H__

#include <gmp.h>
#include <mpfr.h>

#define LISP_TYPES \
    C(l_int) \
    C(l_rational) \
    C(l_float) \
    C(l_bool) \
    C(l_sym) \
    C(l_str) \
    C(l_pair) \
    C(l_hash) \
    C(l_null) \
    C(l_port) \
    C(l_char) \
    C(l_fn)

#define C(x) x,
typedef enum lisp_type_t { LISP_TYPES l_max } lisp_type_t;
#undef C

extern char *lisp_types_list[];

#define MPFR_ROUND_TYPE MPFR_RNDN  /* roundTiesToEven (IEEE-754) */

#define LISP_EXCEPTIONS \
    C(le_success) \
    C(le_arity) \
    C(le_type) \
    C(le_lookup) \
    C(le_internal) \
    C(le_syntax) \
    C(le_system) \
    C(le_raise) \
    C(le_warn) \
    C(le_div)

#define C(x) x,
typedef enum lisp_exception_t { LISP_EXCEPTIONS le_max } lisp_exception_t;
#undef C

extern char *lisp_exceptions_list[];

/* typedef enum lisp_exception_t { */
/*     le_arity = 1,      /\* wrong arity for function *\/ */
/*     le_type,           /\* wrong type for operation *\/ */
/*     le_lookup,         /\* could not bind -- not in environment *\/ */
/*     le_internal,       /\* internal error (malloc?) *\/ */
/*     le_syntax,         /\* parse error *\/ */
/*     le_raise,          /\* deliberate raise *\/ */
/*     le_warn            /\* deliberate raise *\/ */
/* } lisp_exception_t; */

typedef enum lisp_funtype_t {
    lf_native,
    lf_lambda,
    lf_macro
} lisp_funtype_t;

typedef struct lv_t lv_t;

typedef struct lstack_t {
    struct lstack_t *next;
    void *data;
} lstack_t;

typedef struct lexec_t {
    lv_t *env;              // current working environment
    lstack_t *env_stack;    // environment stack
    lstack_t *ex_stack;     // exception handler stack
    lstack_t *eval_stack;   // evaluation stack

    /* is this really necessary? */
    lisp_exception_t exc;   // current exception
    char *msg;
    void (*ehandler)(struct lexec_t *exec);
} lexec_t;

typedef lv_t *(*lisp_method_t)(lexec_t *, lv_t*);

typedef struct port_info_t port_info_t;  /* ports.c */

#define L_CHAR(what)    (what)->value.ch.value
#define L_INT(what)     (what)->value.i.value
#define L_RAT(what)     (what)->value.r.value
#define L_FLOAT(what)   (what)->value.f.value
#define L_BOOL(what)    (what)->value.b.value
#define L_SYM(what)     (what)->value.s.value
#define L_STR(what)     (what)->value.c.value
#define L_CDR(what)     (what)->value.p.cdr
#define L_CAR(what)     (what)->value.p.car
#define L_HASH(what)    (what)->value.h.value

#define L_FN(what)      (what)->value.l.fn
#define L_FN_FTYPE(what) (what)->value.l.ftype
#define L_FN_ARGS(what) (what)->value.l.formals
#define L_FN_BODY(what) (what)->value.l.body
#define L_FN_ENV(what)  (what)->value.l.env

#define L_PORT(what)    (what)->value.port.pi

#define L_CADR(what)    L_CAR(L_CDR(what))
#define L_CAAR(what)    L_CAR(L_CAR(what))
#define L_CDAR(what)    L_CDR(L_CAR(what))
#define L_CDDR(what)    L_CDR(L_CDR(what))
#define L_CADAR(what)   L_CAR(L_CDR(L_CAR(what)))
#define L_CADDDR(what)  L_CAR(L_CDR(L_CDR(L_CDR(what))))
#define L_CADDR(what)   L_CAR(L_CDR(L_CDR(what)))

typedef struct lisp_char_t {
    char value;
} lisp_char_t;

typedef struct lisp_int_t {
    mpz_t value;
} lisp_int_t;

typedef struct lisp_rational_t {
    mpq_t value;
} lisp_rational_t;

typedef struct lisp_float_t {
    mpfr_t value;
} lisp_float_t;

typedef struct lisp_bool_t {
    int value;
} lisp_bool_t;

typedef struct lisp_symbol_t {
    char *value;
} lisp_symbol_t;

typedef struct lisp_string_t {
    char *value;
} lisp_string_t;

typedef struct lisp_pair_t {
    lv_t *car;
    lv_t *cdr;
} lisp_pair_t;

typedef struct lisp_hash_t {
    void *value;
    lisp_type_t index_type;
} lisp_hash_t;

typedef struct lisp_null_t {
} lisp_null_t;

typedef struct lisp_fn_t {
    lisp_method_t fn;
    lisp_funtype_t ftype;
    lv_t *formals;
    lv_t *body;
    lv_t *env;
} lisp_fn_t;

typedef struct lisp_port_t {
    port_info_t *pi;
} lisp_port_t;

typedef struct lv_t {
    lisp_type_t type;
    int col;
    int row;
    lv_t *bound;
    char *file;
    union {
        lisp_char_t ch;
        lisp_int_t i;
        lisp_rational_t r;
        lisp_float_t f;
        lisp_bool_t b;
        lisp_symbol_t s;
        lisp_string_t c;
        lisp_pair_t p;
        lisp_hash_t h;
        lisp_fn_t l;
        lisp_port_t port;
    } value;
} lv_t;

/** given a native c type, box it into a lisp type struct */
extern lv_t *lisp_create_type(void *value, lisp_type_t type);

#endif /* __LISP_TYPES_H__ */
