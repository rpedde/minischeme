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
#include "ports.h"
#include "parser.h"

static int is_nil(lv_t *v) {
    return(v && v->type == l_null);
}

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
    lv_t *env_sym;
    lv_t *result;
    lv_t *arg;
    lv_t *str;
    char sym_buf[20];
    lexec_t *exec;

    exec = lisp_context_new(5); /* get r5rs environment */

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

        parsed_value = c_parse_string(exec, cmd);

        /* null input */
        if(parsed_value->type == l_null)
            continue;

        /* a non-continuable error */
        if(parsed_value->type == l_err)
            continue;

        /* parsed_value = lisp_parse_string(cmd); */
        /* if(!parsed_value) { */
        /*     fprintf(stderr, "synax error\n"); */
        /*     continue; */
        /* } */

        // e!
        result = lisp_execute(exec, parsed_value);

        // p!
        if(result && !is_nil(result)) {
            sprintf(sym_buf, "$%d", line);
            env_sym = lisp_create_symbol(sym_buf);
            c_hash_insert(L_CAR(exec->env), env_sym, result);

            dprintf(1, "%s = ", sym_buf);

            str = lisp_str_from_value(exec, result, 0);
            printf("%s\n", L_STR(str));
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
