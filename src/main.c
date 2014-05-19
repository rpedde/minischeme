/*
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "lisp-types.h"

void usage(char *a0) {
    printf("Usage: %s [options]\n\n", a0);
    printf("Valid options\n");
    printf(" -h           this help page\n");

    printf("\n\n");
}



void repl(int level) {
    char prompt[30];
    char *cmd;
    int quit = 0;
    int line = 1;
    lisp_value_t *parsed_value;

    while(!quit) {
        snprintf(prompt, sizeof(prompt), "%d:%d> ", level, line);

        // r!
        cmd = readline(prompt);
        
        if(!cmd) {
            printf("\n");
            quit = 1; 
            break;
        }

        parsed_value = lisp_parse_string(cmd);

        // e!

        // p!

        // and l.  ;)
        add_history(cmd);
        free(cmd);
        line++;
    }
}

int main(int argc, char *argv[]) {
    int option;
    char *infile = NULL;

    while((option = getopt(argc, argv, "f:h")) != -1) {
        switch(option) {
        case 'h':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
            break;

        case 'f':
            infile = optarg;
            break;

        default:
            fprintf(stderr, "Unknown argument: '%c'\n", option);
            usage(argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    if(infile) {
        // load the file and execute it.
        repl(0);
    } else {
        repl(0);
    }

    exit(EXIT_SUCCESS);
}
