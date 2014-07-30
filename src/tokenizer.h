/*
 * Copyright (C) 2014 Ron Pedde <ron@pedde.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_

#include <stdint.h>

typedef union lexer_value_t {
    int64_t i_value;
    double f_value;
    char *s_value;
    char ch_value;
    lv_t *lisp_value;
} lexer_value_t;

#ifndef YYSTYPE
# define YYSTYPE lexer_value_t
# define YYSTYPE_IS_TRIVIAL 1
#endif

YYSTYPE yylval;

/* predeclare the lex/yacc stuff */
typedef struct yyltype_t {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} yyltype_t;

typedef struct lexer_shared_t {
    lv_t *result;
    yyltype_t *pos;
    char *file;
    yyltype_t *error_pos;
    int error;
} lexer_shared_t;

#define YYLTYPE yyltype_t
typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern int yylex(YYSTYPE *yyval_param, YYLTYPE *yylloc_param, void *yyscanner);
extern YY_BUFFER_STATE yy_scan_string(const char *, void *yyscanner );
extern int yylex_init(void **scanner);
extern int yylex_destroy (void *scanner);

extern void *ParseAlloc(void *(*mallocProc)(size_t));
extern void Parse(void *, int, lexer_value_t, lexer_shared_t *);

#endif /* _TOKENIZER_H_ */
