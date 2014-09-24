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
    return mpfr_get_d(L_FLOAT(v), MPFR_RNDN);
}

int test_special_forms_quote(void *scaffold) {
    lv_t *r, *er;
    lexec_t *exec = (lexec_t *)scaffold;
    int ex_result;

    r = c_parse_string(exec, "(quote 1)");
    er = c_sequential_eval(exec, r);

    assert(er->type == l_int);
    assert(int_value(er) == 1);

    r = c_parse_string(exec, "(quote (1 2 3))");
    er = c_sequential_eval(exec, r);

    assert(er->type == l_pair);
    assert(c_list_length(er) == 3);

    // expect arity exception
    lisp_execute(exec, c_parse_string(exec, "(quote 1 2 3)"));
    assert(exec->exc == le_arity);

    return 1;
}

int test_plus(void *scaffold) {
    lv_t *r;
    int ex_result;
    lexec_t *exec = (lexec_t *)scaffold;

    /* test base case */
    r = c_sequential_eval(exec, c_parse_string(exec, "(+)"));
    assert(r->type == l_int);
    assert(int_value(r) == 0);

    /* test numeric adds only */
    lisp_execute(exec, c_parse_string(exec, "(+ 1 (quote arf))"));
    assert(exec->exc == le_type);

    /* test simple int add */
    r = c_sequential_eval(exec, c_parse_string(exec, "(+ 1 2)"));

    assert(r->type == l_int);
    assert(int_value(r) == 3);

    /* test promotion */
    r = c_sequential_eval(exec, c_parse_string(exec, "(+ 1 0.2)"));

    assert(r->type == l_float);
    assert(float_value(r) == 1.2);

    /* test listwise adds */
    r = c_sequential_eval(exec, c_parse_string(exec, "(+ 1 2 3)"));

    assert(r->type == l_int);
    assert(int_value(r) == 6);

    return 1;
}

int test_equal(void *scaffold) {
    lv_t *r;
    lexec_t *exec = (lexec_t *)scaffold;
    char *ev;
    int idx;

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
        r = c_sequential_eval(exec, c_parse_string(exec, passing[idx]));
        assert(r->type == l_bool);
        assert(L_BOOL(r) == 1);
        idx++;
    }

    return 1;
}
