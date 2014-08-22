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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <regex.h>

#include "lisp-types.h"
#include "primitives.h"
#include "ports.h"

typedef enum port_type_t { PT_FILE, PT_STRING } port_type_t;
typedef enum port_dir_t { PD_INPUT, PD_OUTPUT, PD_BOTH } port_dir_t;

typedef struct port_file_info_t {
    int fd;
    lv_t *filename;
} port_file_info_t;

typedef struct port_string_info_t {
    char *buffer;
    size_t pos;
} port_string_info_t;

typedef struct port_info_t {
    port_type_t type;
    port_dir_t dir;
    union {
        port_file_info_t fi;
        port_string_info_t si;
    } info;
    int eof;
    int peek;
    int peeked_char;
} port_info_t;

/* for tokenization */
#define R_RATIONAL "^[-+]?[0-9]+\\/[0-9]+$"
#define R_FLOAT    "^[-+]?([0-9]*)?\\.([0-9]+)?([eE][-+]?[0-9]+)?$"
#define R_INTEGER  "^[-+]?[0-9]+$"

/* Forwards */
ssize_t c_read_fd(lexec_t *exec, int fd, char *buffer, size_t len);
ssize_t c_write_fd(lexec_t *exec, int fd, char *buffer, size_t len);
ssize_t c_read(lexec_t *exec, lv_t *port, char *buffer, size_t len);
ssize_t c_write(lexec_t *exec, lv_t *port, char *buffer, size_t len);
int c_read_char(lexec_t *exec, lv_t *port);
int c_peek_char(lexec_t *exec, lv_t *port);

/**
 * open a file system file for either read or write (or both).
 *
 * there are unspecified cases around here regarding
 * O_CREAT, O_APPEND, and O_TRUNC.  Not sure how this
 * should be handled.
 */
static lv_t *c_open_file(lexec_t *exec, lv_t *v, port_dir_t dir) {
    assert(exec);
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *filename = L_CAR(v);
    rt_assert(filename->type == l_str, le_type, "filename requires string");

    port_info_t *pi = (port_info_t *)safe_malloc(sizeof(port_info_t));
    memset(pi, 0, sizeof(port_info_t));

    pi->type = PT_FILE;
    pi->dir = dir;

    pi->info.fi.filename = filename;

    int f_mode = O_RDONLY;

    switch(dir) {
    case PD_INPUT:
        f_mode = O_RDONLY;
        break;
    case PD_OUTPUT:
        f_mode = O_WRONLY;
        break;
    case PD_BOTH:
        f_mode = O_RDWR;
        break;
    default:
        assert(0); /* can't happen */
    }

    pi->info.fi.fd = open(L_STR(filename), f_mode);
    rt_assert(pi->info.fi.fd != -1, le_system, strerror(errno));

    return lisp_create_port(pi);
}

/**
 * c helper for p_close_input_port and p_close_output_port
 */
static lv_t *c_close_file(lexec_t *exec, lv_t *port) {
    assert(exec);
    assert(port && port->type == l_port);
    assert(L_PORT(port)->type == PT_FILE);

    close(L_PORT(port)->info.fi.fd);
    return lisp_create_null();
}

lv_t *p_open_input_file(lexec_t *exec, lv_t *v) {
    return c_open_file(exec, v, PD_INPUT);
}

lv_t *p_open_output_file(lexec_t *exec, lv_t *v) {
    return c_open_file(exec, v, PD_OUTPUT);
}

lv_t *p_close_input_port(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");
    lv_t *port = L_CAR(v);
    rt_assert(L_PORT(port)->dir == PD_INPUT,
              le_type, "not an input port");

    switch(L_PORT(port)->type) {
    case PT_FILE:
        return c_close_file(exec, port);
        break;
    default:
        break;
    }

    /* an unhandled type */
    assert(0);
}

/**
 * read handler for bare unbuffered fds
 */
ssize_t c_read_fd(lexec_t *exec, int fd, char *buffer, size_t len) {
    ssize_t bytes_read;

    bytes_read = read(fd, buffer, len);
    rt_assert(bytes_read != -1, le_system, strerror(errno));

    return bytes_read;
}

