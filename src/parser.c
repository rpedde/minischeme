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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <regex.h>

#include "lisp-types.h"
#include "primitives.h"
#include "ports.h"
#include "parser.h"

/* for tokenization */
#define R_RATIONAL "^[-+]?[0-9]+\\/[0-9]+$"
#define R_FLOAT    "^[-+]?([0-9]*)?\\.([0-9]+)?([eE][-+]?[0-9]+)?$"
#define R_INTEGER  "^[-+]?[0-9]+$"

/* tokenization */
typedef enum token_type_t { T_QUOTE, T_QUASIQUOTE, T_UNQUOTESPLICING,
                            T_UNQUOTE, T_OPENPAREN, T_CLOSEPAREN,
                            T_DOT, T_INTEGER, T_RATIONAL, T_FLOAT,
                            T_BOOL, T_SYMBOL, T_STRING, T_CHAR, T_EOF
} token_type_t;

typedef struct token_t {
    token_type_t tok;
    char *s_value;
} token_t;

#define TOKENIZER_MAX_TOKEN 256

/* Forwards */
static lv_t *c_parse_sexpr(lexec_t *exec, lv_t *port, token_t *tok);
static lv_t *c_parse_list(lexec_t *exec, lv_t *port, token_t *tok);
static lv_t *c_parse_atom(lexec_t *exec, lv_t *port, token_t *tok);

/* special characters */
typedef struct special_char_t {
    char *name;
    int value;
} special_char_t;

special_char_t special_chars[] = {
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

        /* non-standard */
        { "newline", 10 },
        { NULL, 0 }
};


/**
 * turn a special char string into a character
 * value.
 *
 * Forms are:
 *  #\<special value>
 *  #\x<hex digit>
 *  #\<char value>
 *
 * probably others, like octal digits, but meh.
 */
lv_t *c_char_value(lexec_t *exec, char *value) {
    int val;
    special_char_t *current = special_chars;

    assert(exec && value);
    assert(value[0] == '#');
    assert(value[1] == '\\');

    if(strlen(&value[2]) == 1) {
        return lisp_create_char(value[2]);
    } else if(value[2] == 'x') {
        rt_assert(strlen(&value[3]) == 2, le_syntax, "invalid char specifier");
        if(sscanf(&value[3], "%02x", &val) != 1) {
            rt_assert(0, le_syntax, "malformed hex value");
        }
        return lisp_create_char(val);
    } else {
        while(current->name && (strcasecmp(current->name, &value[2])))
            current++;

        rt_assert(current->name, le_syntax, "unknown special character");
        return lisp_create_char(current->value);
    }


    /* can't happen */
    assert(0);
    return lisp_create_null();
}

token_t *c_new_token(token_type_t tok, char *s_value) {
    token_t *pnew;
    pnew = safe_malloc(sizeof(token_t));
    pnew->tok = tok;
    pnew->s_value = NULL;

    if(s_value)
        pnew->s_value = safe_strdup(s_value);

    return pnew;
}

regex_t *c_compile_regex(char *expr) {
    int ret;
    regex_t *result;

    result = safe_malloc(sizeof(regex_t));
    ret = regcomp(result, expr, REG_EXTENDED);
    assert(!ret);

    return result;
}

token_t *c_determine_token(char *val) {
    static regex_t *r_rational = NULL;
    static regex_t *r_float = NULL;
    static regex_t *r_int = NULL;
    int ret;

    if(!r_rational) r_rational = c_compile_regex(R_RATIONAL);
    if(!r_float) r_float = c_compile_regex(R_FLOAT);
    if(!r_int) r_int = c_compile_regex(R_INTEGER);

    if(!strcmp(val, ".")) {
        return c_new_token(T_DOT, NULL);
    } else if(!strncmp(val, "#\\", 2)) {
        return c_new_token(T_CHAR, val);
    } else if(!strcmp(val, "'")) {
        return c_new_token(T_QUOTE, NULL);
    } else if(!strcmp(val, ",")) {
        return c_new_token(T_UNQUOTE, NULL);
    } else if(!strcmp(val, "`")) {
        return c_new_token(T_QUASIQUOTE, NULL);
    } else if(!strcmp(val, "#t") || !strcmp(val, "#f")) {
        return c_new_token(T_BOOL, val);
    }

    /* now, check regex terms:
     *
     * rational: [\-\+]?[0-9]+\/[0-9]+
     * float: [\-\+]?([0-9]*)?\.([0-9]+)?([eE][\-\+]?[0-9]+)?
     * integer: [\-\+]?[0-9]+
     */

    ret = regexec(r_rational, val, 0, NULL, 0);
    if(!ret)
        return c_new_token(T_RATIONAL, val);

    ret = regexec(r_float, val, 0, NULL, 0);
    if(!ret)
        return c_new_token(T_FLOAT, val);

    ret = regexec(r_int, val, 0, NULL, 0);
    if(!ret)
        return c_new_token(T_INTEGER, val);

    /* and anything else is a symbol */
    return c_new_token(T_SYMBOL, val);
}

/**
 * given the a port, read characters until the next token,
 * and return the token.  This is super naive and inefficent,
 * but that's okay.  :)
 */
