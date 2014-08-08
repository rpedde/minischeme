#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>
#include <gmp.h>

#include "lisp-types.h"
#include "primitives.h"
#include "selfcheck.h"

int int_value(lv_t *v) {
    return mpz_get_ui(L_INT(v));
}

double float_value(lv_t *v) {
    return mpf_get_d(L_FLOAT(v));
}

int test_special_forms_quote(void *scaffold) {
    lv_t *r, *er;
    lexec_t *exec;

    int ex_result;

    exec = lisp_context_new(5);
    lisp_set_ehandler(exec, null_ehandler);

    r = lisp_parse_string("(quote 1)");
    er = c_sequential_eval(exec, r);

    assert(er->type == l_int);
    assert(int_value(er) == 1);

    r = lisp_parse_string("(quote (1 2 3))");
    er = c_sequential_eval(exec, r);

    assert(er->type == l_pair);
    assert(c_list_length(er) == 3);

    // expect arity exception
    lisp_execute(exec, lisp_parse_string("(quote 1 2 3)"));
    assert(exec->exc == le_arity);

    return 1;
}

int test_plus(void *scaffold) {
    lv_t *r;
    lexec_t *exec;
    int ex_result;

    exec = lisp_context_new(5);
    lisp_set_ehandler(exec, null_ehandler);

    /* test base case */
    r = c_sequential_eval(exec, lisp_parse_string("(+)"));
    assert(r->type == l_int);
    assert(int_value(r) == 0);

    /* test numeric adds only */
    lisp_execute(exec, lisp_parse_string("(+ 1 (quote arf))"));
    assert(exec->exc == le_type);

    /* test simple int add */
    r = c_sequential_eval(exec, lisp_parse_string("(+ 1 2)"));

    assert(r->type == l_int);
    assert(int_value(r) == 3);

    /* test promotion */
    r = c_sequential_eval(exec, lisp_parse_string("(+ 1 0.2)"));

    assert(r->type == l_float);
    assert(float_value(r) == 1.2);

    /* test listwise adds */
    r = c_sequential_eval(exec, lisp_parse_string("(+ 1 2 3)"));

    assert(r->type == l_int);
    assert(int_value(r) == 6);

    return 1;
}

int test_equal(void *scaffold) {
    lv_t *r;
    lexec_t *exec;
    char *ev;
    int idx;

    exec = lisp_context_new(5);

    char *passing[] = {
        "(equal? 1 1)",
        "(equal? (quote #t) (quote #t))",
        "(equal? equal? equal?)",
        "(equal? (quote #t) (equal? 1 1))",
        NULL
    };

    char *failing[] = {
        "(equal? 1 2)",
        "(equal? equal? null?)",
        "(equal? 1.0 1)",
        "(equal? (quote #t) (equal? 1 2))",
        NULL
    };

    idx = 0;
    while(passing[idx]) {
        r = c_sequential_eval(exec, lisp_parse_string(passing[idx]));
        assert(r->type == l_bool);
        assert(L_BOOL(r) == 1);
        idx++;
    }

    return 1;
}
