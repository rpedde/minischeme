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

#include "lisp-types.h"
#include "parser.h"

#undef YY_INPUT
#define YY_INPUT(b, r, s)  parser_read_input(b, &r, s)
%}

%option yylineno
%option noyywrap

qstring \"[^\"\n]*[\"\n]
symbol [^\t\r\n \)\(]*

%%
[\t ]+
\r
;.*\n

\(                             { return(OPENPAREN); }
\)                             { return(CLOSEPAREN); }

\.                             { return(DOT); }

\.[0-9]+                       { yylval.f_value = atof(yytext); return(FLOAT); }
[0-9]+\.[0-9]*                 { yylval.f_value = atof(yytext); return(FLOAT); }

[0-9]+                         { yylval.i_value = atoi(yytext); return(INTEGER); }

{qstring}                      { yylval.s_value = (char*)&(strdup(yytext)[1]);
                                 yylval.s_value[strlen(yylval.s_value) - 1] = '\0';
                                 return(STRING); }

{symbol}                       { yylval.s_value = (char*)strdup(yytext);
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

#undef yywrap

int yywrap(void) {
    return(-1);
}

void yyflush(void) {
    YY_FLUSH_BUFFER;
}