token_t *c_get_token(lexec_t *exec, lv_t *port) {
    int result;
    int in_quote = 0;
    char buffer[TOKENIZER_MAX_TOKEN];
    int pos = 0;

    memset(buffer, 0, TOKENIZER_MAX_TOKEN);

    while(1) {
        result = c_peek_char(exec, port);

        if(in_quote) {
            if(result == '\\') {
                /* get the next char and escape it */
                result = c_read_char(exec, port);
                result = c_read_char(exec, port);

                if (result == -1)
                    rt_assert(0, le_syntax, "unterminated quote");

                switch(result) {
                case 'n':
                    buffer[pos++] = '\n';
                    break;
                case '\\':
                    buffer[pos++] = '\\';
                    break;
                case 'r':
                    buffer[pos++] = '\r';
                    break;
                case 't':
                    buffer[pos++] = '\t';
                    break;
                case '"':
                    buffer[pos++] = '"';
                    break;
                default:
                    rt_assert(0, le_syntax, "bad character escape");
                    break;
                }
            } else if(result == '"') {
                c_read_char(exec, port); /* consume it */
                in_quote = 0;
                return c_new_token(T_STRING, buffer);
            } else if(result == -1) {
                rt_assert(0, le_syntax, "no closing quote");
            } else {
                c_read_char(exec, port); /* consume it */
                buffer[pos++] = result;
            }
        } else {
            switch(result) {
            case -1:
                if(pos)
                    return c_determine_token(buffer);
                return c_new_token(T_EOF, buffer);
                break;

            case ';':
                if(pos)
                    return c_determine_token(buffer);

                /* otherwise, run out the line */
                while(result != -1 && result != '\n' && result != '\r')
                    result = c_read_char(exec, port);

                if(result == -1)
                    return c_new_token(T_EOF, buffer);
                break;
            case '"':
                rt_assert(!pos, le_syntax, "unexpected quote");
                c_read_char(exec, port); /* consume the character */
                in_quote = 1;
                break;
            case '(':
                if(pos)
                    return c_determine_token(buffer);
                c_read_char(exec, port);
                return c_new_token(T_OPENPAREN, NULL);
                break;
            case ')':
                if(pos)
                    return c_determine_token(buffer);

                c_read_char(exec, port);
                return c_new_token(T_CLOSEPAREN, NULL);
                break;
            case '@':
                result = c_read_char(exec, port);
                if(pos == 1 && buffer[0] == ',')
                    return c_new_token(T_UNQUOTESPLICING, NULL);
                buffer[pos++] = result;
                break;

            case '`':
            case '\'':
                if(pos) {
                    return c_determine_token(buffer);
                } else {
                    result = c_read_char(exec, port);
                    return c_new_token(result == '`' ? T_QUASIQUOTE : T_QUOTE, NULL);

                }
                break;
            case ' ':
            case '\n':
            case '\r':
                if(pos)
                    return c_determine_token(buffer);
                c_read_char(exec, port);
                break;
            default:
                buffer[pos++] = c_read_char(exec, port);
                break;
            }
        }
    }
}

lv_t *p_read(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "expecting port");

    lv_t *port = L_CAR(v);
    rt_assert(port->type == l_port, le_type, "expecting port");

    return c_parse(exec, port);
}

lv_t *c_parse(lexec_t *exec, lv_t *port) {
    lv_t *res;
    token_t *tok;

    assert(exec);
    assert(port && port->type == l_port &&
           c_port_direction(exec, port) != PD_OUTPUT);

    tok = c_get_token(exec, port);
    if(tok->tok == T_EOF)
        return lisp_create_err(les_read);

    res = c_parse_sexpr(exec, port, tok);
    return res;
}

static lv_t *c_parse_sexpr(lexec_t *exec, lv_t *port, token_t *tok) {
    /* QUOTE sexpr
     * QUASIQUOTE sexpr
     * UNQUOTESPLICING sexpr
     * UNQUOTE sexpr
     * atom
     * list
     */
    lv_t *meta_symbol = NULL;
    lv_t *result;
    lv_t *ptr;
    token_t *t_next;

    switch(tok->tok) {
    case T_QUOTE:
    case T_QUASIQUOTE:
    case T_UNQUOTESPLICING:
    case T_UNQUOTE:
        if(tok->tok == T_QUOTE)
            meta_symbol=lisp_create_symbol("quote");
        else if(tok->tok == T_QUASIQUOTE)
            meta_symbol = lisp_create_symbol("quasiquote");
        else if(tok->tok == T_UNQUOTESPLICING)
            meta_symbol = lisp_create_symbol("unquotesplicing");
        else
            meta_symbol = lisp_create_symbol("unquote");

        t_next = c_get_token(exec, port);
        return lisp_create_pair(meta_symbol, lisp_create_pair(c_parse_sexpr(exec, port, t_next), NULL));

    case T_OPENPAREN:
        t_next = c_get_token(exec, port);
        return c_parse_list(exec, port, t_next);
        break;

    case T_EOF:
        rt_assert(0, le_syntax, "unexpected eof");

    default:
        return c_parse_atom(exec, port, tok);
    }

    assert(0);
}