/**
 * write handler for bare unbuffered fds
 */
ssize_t c_write_fd(lexec_t *exec, int fd, char *buffer, size_t len) {
    ssize_t bytes_written;

    bytes_written = write(fd, buffer, len);
    rt_assert(bytes_written != -1, le_system, strerror(errno));

    /* is bytes_written != len an error?  If not, how do we pass
       it back to the interpreter? */

    return bytes_written;
}

/**
 * c dispatch function for reading an arbitrary port type
 */
ssize_t c_read(lexec_t *exec, lv_t *port, char *buffer, size_t len) {
    ssize_t res;

    assert(exec && port);
    assert(port->type == l_port);

    switch(L_PORT(port)->type) {
    case PT_FILE:
        /* will assert in c_read_fd on error */
        res = c_read_fd(exec, L_PORT(port)->info.fi.fd, buffer, len);
        if(!res)
            L_PORT(port)->eof = 1;
        break;
    default:
        /* unhandled port type */
        assert(0);
    }

    return res;
}

/**
 * c dispatch function for writing an arbitrary port type
 */
ssize_t c_write(lexec_t *exec, lv_t *port, char *buffer, size_t len) {
    assert(exec && port);
    assert(port->type == l_port);

    switch(L_PORT(port)->type) {
    case PT_FILE:
        return c_write_fd(exec, L_PORT(port)->info.fi.fd, buffer, len);
        break;
    default:
        /* unhandled port type */
        assert(0);
    }
}

/**
 * c_read_char - return a character, or -1 on eof,
 * asserting on any read errors
 */
int c_read_char(lexec_t *exec, lv_t *port) {
    int result;
    char data;

    assert(exec && port && port->type == l_port);

    if(L_PORT(port)->peek) {
        L_PORT(port)->peek = 0;
        return L_PORT(port)->peeked_char;
    }

    result = c_read(exec, port, &data, 1);
    if(result == 1)
        return data;

    return -1;
}

/**
 * c_peek_char - returns a character, or -1 on eof of
 * the next char to be read
 */
int c_peek_char(lexec_t *exec, lv_t *port) {
    int result;
    char data;

    assert(exec && port && port->type == l_port);
    if(L_PORT(port)->peek)
        return L_PORT(port)->peeked_char;

    /* otherwise, advance the read */
    L_PORT(port)->peek = 0;
    L_PORT(port)->peeked_char = c_read_char(exec, port);
    L_PORT(port)->peek = 1;

    return L_PORT(port)->peeked_char;
}


/**
 * (read-char port)
 */
lv_t *p_read_char(lexec_t *exec, lv_t *v) {
    int result;

    assert(exec);
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *port = L_CAR(v);
    rt_assert(port->type == l_port && L_PORT(port)->dir == PD_INPUT,
              le_type, "expecting input port");

    result = c_read_char(exec, port);

    if(result == -1)
        return lisp_create_null();  /* eof */

    return lisp_create_char(result);
}

/**
 * (peek-char port)
 */
lv_t *p_peek_char(lexec_t *exec, lv_t *v) {
    int result;

    assert(exec);
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *port = L_CAR(v);
    rt_assert(port->type == l_port && L_PORT(port)->dir == PD_INPUT,
              le_type, "expecting input port");

    result = c_peek_char(exec, port);

    if(result == -1)
        return lisp_create_null();  /* eof */
    return lisp_create_char(result);
}

lv_t *p_close_output_port(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");
    lv_t *port = L_CAR(v);
    rt_assert(L_PORT(port)->dir == PD_OUTPUT,
              le_type, "not an output port");

    switch(L_PORT(port)->type) {
    case PT_FILE:
        return c_close_file(exec, port);
        break;
    default:
        break;
    }

    /* an unhandled type */
    assert(0);
}

/**
 * (input-port? obj)
 *
 * returns #t if obj is an input port
 */
lv_t *p_input_portp(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *port = L_CAR(v);
    if((port->type == l_port) && (L_PORT(port)->dir == PD_INPUT)) {
        return lisp_create_bool(1);
    }
    return lisp_create_bool(0);
}


