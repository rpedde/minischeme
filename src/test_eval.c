#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lisp-types.h"
#include "primitives.h"
#include "selfcheck.h"


int test_special_forms_quote(void *scaffold) {
    lv_t *r, *er;
    lv_t *env;

    env = scheme_report_environment(NULL, NULL);

    r = lisp_parse_string("(quote 1)");
    er = lisp_eval(env, r);

    assert(er->type == l_int);
    assert(L_INT(er) == 1);

    r = lisp_parse_string("(quote (1 2 3))");
    er = lisp_eval(env, r);

    assert(er->type == l_pair);
    assert(c_list_length(er) == 3);
}
