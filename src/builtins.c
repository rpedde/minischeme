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

#include "lisp-types.h"
#include "primitives.h"
#include "builtins.h"

static lv_t *s_is_type(lv_t *v, lisp_type_t t) {
    if(v->type == t)
        return lisp_create_bool(1);
    return lisp_create_bool(0);
}

lv_t *p_nullp(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "wrong arity");
    lv_t *a0 = L_CAR(v);

    return s_is_type(a0, l_null);
}

lv_t *p_symbolp(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "wrong arity");
    lv_t *a0 = L_CAR(v);

    return s_is_type(a0, l_sym);
}

lv_t *p_atomp(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "wrong arity");
    lv_t *a0 = L_CAR(v);

    if(a0->type != l_pair)
        return lisp_create_bool(1);
    return lisp_create_bool(0);
}

lv_t *p_consp(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "wrong arity");
    lv_t *a0 = L_CAR(v);

    return s_is_type(a0, l_pair);
}

lv_t *p_listp(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "wrong arity");
    lv_t *a0 = L_CAR(v);

    if((a0->type == l_pair) || (a0->type == l_null))
        return lisp_create_bool(1);
    return lisp_create_bool(0);
}

lv_t *p_pairp(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "wrong arity");
    lv_t *a0 = L_CAR(v);

    return lisp_create_bool(a0->type == l_pair);
}

/**
 * c helper for equalp
 */
int c_equalp(lv_t *a1, lv_t *a2) {
    int result = 0;

    if(a1->type != a2->type)
        return 0;

    switch(a1->type) {
    case l_int:
        result = (L_INT(a1) == L_INT(a2));
        break;
    case l_float:
        result = (L_FLOAT(a1) == L_FLOAT(a2));
        break;
    case l_bool:
        if((L_BOOL(a1) == 0 && L_BOOL(a2) == 0) ||
           (L_BOOL(a1) != 0 && L_BOOL(a1) != 0))
            result = 1;
        break;
    case l_sym:
        if(strcmp(L_SYM(a1), L_SYM(a2)) == 0)
            result = 1;
        break;
    case l_str:
        if(strcmp(L_STR(a1), L_STR(a2)) == 0)
            result = 1;
        break;
    case l_hash:
        result = (L_HASH(a1) == L_HASH(a2));
        break;
    case l_null:
        result = 1;
        break;
    case l_fn:
        result = (L_FN(a1) == L_FN(a1));
        break;
    case l_pair:
        /* this is perhaps not right */
        if(!(c_equalp(L_CAR(a1), L_CAR(a2))))
            return 0;
        if(L_CDR(a1) && L_CDR(a2))
            return c_equalp(L_CDR(a1), L_CDR(a2));
        if(!L_CDR(a1) && !L_CDR(a2))
            return 1;
        result = 0;
        break;
    }

    return result;
}

/**
 * lisp wrapper around c_equalp
 */
lv_t *p_equalp(lexec_t *exec, lv_t *v) {
    int result;

    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 2, le_arity, "wrong arity");

    lv_t *a1 = L_CAR(v);
    lv_t *a2 = L_CADR(v);

    return(lisp_create_bool(c_equalp(a1, a2)));
}

/**
 * add numeric types
 */
lv_t *p_plus(lexec_t *exec, lv_t *v) {
    int i_result = 0;
    double f_result = 0.0;

    lv_t *current = v;
    lv_t *op;

    lisp_type_t ret_type = l_int;

    assert(v && (v->type == l_pair || v->type == l_null));

    while(current && (current->type == l_pair)) {
        op = L_CAR(current);

        rt_assert((op->type == l_int || op->type == l_float),
                  le_type, "non-numeric add");

        /* promote result, if necessary */
        if(ret_type != op->type) {
            if(ret_type == l_int) {
                ret_type = l_float;
                f_result = (float)i_result;
            }
        }

        if(ret_type == l_float) {
            if(op->type == l_int) {
                f_result += (float)L_INT(op);
            } else {
                f_result += L_FLOAT(op);
            }
        } else {
            i_result += L_INT(op);
        }

        current = L_CDR(current);
    }

    if(ret_type == l_int)
        return lisp_create_int(i_result);

    return lisp_create_float(f_result);
}

lv_t *p_set_cdr(lexec_t *exec, lv_t *v) {
    assert(v && (v->type == l_pair));

    rt_assert(c_list_length(v) == 2, le_arity, "set-cdr arity");
    rt_assert(L_CAR(v)->type == l_pair, le_type, "set-cdr on non-pair");

    if(L_CADR(v)->type == l_null)
        L_CDR(L_CAR(v)) = NULL;
    else
        L_CDR(L_CAR(v)) = L_CADR(v);

    return lisp_create_null();
}

lv_t *p_set_car(lexec_t *exec, lv_t *v) {
    assert(v && (v->type == l_pair));

    rt_assert(c_list_length(v) == 2, le_arity, "set-cdr arity");
    rt_assert(L_CAR(v)->type == l_pair, le_type, "set-car on non-pair");

    L_CAR(L_CAR(v)) = L_CADR(v);
    return lisp_create_null();
}

