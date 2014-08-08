%include {
#include <stdint.h>
#include <string.h>

#include "lisp-types.h"
#include "primitives.h"
#include "tokenizer.h"
#include "assert.h"

#define STAMP(v) \
        lisp_stamp_value((v.lisp_value), \
                         ls->pos->first_line + 1, \
                         ls->pos->first_column + 1, ls->file)
}


%token_type {lexer_value_t}
%default_type {lexer_value_t}
%extra_argument { lexer_shared_t *ls }
%syntax_error {
    ls->error = 1;
    memcpy(ls->error_pos, ls->pos, sizeof(yyltype_t));
}


program ::= listitems(A).              { ls->result = A.lisp_value; }

sexpr(A) ::= QUOTE sexpr(B).           { A.lisp_value = lisp_wrap_type("quote", B.lisp_value); }
sexpr(A) ::= QUASIQUOTE sexpr(B).      { A.lisp_value = lisp_wrap_type("quasiquote", B.lisp_value); }
sexpr(A) ::= UNQUOTESPLICING sexpr(B). { A.lisp_value = lisp_wrap_type("unquote-splicing", B.lisp_value); }
sexpr(A) ::= UNQUOTE sexpr(B).         { A.lisp_value = lisp_wrap_type("unquote", B.lisp_value); }

sexpr(A) ::= atom(B).            { A.lisp_value = B.lisp_value; }
sexpr(A) ::= list(B).            { A.lisp_value = B.lisp_value; }

list(A) ::= OPENPAREN listitems(B) CLOSEPAREN.{ A.lisp_value = B.lisp_value; }
list(A) ::= OPENPAREN CLOSEPAREN.             { A.lisp_value =
                                                lisp_create_null(); STAMP(A); }

listitems(A) ::= sexpr(B).               { A.lisp_value = lisp_create_pair(
                                           B.lisp_value, NULL); STAMP(A); }
listitems(A) ::= sexpr(B) listitems(C).  { A.lisp_value = lisp_create_pair(
                                           B.lisp_value, C.lisp_value);
                                           STAMP(A); }
listitems(A) ::= sexpr(B) DOT sexpr(C).  { A.lisp_value = lisp_create_pair(
                                           B.lisp_value, C.lisp_value);
                                           STAMP(A); }

atom(A) ::= INTEGER(B).  { A.lisp_value = lisp_create_int_str(B.s_value); STAMP(A); }
atom(A) ::= RATIONAL(B). { A.lisp_value = lisp_create_rational_str(B.s_value); STAMP(A); }
atom(A) ::= FLOAT(B).    { A.lisp_value = lisp_create_float_str(B.s_value); STAMP(A); }
atom(A) ::= BOOL(B).     { A.lisp_value = lisp_create_bool(B.i_value); STAMP(A); }
atom(A) ::= SYMBOL(B).   { A.lisp_value = lisp_create_symbol(B.s_value); STAMP(A); }
atom(A) ::= STRING(B).   { A.lisp_value = lisp_create_string(B.s_value); STAMP(A); }
atom(A) ::= CHAR(B).     { A.lisp_value = lisp_create_char(B.ch_value); STAMP(A); }
