#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lisp-types.h"
#include "primitives.h"
#include "selfcheck.h"

int test_string_parsing(void *scaffold) {
    lisp_value_t *result;

    result = lisp_parse_string("\"this is a string\"");

    assert(result->type == l_str);
    assert(strcmp(L_STR(result), "this is a string") == 0);
    return 1;
}

int test_int_parsing(void *scaffold) {
    lisp_value_t *result;

    result = lisp_parse_string("7");

    assert(result->type == l_int);
    assert(L_INT(result) == 7);
    return 1;
}

int test_float_parsing(void *scaffold) {
    lisp_value_t *result;

    result = lisp_parse_string("1.");

    assert(result->type == l_float);
    assert(L_FLOAT(result) == 1.0);

    result = lisp_parse_string(".1");

    assert(result->type == l_float);
    assert(L_FLOAT(result) == 0.1);
    return 1;
}