static lv_t *c_parse_list(lexec_t *exec, lv_t *port, token_t *tok) {
    lv_t *res, *ptr, *pnew, *next_item;
    token_t *t_next;

    res = NULL;
    ptr = res;

    t_next = tok;
    while(1) {
        switch(t_next->tok) {
        case T_DOT:
            t_next = c_get_token(exec, port);
            rt_assert(ptr, le_syntax, "unexpected '.'");

            next_item = c_parse_sexpr(exec, port, t_next);
            if(next_item->type != l_null) {
                L_CDR(ptr) = next_item;
            }

            t_next = c_get_token(exec, port);
            rt_assert(t_next->tok == T_CLOSEPAREN, le_syntax, "expecting ')'");
            return res;

        case T_EOF:
            rt_assert(0, le_syntax, "missing close paren");
            break;

        case T_CLOSEPAREN:
            if(!res)
                return lisp_create_null();
            return res;

        default:
            next_item = c_parse_sexpr(exec, port, t_next);

            /* otherwise, it's not a () */
            pnew = lisp_create_pair(next_item, NULL);
            if(!ptr) {
                res = pnew;
                ptr = pnew;
            } else {
                L_CDR(ptr) = pnew;
                ptr = pnew;
            }
            break;
        }
        t_next = c_get_token(exec, port);
    }

}

static lv_t *c_parse_atom(lexec_t *exec, lv_t *port, token_t *tok) {
    lv_t *pnew;

    switch(tok->tok) {
    case T_INTEGER:
        return lisp_create_int_str(tok->s_value);
    case T_RATIONAL:
        return lisp_create_rational_str(tok->s_value);
    case T_FLOAT:
        return lisp_create_float_str(tok->s_value);
    case T_BOOL:
        if(!strcmp(tok->s_value, "#t"))
            return lisp_create_bool(1);
        if(!strcmp(tok->s_value, "#f"))
            return lisp_create_bool(0);
        rt_assert(0, le_syntax, "invalid bool value");
        break;
    case T_SYMBOL:
        return lisp_create_symbol(tok->s_value);
    case T_STRING:
        return lisp_create_string(tok->s_value);
    case T_CHAR:
        return c_char_value(exec, tok->s_value);
    default:
        assert(0); /* unexpected type */
    }
}

lv_t *p_parsetest(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);

    lv_t *port = c_open_string(exec, v, PD_INPUT);

    return c_parse(exec, port);
}

lv_t *p_toktest(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);

    lv_t *port = c_open_file(exec, v, PD_INPUT);

    fprintf(stderr, "\n");
    while(!c_port_eof(exec, port)) {
        c_get_token(exec, port);
    }
    fprintf(stderr, "\n");
    return lisp_create_null();
}


lv_t  *c_parse_port(lexec_t *exec, lv_t *port) {
    jmp_buf jb;
    lv_t *res, *current, *expr;
    int exc;

    assert(exec);
    assert(port);

    lisp_exec_push_ex(exec, &jb);

    res = NULL;

    /* right now, we return a lisp error object
     * on eof, and rt_assert for any kind of parse
     * error */
    if((exc = setjmp(jb)) == 0) {
        while(1) {
            expr = c_parse(exec, port);
            if(expr->type == l_err) { /* eof */
                if(!res)
                    return lisp_create_null();
                return res;
            }
            /* one more valid term */
            if(!res) {
                res = lisp_create_pair(expr, NULL);
                current = res;
            } else {
                L_CDR(current) = lisp_create_pair(expr, NULL);
                current = L_CDR(current);
            }
        }
        /* this gets automatically popped on an exception */
        lisp_pop_ex(exec);
    }

    /* we'll let gc take care of the incomplete list */
    if(exec->ehandler)
        exec->ehandler(exec);
    else
        default_ehandler(exec);

    return lisp_create_err(les_read);
}


/**
 * this is _not_ p_read.  this is a c courtesy wrapper
 * that assume reading from a repl.
 *
 * it returns a list of lv_t, suitable for sequential
 * eval.  if there is no exception context present, a
 * default exception context will be provided.
 *
 * on error, an error object will be returned (le_read).  if
 * the error is due to incomplete data (unterminated
 * string or list), then an error of subtype
 * les_incomplete will be returned.
 */
lv_t *c_parse_string(lexec_t *exec, char *str) {
    lv_t *l_port, *l_str;

    assert(exec);
    assert(str);

    l_str = lisp_create_string(str);
    l_port = c_open_string(exec, lisp_create_pair(l_str, NULL), PD_INPUT);

    return c_parse_port(exec, l_port);
}

/*
 * again, this is not "load".  it's wrapped to
 * be useful mostly just for bootstrapping the
 * environment
 */
lv_t *c_parse_file(lexec_t *exec, char *file) {
    lv_t *l_port, *l_file;
    lv_t *retval;

    assert(exec);
    assert(file);

    l_file = lisp_create_string(file);
    l_port = c_open_file(exec, lisp_create_pair(l_file, NULL), PD_INPUT);

    return c_parse_port(exec, l_port);
}