/**
 * (output-port? obj)
 *
 * returns #t if obj is an output port
 */
lv_t *p_output_portp(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *port = L_CAR(v);
    if((port->type == l_port) && (L_PORT(port)->dir == PD_OUTPUT)) {
        return lisp_create_bool(1);
    }
    return lisp_create_bool(0);
}



/*
 * These functions require a concept of current input port
 * and current output port.  These probably need to be
 * exec stack items, or perhaps private environment values.
 *
 * regardless, they are not yet implemented
 */
lv_t *p_current_input_port(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}

lv_t *p_current_output_port(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}

lv_t *p_with_input_from_file(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}

lv_t *p_with_output_to_file(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}

typedef enum token_type_t { T_QUOTE, T_QUASIQUOTE, T_UNQUOTESPLICING,
                            T_UNQUOTE, T_OPENPAREN, T_CLOSEPAREN,
                            T_DOT, T_INTEGER, T_RATIONAL, T_FLOAT,
                            T_BOOL, T_SYMBOL, T_STRING, T_CHAR, T_EOF
} token_type_t;

typedef struct token_t {
    char *s_value;
} token_t;

#define TOKENIZER_MAX_TOKEN 256

token_t *c_new_token(token_type_t tok, char *val) {
    switch(tok) {
    case T_QUOTE:
        fprintf(stderr, "Token: QUOTE\n");
        break;
    case T_QUASIQUOTE:
        fprintf(stderr, "Token: QUASIQUOTE\n");
        break;
    case T_UNQUOTESPLICING:
        fprintf(stderr, "Token: UNQUOTESPLICING\n");
        break;
    case T_UNQUOTE:
        fprintf(stderr, "Token: UNQUOTE\n");
        break;
    case T_OPENPAREN:
        fprintf(stderr, "Token: OPENPARN\n");
        break;
    case T_CLOSEPAREN:
        fprintf(stderr, "Token: CLOSEPAREN\n");
        break;
    case T_DOT:
        fprintf(stderr, "Token: DOT\n");
        break;
    case T_INTEGER:
        fprintf(stderr, "Token: INTEGER (%s)\n", val);
        break;
    case T_RATIONAL:
        fprintf(stderr, "Token: RATIONAL (%s)\n", val);
        break;
    case T_FLOAT:
        fprintf(stderr, "Token: FLOAT (%s)\n", val);
        break;
    case T_BOOL:
        fprintf(stderr, "Token: BOOL (%s)\n", val);
        break;
    case T_SYMBOL:
        fprintf(stderr, "Token: SYMBOL (%s)\n", val);
        break;
    case T_STRING:
        fprintf(stderr, "Token: STRING (%s)\n", val);
        break;
    case T_CHAR:
        fprintf(stderr, "Token: CHAR (%s)\n", val);
        break;
    case T_EOF:
        fprintf(stderr, "Token: EOF\n");
        break;
    }

    return NULL;
}

regex_t *c_compile_regex(char *expr) {
    int ret;
    regex_t *result;

    fprintf(stderr, "compiling %s\n", expr);

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

    fprintf(stderr, "Determine: %s\n", val);

    if(!strcmp(val, ".")) {
        return c_new_token(T_DOT, NULL);
    } else if(!strncmp(val, "#\\", 2)) {
        /* FIXME */
        fprintf(stderr, "char of some type\n");
    } else if(!strcmp(val, "'")) {
        return c_new_token(T_QUOTE, NULL);
    } else if(!strcmp(val, ",")) {
        return c_new_token(T_UNQUOTE, NULL);
    } else if(!strcmp(val, "`")) {
        return c_new_token(T_QUASIQUOTE, NULL);
    } else if(*val == '#') {
        /* FIXME */
        fprintf(stderr, "special value");
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

lv_t *p_toktest(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);

    lv_t *port = c_open_file(exec, v, PD_INPUT);

    fprintf(stderr, "\n");
    while(!L_PORT(port)->eof) {
        c_get_token(exec, port);
    }
    fprintf(stderr, "\n");
    return lisp_create_null();
}
