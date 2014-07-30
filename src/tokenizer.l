%{

/*
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
#include <stdarg.h>
#include <assert.h>

#include "grammar.h"
#include "lisp-types.h"
#include "tokenizer.h"

/* handle locations */


#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno; \
    yylloc->first_column = yycolumn; yylloc->last_column = yycolumn+yyleng-1; \
    yycolumn += yyleng;

#define YY_USER_INIT yycolumn = 0;


extern char tok_hexint(char *str);
extern char tok_char_special(char *str);

%}

%option yylineno
%option noyywrap
%option reentrant
%option bison-locations

qstring \"[^\"\n]*[\"\n]
symbol [^\t\r\n \)\(,`']*
special_chars (nul|soh|stx|etx|eot|enq|ack|bel|bs|ht|lf|vt|ff|cr|so|si|dle|dc1|dc2|dc3|dc4|nak|syn|etb|can|em|sub|esc|fs|gs|rs|us|del|altmode|backnext|backspace|call|linefeed|page|return|rubout|space|tab)


%%
[\t ]+
\r
;.*\n
\n
\(                             { return(OPENPAREN); }
\)                             { return(CLOSEPAREN); }

#\\{special_chars}             { yylval->ch_value = tok_char_special(&yytext[2]); return(CHAR); }

#\\x[0-9A-Fa-f][0-9A-Fa-f]     { yylval->ch_value = tok_hexint(&yytext[3]); return(CHAR); }

#\\.                           { yylval->ch_value = yytext[2]; return(CHAR); }
\.                             { return(DOT); }
'                              { return(QUOTE); }
,@                             { return(UNQUOTESPLICING); }
,                              { return(UNQUOTE); }
`                              { return(QUASIQUOTE); }

\.[0-9]+                       { yylval->f_value = atof(yytext); return(FLOAT); }
[0-9]+\.[0-9]*                 { yylval->f_value = atof(yytext); return(FLOAT); }

[0-9]+                         { yylval->i_value = atoi(yytext); return(INTEGER); }

{qstring}                      { yylval->s_value = (char*)&(strdup(yytext)[1]);
                                 yylval->s_value[strlen(yylval->s_value) - 1] = '\0';
                                 return(STRING); }

\#t                            { yylval->i_value = 1; return(BOOL); }
\#f                            { yylval->i_value = 0; return(BOOL); }

{symbol}                       { yylval->s_value = (char*)strdup(yytext);
                                 return(SYMBOL); }

%%

void yyerror(char *format, ...) {
    va_list args;

    // fprintf(stderr, "%s:%d: error: ", parser_file, parser_line);

    fprintf(stdout, "Error: ");
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);

    fprintf(stdout, "\n");
}

char tok_hexint(char *str) {
    int val;
    int tokens;

    tokens = sscanf(str, "%02x", &val);
    if(tokens != 1)
        assert(0);

    return (char)val;
}

char tok_char_special(char *str) {
    typedef struct toklist_t {
        char *string;
        char result;
    } toklist_t;

    toklist_t *current;
    toklist_t toklist[] = {
        { "nul", 0 },
        { "soh", 1 },
        { "stx", 2 },
        { "etx", 3 },
        { "eot", 4 },
        { "enq", 5 },
        { "ack", 6 },
        { "bel", 7 },
        { "bs", 8 },
        { "ht", 9 },
        { "lf", 10 },
        { "vt", 11 },
        { "ff", 12 },
        { "cr", 13 },
        { "so", 14 },
        { "si", 15 },
        { "dle", 16 },
        { "dc1", 17 },
        { "dc2", 18 },
        { "dc3", 19 },
        { "dc4", 20 },
        { "nak", 21 },
        { "syn", 22 },
        { "etb", 23 },
        { "can", 24 },
        { "em", 25 },
        { "sub", 26 },
        { "esc", 27 },
        { "fs", 28 },
        { "gs", 29 },
        { "rs", 30 },
        { "us", 31 },
        { "del", 127 },
        { "altmode", 27 },
        { "backnext", 31 },
        { "backspace", 8 },
        { "call", 26 },
        { "linefeed", 10 },
        { "page", 12 },
        { "return", 13 },
        { "rubout", 127 },
        { "space", 32 },
        { "tab", 9 },
        { NULL, 0 }
    };

    current = &toklist[0];
    while(current->string && (strcasecmp(current->string, str)))
        current++;

    if(current->string)
        return current->result;

    assert(0);
}
