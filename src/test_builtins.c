#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

#include "lisp-types.h"
#include "primitives.h"
#include "selfcheck.h"

int test_special_forms_quote(void *scaffold) {
    lv_t *r, *er;
    lv_t *env;

    jmp_buf jb;
    int ex_result;

    env = scheme_report_environment(NULL, NULL);

    r = lisp_parse_string("(quote 1)");
    er = lisp_eval(env, r);

    assert(er->type == l_int);
    assert(L_INT(er) == 1);

    r = lisp_parse_string("(quote (1 2 3))");
    er = lisp_eval(env, r);

    assert(er->type == l_pair);
    assert(c_list_length(er) == 3);

    c_set_top_context(&jb);
    if((ex_result = setjmp(jb)) == 0) {
        r = lisp_eval(env, lisp_parse_string("(quote 1 2 3)"));
        /* we expect a runtime assert here, so this shouldn't run */
        assert(0);
    } else {
        assert(ex_result == (int)le_arity);
    }
    c_set_top_context(NULL);

    return 1;
}

int test_plus(void *scaffold) {
    lv_t *r;
    lv_t *env = scheme_report_environment(NULL, NULL);
    jmp_buf jb;
    int ex_result;

    /* test base case */
    r = lisp_eval(env, lisp_parse_string("(+)"));
    assert(r->type == l_int);
    assert(L_INT(r) == 0);

    /* test numeric adds only */
    c_set_top_context(&jb);
    if((ex_result = setjmp(jb)) == 0) {
        r = lisp_eval(env, lisp_parse_string("(+ 1 (quote arf))"));
        /* we expect a runtime assert here, so this shouldn't run */
        assert(0);
    } else {
        assert(ex_result == (int)le_type);
    }
    c_set_top_context(NULL);

    /* test simple int add */
    r = lisp_eval(env, lisp_parse_string("(+ 1 2)"));

    assert(r->type == l_int);
    assert(L_INT(r) == 3);

    /* test promotion */
    r = lisp_eval(env, lisp_parse_string("(+ 1 0.2)"));

    assert(r->type == l_float);
    assert(L_FLOAT(r) == 1.2);

    /* test listwise adds */
    r = lisp_eval(env, lisp_parse_string("(+ 1 2 3)"));

    assert(r->type == l_int);
    assert(L_INT(r) == 6);

    return 1;
}