lv_t *p_inspect(lexec_t *exec, lv_t *v) {
    lv_t *arg;
    int show_line = 1;
    char buffer[256];

    assert(v && (v->type == l_pair));
    rt_assert(c_list_length(v) == 1, le_arity, "inspect arity");

    arg = L_CAR(v);
    memset(buffer, 0, sizeof(buffer));

    strcat(buffer, "type: ");

    if(arg->type == l_fn) {
        if(L_FN(arg)) {
            strcat(buffer, "built-in function");
            show_line = 0;
        } else {
            strcat(buffer, "lambda, declared at");
        }
    } else {
        strcat(buffer, lisp_types_list[arg->type] + 2);
    }

    if(show_line)
        sprintf(buffer + strlen(buffer), " %s:%d:%d",
                arg->file, arg->row, arg->col);

    if(arg->bound)
        sprintf(buffer + strlen(buffer), ", bound to: %s",
                L_SYM(arg->bound));

    return lisp_create_string(buffer);
}

lv_t *p_load(lexec_t *exec, lv_t *v) {
    assert(v && (v->type == l_pair));
    rt_assert(c_list_length(v) == 1, le_arity, "load arity");
    rt_assert(L_CAR(v)->type == l_str, le_type, "filename must be string");

    return c_sequential_eval(exec, lisp_parse_file(L_STR(L_CAR(v))));
}

lv_t *p_length(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "length arity");
    return lisp_create_int(c_list_length(L_CAR(v)));
}

lv_t *p_assert(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "assert arity");
    rt_assert(L_CAR(v)->type == l_bool, le_type, "assert not bool");

    if(!L_BOOL(L_CAR(v)))
        rt_assert(0, le_internal, "error raised");

    return lisp_create_null();
}

lv_t *p_warn(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "warn arity");
    rt_assert(L_CAR(v)->type == l_bool, le_type, "warn not bool");

    if(!L_BOOL(L_CAR(v)))
        rt_assert(0, le_warn, "warning raised");

    return lisp_create_null();
}

lv_t *p_not(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "not arity");
    rt_assert(L_CAR(v)->type == l_bool, le_type, "not bool");

    return lisp_create_bool(!(L_BOOL(L_CAR(v))));
}

lv_t *p_car(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "car arity");
    rt_assert(L_CAR(v)->type == l_pair, le_type, "car on non-list");

    if(L_CAAR(v)->type == l_null)
        return lisp_create_null();

    return L_CAAR(v);
}

lv_t *p_cdr(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "cdr arity");
    rt_assert(L_CAR(v)->type == l_pair, le_type, "cdr on non-list");

    if(L_CDAR(v) == NULL)
        return lisp_create_null();

    return L_CDAR(v);
}

lv_t *p_cons(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 2, le_arity, "cons arity");
    return lisp_create_pair(L_CAR(v), L_CADR(v));
}

/**
 * generate a unique symbol.  This should probably
 */
lv_t *p_gensym(lexec_t *exec, lv_t *v) {
    static int sym_no=0;
    char buffer[20];

    assert(v && (v->type == l_pair || v->type == l_null));

    rt_assert(c_list_length(v) == 0, le_arity, "gensym arity");
    snprintf(buffer, sizeof(buffer), "<gensym-%05d>", sym_no++);

    return lisp_create_symbol(buffer);
}

/**
 * print a string
 */
lv_t *p_display(lexec_t *exec, lv_t *v) {
    lv_t *str;

    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "display arity");

    str = lisp_str_from_value(L_CAR(v));
    fprintf(stdout, "%s", L_STR(str));
    fflush(stdout);

    return lisp_create_null();
}

/**
 * given a string format specifier, create a new
 * string with the correct format
 */
lv_t *p_format(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    lv_t *current_arg = NULL;
    char *format, *current;
    char *return_buffer, *return_ptr;
    int len;

    /* make sure the format string is a string */
    rt_assert(L_CAR(v)->type == l_str, le_type, "bad format specifier");

    format = L_STR(L_CAR(v));
    current = format;

    current_arg = L_CDR(v);

    /* first, find out how long the destination string
     * must be, using the lisp_snprintf stuff */
    len = 0;
    while(*current) {
        if(*current != '~') {
            len++;
        } else {
            current++;
            switch(*current) {
            case 'A':
            case 'S':
                rt_assert(current_arg && L_CAR(current_arg),
                          le_arity, "insufficient args");

                len += lisp_snprintf(NULL, 0, L_CAR(current_arg));
                current_arg = L_CDR(current_arg);
                break;
            case '~':
            case '%':
                len++;
                break;
            default:
                rt_assert(0, le_syntax, "bad format specifier");
            }
        }
        current++;
    }

    fprintf(stderr, "propsed len: %d\n", len);

    current = format;
    current_arg = L_CDR(v);

    return_buffer = safe_malloc(len + 1);
    return_ptr = return_buffer;

    memset(return_buffer, 0, sizeof(return_buffer));

    int item_len = 0;

    while(*current) {
        if(*current != '~') {
            *return_ptr = *current;
            return_ptr++;
        } else {
            current++;
            if(*current == 'A' ||
               *current == 'S') {
                item_len = lisp_snprintf(return_ptr,
                                         len - (return_ptr - return_buffer) + 1,
                                         L_CAR(current_arg));
                current_arg = L_CDR(current_arg);
                return_ptr += item_len;
            } else if (*current == '~') {
                *return_ptr = '~';
                return_ptr++;
            } else if (*current == '%') {
                *return_ptr = '\n';
                return_ptr++;
            } else {
                rt_assert(0, le_syntax, "bad format specifier");
            }

        }
        current++;
    }

    /* FIXME: need a non-strduping create_string */
    return lisp_create_string(return_buffer);
}
