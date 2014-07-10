#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <setjmp.h>
#include <dirent.h>

#include "lisp-types.h"
#include "primitives.h"

#define COLOR_RED    "\x1b[31m"
#define COLOR_GREEN  "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
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

typedef enum color_t { red, green, yellow, none } color_t;
typedef enum result_t { SUCCESS, FAIL, SKIP, WARN } result_t;

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
        case yellow:
            printf(COLOR_YELLOW);
            break;
        case none:
            printf(COLOR_RESET);
            break;
        }
    }
}

void print_test_start(char *test_name) {
    printf("%s%*s: ", test_name,
           (int)MAX(1, 40 - strlen(test_name)), " ");
    fflush(stdout);
}

void print_test_result(result_t result) {
    switch(result) {
    case SUCCESS:
        set_color(green);
        printf("Ok");
        break;
    case FAIL:
        set_color(red);
        printf("FAIL");
        break;
    case SKIP:
        set_color(yellow);
        printf("SKIP");
        break;
    case WARN:
        set_color(yellow);
        printf("WARN");
        break;
    }

    set_color(none);
    printf("\n");
}

int run_scm_tests(char *testdir) {
    DIR *d;
    lexec_t *exec;
    lv_t *result;
    char buffer[4096];
    struct dirent *de;
    char *test;
    jmp_buf jb;
    int success = 1;
    int err;

    c_set_top_context(&jb);

    d = opendir(testdir);
    if(!d) {
        perror("opendir");
        return 0;
    }

    while((de = readdir(d))) {
        if((strlen(de->d_name) > 4) && (!strncasecmp(de->d_name, "test", 4))) {
            exec = lisp_context_new(5);

            snprintf(buffer, sizeof(buffer), "(load \"%s/%s\")",
                     testdir, de->d_name);

            c_sequential_eval(exec, lisp_parse_string(buffer));

            /* run through the environment, calling all the tests */
            void maybe_run_test(lv_t *l_n, lv_t *l_t) {
                if(!(l_n->type == l_sym || l_n->type == l_str)) {
                    exit(EXIT_FAILURE);
                }

                if(l_n->type == l_sym)
                    test = L_SYM(l_n);
                if(l_n->type == l_str)
                    test = L_STR(l_n);

                if((strlen(test) > 4) && (!strncasecmp(test, "test", 4))) {
                    print_test_start(test);
                    snprintf(buffer, sizeof(buffer), "(%s)", test);

                    if((err = setjmp(jb)) == 0) {
                        result = c_sequential_eval(exec, lisp_parse_string(buffer));
                        err = 0;
                    }

                    if(err == 0) {
                        print_test_result(SUCCESS);
                    } else if (err == le_warn) {
                        print_test_result(WARN);
                    } else {
                        print_test_result(FAIL);
                        success = 0;
                    }
                }
            }

            c_hash_walk(L_CAR(exec->env), maybe_run_test);
        }
    }

    closedir(d);
    return success;
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

        print_test_start(current->test_name);

        result = current->test_fn(NULL);

        if(result)
            print_test_result(SUCCESS);
        else {
            print_test_result(FAIL);
            success = 0;
        }

        current++;
    }

    if(!run_scm_tests("tests"))
        success = 0;

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
        print_test_result(SUCCESS);
    } else {
        print_test_result(FAIL);
    }

    exit(!success);
}
