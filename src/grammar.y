%include {
#include <stdint.h>

#include "lisp-types.h"
#include "primitives.h"
#include "tokenizer.h"
#include "assert.h"

}

%token_type {lexer_value_t}
%default_type {lexer_value_t}
%extra_argument { lv_t **result }


program ::= listitems(A).        { *result = A.lisp_value; }

sexpr(A) ::= atom(B).            { A.lisp_value = B.lisp_value; }
sexpr(A) ::= list(B).            { A.lisp_value = B.lisp_value; }

list(A) ::= OPENPAREN listitems(B) CLOSEPAREN.{ A.lisp_value = B.lisp_value; }
list(A) ::= OPENPAREN CLOSEPAREN.             { A.lisp_value =
                                                lisp_create_null(); }

listitems(A) ::= sexpr(B).               { A.lisp_value = lisp_create_pair(
                                           B.lisp_value, NULL); }
listitems(A) ::= sexpr(B) listitems(C).  { A.lisp_value = lisp_create_pair(
                                           B.lisp_value, C.lisp_value); }
listitems(A) ::= sexpr(B) DOT sexpr(C).  { A.lisp_value = lisp_create_pair(
                                           B.lisp_value, C.lisp_value); }

atom(A) ::= INTEGER(B). { A.lisp_value = lisp_create_int(B.i_value); }
atom(A) ::= FLOAT(B).   { A.lisp_value = lisp_create_float(B.f_value); }
atom(A) ::= BOOL(B).    { A.lisp_value = lisp_create_bool(B.i_value); }
atom(A) ::= SYMBOL(B).  { A.lisp_value = lisp_create_symbol(B.s_value); }
atom(A) ::= STRING(B).  { A.lisp_value = lisp_create_string(B.s_value); }
