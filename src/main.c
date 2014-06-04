/*
 * Simple lisp interpreter
 *
 * Copyright (C) 2014 Ron Pedde (ron@pedde.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <setjmp.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "lisp-types.h"
#include "primitives.h"

static int is_nil(lv_t *v) {
    return(v && v->type == l_null);
}

static jmp_buf jb;

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
    lv_t *parsed_value;
    lv_t *result;
    lv_t *env = scheme_report_environment(NULL, NULL);

    c_set_top_context(&jb);

    while(!quit) {
        snprintf(prompt, sizeof(prompt), "%d:%d> ", level, line);

        // r!
        cmd = readline(prompt);

        if(!cmd) {
            printf("\n");
            quit = 1;
            break;
        }

        if(!*cmd)
            continue;

        if(setjmp(jb) == 0) {
            parsed_value = lisp_parse_string(cmd);
        } else {
            parsed_value = NULL;
        }

        if(!parsed_value) {
            continue;
        }

        // e!
        result = c_sequential_eval(env, parsed_value);

        // p!
        if(result && !is_nil(result)) {
            lisp_dump_value(1, result, 0);
            printf("\n");
        }

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
