#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define COLOR_RED    "\x1b[31m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_RESET  "\x1b[0m"

#define MAX(a, b) (a) > (b) ? (a) : (b)

#define MAX_ERRORS 10
static int current_errors = 0;
static char *error_list[MAX_ERRORS];
static char *current_test;

void enqueue_error(char *msg, char *file, int line) {
    char *error;

    asprintf(&error, "%s: In test %s:\n%s:%d: error: %s\n",
             file, current_test, file, line, msg);
    error_list[current_errors++] = error;
}

#include "test-definitions.h"


typedef struct test_t {
    char *test_name;
    int (*test_fn)(void *);
} test_t;

#define C(x) { #x, x },
test_t test_list[] = { TESTS { NULL, NULL }};
#undef C

typedef enum color_t { red, green, none } color_t;

int should_colorize(void) {
    if(!isatty(1))
        return 0;

    char *term = getenv("TERM");
    return term && strcmp(term, "dumb") != 0;
}

int set_color(color_t color) {
    if(should_colorize()) {
        switch(color) {
        case red:
            printf(COLOR_RED);
            break;
        case green:
            printf(COLOR_GREEN);
            break;
        case none:
            printf(COLOR_RESET);
            break;
        }
    }
}


int main(int argc, char *argv[]) {
    test_t *current = test_list;
    int success = 1;
    int result;

    /* suppress error messages */
    c_set_emit_on_error(0);

    printf("Testing.\n------------------------------------------------\n");

    while((current->test_fn) && (current_errors < MAX_ERRORS)) {
        current_test = current->test_name;
        printf("%s%*s: ", current->test_name,
               (int)MAX(1, 40 - strlen(current->test_name)), " ");

        fflush(stdout);
        result = current->test_fn(NULL);
        if(result) {
            set_color(green);
            printf("Ok");
        } else {
            set_color(red);
            printf("Error");
            success = 0;
        }

        set_color(none);
        printf("\n");

        current++;
    }

    printf("------------------------------------------------\n");

    if(current_errors == MAX_ERRORS) {
        set_color(red);
        printf("Maximum errors reached.  Aborting.\n");
        set_color(none);
    }

    if(current_errors) {
        printf("\nErrors:\n------------------------------------------------\n");
        for(int index=0; index < current_errors; index++) {
            printf("%s", error_list[index]);
        }
    }

    printf("\nResult: ");
    if(success) {
        set_color(green);
        printf("Ok");
    } else {
        set_color(red);
        printf("Error");
    }

    set_color(none);
    printf("\n");

    exit(!success);
}
