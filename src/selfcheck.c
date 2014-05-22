#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX(a, b) (a) > (b) ? (a) : (b)

#include "test-definitions.h"

typedef struct test_t {
    char *test_name;
    int (*test_fn)(void *);
} test_t;
    
#define C(x) { #x, x },
test_t test_list[] = { TESTS { NULL, NULL }};
#undef C

int main(int argc, char *argv[]) {
    test_t *current = test_list;
    int success=1;
    int result;

    printf("Testing.\n--------------------------------------------------------------------\n");

    while(current->test_fn) {
        printf("%s%*s: ", current->test_name, MAX(1, 40 - strlen(current->test_name)), " ");
        result = current->test_fn(NULL);
        printf("%s\n", result ? "SUCCESS" : "FAIL");
        success |= result;
        current++;
    }

    printf("--------------------------------------------------------------------\n");
    printf("Result: %s\n", success ? "SUCCESS" : "FAIL");

    exit(!success);
}
