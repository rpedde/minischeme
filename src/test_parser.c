#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "lisp-types.h"
#include "primitives.h"

void test_string_parsing(void *scaffold) {
    lisp_value_t *result;

    result = lisp_parse_string("\"this is a string\"");

    assert(result->type == l_string);
    assert(strcmp(L_STR(result), "this is a string") == 0);
}

void test_int_parsing(void *scaffold) {
    lisp_value_t *result;

    result = lisp_parse_string("7");

    assert(result->type == l_int);
    assert(L_INT(result) == 7);
}

void test_float_parsing(void *scaffold) {
    lisp_value_t *result;

    result = lisp_parse_string("1.");

    assert(result->type == l_float);
    assert(L_FLOAT(result) == 1.0);

    result = lisp_parse_string(".1");

    assert(result->type == l_float);
    assert(L_FLOAT(result) == 0.1);
}

int main(int argc, char *argv[]) {
    test_string_parsing(NULL);
    test_int_parsing(NULL);
    test_float_parsing(NULL);

    exit(EXIT_SUCCESS);
}
