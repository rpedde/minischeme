%{

/*
 * Simple lisp parser
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
#include <stdint.h>
#include <string.h>


#include "lisp-types.h"
#include "primitives.h"

#define YERROR_VERBOSE 1

extern int yylex(void);
extern void yyerror(char *format, ...);
extern void yyflush(void);

static char *lisp_parse_input;
static int lisp_parse_offset;
static lisp_value_t *result;


%}

%union {
    int64_t i_value;
    double f_value;
    char *s_value;
    lisp_value_t *lisp_value;
};

%token <i_value> INTEGER
%token <f_value> FLOAT
%token <i_value> BOOL
%token <s_value> SYMBOL
%token <s_value> STRING
%token OPENPAREN
%token CLOSEPAREN
%token DOT

%type <lisp_value> atom
%type <lisp_value> list
%type <lisp_value> listitems
%type <lisp_value> sexpr

%%

sexpr: atom                        { result = $1; }
| list                             { result = $1; }
;

list: OPENPAREN listitems CLOSEPAREN   { $$ = $2; }
| OPENPAREN CLOSEPAREN                 { $$ = lisp_create_pair(NULL, NULL); }
;

listitems: sexpr                       { $$ = lisp_create_pair($1, NULL); }
| sexpr listitems                      { $$ = lisp_create_pair($1, $2); }
| sexpr DOT sexpr                      { $$ = lisp_create_pair($1, $3); }
;

atom: INTEGER    { $$ = lisp_create_int($1); }
| FLOAT          { $$ = lisp_create_float($1); }
| BOOL           { $$ = lisp_create_bool($1); }
| SYMBOL         { $$ = lisp_create_symbol($1); }
| STRING         { $$ = lisp_create_string($1); }
;

%%

int parser_read_input(char *buffer, int *read, int max) {
    int bytes_to_read = max;
    if(bytes_to_read > strlen(lisp_parse_input + lisp_parse_offset))
        bytes_to_read = strlen(lisp_parse_input + lisp_parse_offset);

    memcpy(buffer, lisp_parse_input + lisp_parse_offset, bytes_to_read);
    lisp_parse_offset += bytes_to_read;
    *read = bytes_to_read;
    return 0;
}

lisp_value_t *lisp_parse_string(char *string) {
    lisp_parse_offset = 0;
    lisp_parse_input = string;

    yyflush();

    if(strlen(string) == 0) {
        return lisp_create_pair(NULL, NULL);
    }

    if(!yyparse()) {
        return result;
    }
    return NULL;
}
