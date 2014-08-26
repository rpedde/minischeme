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
#include <stdint.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdarg.h>
#include <limits.h>

#include <gc.h>

#include "lisp-types.h"
#include "primitives.h"
#include "murmurhash.h"
#include "grammar.h"
#include "tokenizer.h"
#include "redblack.h"

#include "builtins.h"
#include "ports.h"
#include "char.h"
#include "math.h"

typedef struct hash_node_t {
    uint32_t key;
    lv_t *key_item;
    lv_t *value;
} hash_node_t;

typedef struct environment_list_t {
    char *name;
    lv_t *(*fn)(lexec_t *, lv_t *);
} environment_list_t;

typedef enum exec_stack_t {
    es_env,    // environment stack
    es_ex,     // exception stack
    es_eval    // eval stack (for backtrace)
} exec_stack_t;

static int stack_offsets[] = {
    offsetof(lexec_t, env_stack),
    offsetof(lexec_t, ex_stack),
    offsetof(lexec_t, eval_stack)
};

static int gmp_initialized = 0;
static jmp_buf *assert_handler = NULL;
static int emit_on_error = 1;

static environment_list_t s_env_global[] = {
    /* wants at least scheme-report-environment */
    { NULL, NULL }
};

static environment_list_t s_env_prim[] = {
    { "p-+", p_plus },
    { "p-null?", p_nullp },
    { "p-symbol?", p_symbolp },
    { "p-atom?", p_atomp },
    { "p-cons?", p_consp },
    { "p-list?", p_listp },
    { "p-pair?", p_pairp },
    { "p-equal?", p_equalp },
    { "p-set-cdr!", p_set_cdr },
    { "p-set-car!", p_set_car },
    { "p-length", p_length },
    { "p-inspect", p_inspect },
    { "p-load", p_load },
    { "p-assert", p_assert },
    { "p-warn", p_warn },
    { "p-not", p_not },
    { "p-cons", p_cons },
    { "p-car", p_car },
    { "p-cdr", p_cdr },
    { "p-gensym", p_gensym },
    { "p-display", p_display },
    { "p-format", p_format },

    // port functions
    { "p-input-port?", p_input_portp },
    { "p-output-port?", p_output_portp },
    { "p-open-input-file", p_open_input_file },
    { "p-open-output-file", p_open_output_file },
    { "p-close-input-port", p_close_input_port },
    { "p-close-output-port", p_close_output_port },
    { "p-read-char", p_read_char },
    { "p-peek-char", p_peek_char },

    { "p-toktest", p_toktest },
    { "p-parsetest", p_parsetest },
    { "p-read", p_read },

    // char functions
    { "p-char?", p_charp },
    { "p-char=?", p_charequalp },
    { "p-char<?", p_charltp },
    { "p-char>?", p_chargtp },
    { "p-char<=?", p_charltep },
    { "p-char>=?", p_chargtep },
    { "p-char->integer", p_char_integer },

    // math functions
    { "p-integer?", p_integerp },
    { "p-rational?", p_rationalp },
    { "p-float?" , p_floatp },
    { "p-exact?" , p_exactp },
    { "p-inexact?", p_inexactp },
    { "p->", p_gt },
    { "p-<", p_lt },
    { "p->=", p_gte },
    { "p-<=", p_lte },
    { "p-=", p_eq },
    { "p-+", p_plus },
    { "p--", p_minus },
    { "p-*", p_mul },
    { "p-/", p_div },
    { "p-quotient", p_quotient },
    { "p-remainder", p_remainder },
    { "p-modulo", p_modulo },
    { "p-floor", p_floor },
    { "p-ceiling", p_ceiling },
    { "p-truncate", p_truncate },
    { "p-round", p_round },

    // SRFI-6
    { "p-open-input-string", p_open_input_string },

    { NULL, NULL }
};

/**
 * do initial initialization of gmp library.  set allocators,
 * etc.
 */
void *gmp_realloc_wrapper(void *ptr, size_t old_size, size_t new_size) {
    return GC_realloc(ptr, new_size);
}

void gmp_free_wrapper(void *ptr, size_t size) {
    GC_free(ptr);
}

void maybe_initialize_gmp(void) {
    if(!gmp_initialized) {
        mp_set_memory_functions(GC_malloc,
                                gmp_realloc_wrapper,
                                gmp_free_wrapper);
        gmp_initialized = 1;
    }
}

void lisp_set_ehandler(lexec_t *exec, void(*handler)(lexec_t *exec)) {
    assert(exec);
    assert(handler);

    exec->ehandler = handler;
}

void null_ehandler(lexec_t *exec) {
}

void simple_ehandler(lexec_t *exec) {
    assert(exec);

    fprintf(stderr, "Error: %s: %s\n",
            lisp_exceptions_list[exec->exc]+3, exec->msg);
}

void default_ehandler(lexec_t *exec) {
    int index = 0;
    lstack_t *pstack;
    char buffer[256]; /* FIXME: decent display or print */
    int show_line;
    lv_t *arg;
    lisp_exception_t etype;
    char *msg;

    assert(exec);

    msg = exec->msg;
    etype = exec->exc;

    /* do a full stack backtrace */
    fprintf(stderr, "%s error: %s\n", lisp_exceptions_list[etype]+3, msg);

    pstack = exec->eval_stack;

    /* walk the execution stack backwards, dumping all the way */
    while(pstack) {
        show_line = 1;
        assert(pstack->data);

        arg = (lv_t*)pstack->data;

        if(arg->type == l_fn) {
            if(L_FN(arg)) {
                strcpy(buffer, "built-in function");
                show_line = 0;
            } else {
                strcpy(buffer, "lambda, declared at");
            }
        } else {
            strcat(buffer, lisp_types_list[arg->type] + 2);
        }

        if(show_line)
            sprintf(buffer + strlen(buffer), " %s:%d:%d",
                    arg->file,
                    arg->row,
                    arg->col);

        if(arg->bound)
            sprintf(buffer + strlen(buffer), ", bound to '%s'",
                    L_SYM(arg->bound));


        fprintf(stderr, "%d: %s\n", index, buffer);
        pstack = pstack->next;
        index++;
    }
}


void c_rt_assert(lexec_t *exec, lisp_exception_t etype, char *msg) {
    jmp_buf *pjb;

    assert(exec);
    assert(exec->ex_stack);

    exec->exc = etype;
    exec->msg = msg;

    if(exec->ex_stack) {
        pjb = (exec->ex_stack)->data;
        lisp_exec_pop_ex(exec);
        longjmp(*pjb, (int)etype);
    } else {
        default_ehandler(exec);
        exit((int)etype);
    }
}

void *safe_malloc(size_t size) {
    void *result = GC_malloc(size);
    if(!result) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    return result;
}

char *safe_strdup(char *str) {
    char *result = safe_malloc(strlen(str) + 1);
    strcat(result, str);
    return result;
}

static int s_hash_cmp(const void *a, const void *b, const void *e) {
    hash_node_t *v1 = (hash_node_t*)a;
    hash_node_t *v2 = (hash_node_t*)b;

    if(v1->key > v2->key)
        return 1;
    if(v1->key < v2->key)
        return -1;
    return 0;
}

static int s_hash_item(lv_t *item) {
    assert(item);

    if(item->type == l_str)
	return murmurhash2(L_STR(item), strlen(L_STR(item)), 0);
    if(item->type == l_sym)
	return murmurhash2(L_SYM(item), strlen(L_SYM(item)), 0);

    assert(0);
}

void c_hash_walk(lv_t *hash, void(*callback)(lv_t *, lv_t *)) {
    assert(hash && hash->type == l_hash);
    assert(callback);

    void hash_walker(const void *np, const VISIT w, const int d, void *msg) {
        if(w == leaf || w == postorder) {
            // inorder, strangely
            callback(((hash_node_t *)np)->key_item,
                     ((hash_node_t *)np)->value);
        }
    }

    rbwalk(L_HASH(hash), hash_walker, NULL);
}

lv_t *c_hash_fetch(lv_t *hash, lv_t *key) {
    hash_node_t node_key;
    hash_node_t *result;

    assert(hash && hash->type == l_hash);
    assert(key && (key->type == l_str || key->type == l_sym));

    node_key.key = s_hash_item(key);

    result = (hash_node_t *)rbfind(&node_key, L_HASH(hash));
    if(!result)
        return NULL;

    return result->value;
}

int c_hash_insert(lv_t *hash,
                  lv_t *key,
                  lv_t *value) {

    hash_node_t *pnew;
    hash_node_t *result;

    assert(hash->type == l_hash);
    assert(key->type == l_str || key->type == l_sym);

    pnew=safe_malloc(sizeof(hash_node_t));
    pnew->key = s_hash_item(key);
    pnew->key_item = key;
    pnew->value = value;

    result = (hash_node_t *)rbsearch((void*)pnew, L_HASH(hash));

    assert(result);

    result->value = value;
    result->key_item = key;

    return(result != NULL);
}

int c_hash_delete(lv_t *hash, lv_t *key) {
    hash_node_t node_key;
    hash_node_t *result;

    assert(hash->type == l_hash);
    assert(key->type == l_str || key->type == l_sym);

    node_key.key = s_hash_item(key);
    result = (hash_node_t *)rbdelete(&node_key, L_HASH(hash));
    return(result != NULL);
}

lv_t *lisp_create_null(void) {
    lv_t *result = safe_malloc(sizeof(lv_t));
    result->type = l_null;
    return result;
}

lv_t *lisp_create_hash(void) {
    lv_t *result;

    result = safe_malloc(sizeof(lv_t));
    result->type = l_hash;
    L_HASH(result) = rbinit(s_hash_cmp, NULL);

    assert(L_HASH(result));

    return result;
}

lv_t *lisp_create_pair(lv_t *car, lv_t *cdr) {
    lv_t *result;

    result = safe_malloc(sizeof(lv_t));

    result->type = l_pair;
    L_CAR(result) = car;

    if(cdr && cdr->type == l_null)
        L_CDR(result) = NULL;
    else
        L_CDR(result) = cdr;

    return result;
}

lv_t *lisp_create_type(void *value, lisp_type_t type) {
    lv_t *result;

    result = safe_malloc(sizeof(lv_t));

    result->type = type;

    result->row = 0;
    result->col = 0;
    result->file = NULL;

    switch(type) {
    case l_char:
        L_CHAR(result) = *((char*)value);
        break;
    case l_int:
        mpz_init(L_INT(result));
        mpz_set_si(L_INT(result), *(int64_t *)value);
        break;
    case l_rational:
        mpq_init(L_RAT(result));
        break;
    case l_float:
        mpfr_init(L_FLOAT(result));
        mpfr_set_d(L_FLOAT(result), *(double*)value, MPFR_ROUND_TYPE);
        break;
    case l_bool:
        L_BOOL(result) = *((int*)value);
        break;
    case l_sym:
        L_SYM(result) = safe_strdup((char*)value);
        break;
    case l_str:
        L_STR(result) = safe_strdup((char*)value);
        break;
    case l_fn:
        L_FN(result) = (lisp_method_t)value;
        break;
    case l_port:
        L_PORT(result) = (port_info_t *)value;
        break;
    default:
        assert(0);
        fprintf(stderr, "Bad type");
        exit(EXIT_FAILURE);
    }

    return result;
}

/**
 * typechecked wrapper around lisp_create_type for chars
 */
lv_t *lisp_create_char(char value) {
    return lisp_create_type((void*)&value, l_char);
}

/**
 * typechecked wrapper around lisp_create_type for strings
 */
lv_t *lisp_create_string(char *value) {
    return lisp_create_type((void*)value, l_str);
}

/**
 * typechecked wrapper around lisp_create_type for formatted strings
 */
lv_t *lisp_create_formatted_string(char *fmt, ...) {
    va_list args;
    char *tmp;
    lv_t *v;
    int res;

    va_start(args, fmt);
    res = vasprintf(&tmp, fmt, args);
    va_end(args);

    if(res == -1) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    v = lisp_create_type((void*)tmp, l_str);
    free(tmp);
    return v;
}

/**
 * typechecked wrapper around lisp_create_type for symbols
 */
lv_t *lisp_create_symbol(char *value) {
    return lisp_create_type((void*)value, l_sym);
}

/**
 * typechecked wrapper around lisp_create_type for rationals
 */
lv_t *lisp_create_rational(int64_t n, int64_t d) {
    lv_t *new_value = lisp_create_type(NULL, l_rational);
    mpq_set_si(L_RAT(new_value), n, d);
    mpq_canonicalize(L_RAT(new_value));
    return new_value;
}

/**
 * lisp_create_type for rational, using the string parser
 * (to be able to represent arbitrary precision).  This
 * is the preferred interface
 */
lv_t *lisp_create_rational_str(char *value) {
    double v = 0;
    int flag;

    lv_t *new_value = lisp_create_type(NULL, l_rational);

    /* now parse the string */
    flag = mpq_set_str(L_RAT(new_value), value, 10);
    assert(!flag);

    mpq_canonicalize(L_RAT(new_value));
    return new_value;
}

/**
 * typechecked wrapper around lisp_create_type for floats
 */
lv_t *lisp_create_float(double value) {
    return lisp_create_type((void*)&value, l_float);
}

/**
 * lisp_create_type for float, using the string parser
 * (to be able to represent arbitrary precision).  This
 * is the preferred interface
 */
lv_t *lisp_create_float_str(char *value) {
    double v = 0;
    int flag;

    lv_t *new_value = lisp_create_type((void*)&v, l_float);

    /* now parse the string */
    flag = mpfr_set_str(L_FLOAT(new_value), value, 10, MPFR_ROUND_TYPE);
    assert(!flag);

    return new_value;
}

/**
 * typechecked wrapper around lisp_create_type for ints
 */
lv_t *lisp_create_int(int64_t value) {
    return lisp_create_type((void*)&value, l_int);
}

/**
 * lisp_create_type for int, using the string parser
 * (to be able to represent arbitrary precision).  This
 * is the preferred interface
 */
lv_t *lisp_create_int_str(char *value) {
    int64_t v = 0;
    int flag;

    lv_t *new_value = lisp_create_type((void*)&v, l_int);

    /* now parse the string */
    flag = mpz_set_str(L_INT(new_value), value, 10);
    assert(!flag);

    return new_value;
}

/**
 * typechecked wrapper around lisp_create_type for bools
 */
lv_t *lisp_create_bool(int value) {
    return lisp_create_type((void*)&value, l_bool);
}

/**
 * typechecked wrapper around lisp_create_type for functions
 */
lv_t *lisp_create_native_fn(lisp_method_t value) {
    lv_t *fn = lisp_create_type(value, l_fn);
    L_FN_FTYPE(fn) = lf_native;
    L_FN_ARGS(fn) = NULL;
    L_FN_BODY(fn) = NULL;
    L_FN_ENV(fn) = NULL;

    return fn;
}

/**
 * create a port type
 *
 * FIXME: needs fclose finalizer
 */
lv_t *lisp_create_port(port_info_t *pi) {
    lv_t *p = lisp_create_type((void*)pi, l_port);

    return p;
}


/**
 * defmacro
 */
lv_t *lisp_create_macro(lexec_t *exec, lv_t *formals, lv_t *form) {
    assert(exec);
    rt_assert(formals->type == l_pair ||
              formals->type == l_null ||
              formals->type == l_sym, le_type,
              "formals must be a list, symbol, or ()");

    lv_t *fn = lisp_create_type(NULL, l_fn);
    L_FN_FTYPE(fn) = lf_macro;
    L_FN_ENV(fn) = exec->env;
    L_FN_ARGS(fn) = formals;
    L_FN_BODY(fn) = form;

    return fn;
}


/**
 * lambda-style function, not builtin
 */
lv_t *lisp_create_lambda(lexec_t *exec, lv_t *formals, lv_t *body) {
    assert(exec);

    rt_assert(formals->type == l_pair ||
              formals->type == l_null ||
              formals->type == l_sym, le_type,
              "formals must be a list, symbol, or ()");

    lv_t *fn = lisp_create_type(NULL, l_fn);
    L_FN_FTYPE(fn) = lf_lambda;
    L_FN_ENV(fn) = exec->env;
    L_FN_ARGS(fn) = formals;
    L_FN_BODY(fn) = body;

    return fn;
}

/**
 * stamp row/col/file information on a type
 */
void lisp_stamp_value(lv_t *v, int row, int col, char *file) {
    v->row = row;
    v->col = col;
    v->file = file;
}

int lisp_snprintf(char *buf, int len, lv_t *v) {
    int pair_len = 0;

    switch(v->type) {
    case l_null:
        return snprintf(buf, len, "()");
    case l_int:
        /* return snprintf(buf, len, "%" PRIu64, L_INT(v)); */
        return gmp_snprintf(buf, len, "%Zd", L_INT(v));
    case l_rational:
        return gmp_snprintf(buf, len, "%Qd", L_RAT(v));
    case l_float:
        /* return snprintf(buf, len, "%0.16g", L_FLOAT(v)); */
        return mpfr_snprintf(buf, len, "%Rg", L_FLOAT(v));
    case l_bool:
        return snprintf(buf, len, "%s", L_BOOL(v) ? "#t": "#f");
    case l_sym:
        return snprintf(buf, len, "%s", L_SYM(v));
    case l_str:
        return snprintf(buf, len, "\"%s\"", L_STR(v));
    case l_pair:
        if(len >= 1)
            sprintf(buf, "(");

        pair_len += 1;

        lv_t *vp = v;

        while(vp && L_CAR(vp)) {
            pair_len += lisp_snprintf(buf + pair_len,
                                      (len - pair_len) > 0 ? len - pair_len : 0,
                                      L_CAR(vp));

            if(L_CDR(vp) && (L_CDR(vp)->type != l_pair)) {
                pair_len += snprintf(buf + pair_len,
                                     (len - pair_len) > 0 ? len - pair_len : 0,
                                     " . ");

                pair_len += lisp_snprintf(buf + pair_len,
                                          (len - pair_len) > 0 ? len - pair_len : 0,
                                          L_CDR(vp));
                vp = NULL;
            } else {
                vp = L_CDR(vp);
                if(vp) {
                    if (len - pair_len > 0)
                        snprintf(buf + pair_len, len - pair_len, " ");
                    pair_len++;
                }
            }
        }

        if (len - pair_len > 0) {
            sprintf(buf + pair_len, ")");
        }

        pair_len++;
        return pair_len;
        break;
    case l_fn:
        if(L_FN(v) == NULL)
            return snprintf(buf, len, "<lambda@%p>", v);
        else
            return snprintf(buf, len, "<built-in@%p>", v);
        break;
    case l_char:
        return snprintf(buf, len, "%c", L_CHAR(v));
        break;
    case l_port:
        return snprintf(buf, len, "<port@%p>", v);
        break;
    default:
        // missing a type check.
        assert(0);
    }

}

lv_t *lisp_str_from_value(lv_t *v) {
    int len = lisp_snprintf(NULL, 0, v);
    char *buf = safe_malloc(len + 1);

    memset(buf, 0, len + 1);
    lisp_snprintf(buf, len + 1, v);

    return lisp_create_string(buf);
}

/**
 * print a value to a fd, in a debug form
 */
void lisp_dump_value(int fd, lv_t *v, int level) {
    switch(v->type) {
    case l_null:
        dprintf(fd, "()");
        break;
    case l_int:
        dprintf(fd, "%" PRIu64, L_INT(v));
        break;
    case l_float:
        dprintf(fd, "%0.16g", L_FLOAT(v));
        break;
    case l_bool:
        dprintf(fd, "%s", L_BOOL(v) ? "#t": "#f");
        break;
    case l_sym:
        dprintf(fd, "%s", L_SYM(v));
        break;
    case l_str:
        dprintf(fd, "\"%s\"", L_STR(v));
        break;
    case l_char:
        dprintf(fd, "#\%02x", L_CHAR(v));
        break;
    case l_pair:
        dprintf(fd, "(");
        lv_t *vp = v;
        while(vp && L_CAR(vp)) {
            lisp_dump_value(fd, L_CAR(vp), level + 1);
            if(L_CDR(vp) && (L_CDR(vp)->type != l_pair)) {
                dprintf(fd, " . ");
                lisp_dump_value(fd, L_CDR(vp), level + 1);
                vp = NULL;
            } else {
                vp = L_CDR(vp);
                dprintf(fd, "%s", vp ? " " : "");
            }
        }
        dprintf(fd, ")");
        break;
    case l_fn:
        if(L_FN(v) == NULL)
            dprintf(fd, "<lambda@%p>", v);
        else
            dprintf(fd, "<built-in@%p>", v);
        break;
    default:
        // missing a type check.
        assert(0);
    }
}

lv_t *lisp_args_overlay(lexec_t *exec, lv_t *formals, lv_t *args) {
    lv_t *pf, *pa;
    lv_t *env_layer;

    assert(formals->type == l_pair ||
           formals->type == l_null ||
           formals->type == l_sym);
    assert(args->type == l_pair || args->type == l_null || args->type == l_sym);

    env_layer = lisp_create_hash();
    pf = formals;
    pa = args;

    /* no args */
    if(pf->type == l_null) {
        rt_assert(c_list_length(pa) == 0, le_arity, "too many arguments");
        return env_layer;
    }

    /* single arg gets the whole list */
    if(pf->type == l_sym) {
        c_hash_insert(env_layer, pf, lisp_copy_list(pa));
        return env_layer;
    }

    /* walk through the formal list, matching to args */
    while(pf && L_CAR(pf)) {
        rt_assert(pa && L_CAR(pa), le_arity, "not enough arguments");
        c_hash_insert(env_layer, L_CAR(pf), L_CAR(pa));
        pf = L_CDR(pf);
        pa = L_CDR(pa);

        if(pf && pf->type == l_sym) {
            /* improper list */
            if(!pa) {
                c_hash_insert(env_layer, pf, lisp_create_null());
            } else {
                c_hash_insert(env_layer, pf, lisp_copy_list(pa));
            }
            return env_layer;
        }

        rt_assert(!pf || pf->type == l_pair, le_type, "unexpected formal type");
    }

    rt_assert(!pa, le_arity, "too many arguments");

    return env_layer;
}

lv_t *lisp_exec_fn(lexec_t *exec, lv_t *fn, lv_t *args) {
    lv_t *parsed_args;
    lv_t *layer, *newenv;
    lv_t *macrofn;
    lv_t *result;

    assert(exec && fn && args);
    rt_assert(fn->type == l_fn, le_type, "not a function");

    lisp_exec_push_eval(exec, fn);

    switch(L_FN_FTYPE(fn)) {
    case lf_native:
        result = L_FN(fn)(exec, args);
        break;
    case lf_lambda:
        layer = lisp_args_overlay(exec, L_FN_ARGS(fn), args);
        newenv = lisp_create_pair(layer, L_FN_ENV(fn));
        lisp_exec_push_env(exec, newenv);
        result = lisp_eval(exec, L_FN_BODY(fn));
        lisp_exec_pop_env(exec);
        break;
    case lf_macro:
        layer = lisp_args_overlay(exec, L_FN_ARGS(fn), args);
        newenv = lisp_create_pair(layer, L_FN_ENV(fn));
        lisp_exec_push_env(exec, newenv);
        macrofn = lisp_eval(exec, L_FN_BODY(fn));
        result = lisp_eval(exec, macrofn);
        lisp_exec_pop_env(exec);
        break;
    default:
        assert(0);
    }

    lisp_exec_pop_eval(exec);

    return result;
}

/**
 * evaluate an expression under let
 */
lv_t *lisp_let(lexec_t *exec, lv_t *args, lv_t *expr) {
    lv_t *argp = args;
    lv_t *newenv;
    lv_t *result;

    newenv = lisp_create_pair(lisp_create_hash(), exec->env);

    rt_assert(args->type == l_null ||
              args->type == l_pair, le_type,
              "let arg type");

    if(args->type == l_pair) {
        /* walk through each element of the list,
           evaling k/v pairs and assigning them
           to an environment to run the expr in */
        while(argp && L_CAR(argp)) {
            rt_assert(c_list_length(L_CAR(argp)) == 2, le_arity,
                      "let arg arity");
            c_hash_insert(L_CAR(newenv), L_CAAR(argp),
                          lisp_eval(exec, L_CADAR(argp)));
            argp=L_CDR(argp);
        }
    }

    lisp_exec_push_env(exec, newenv);
    result = lisp_eval(exec, expr);
    lisp_exec_pop_env(exec);

    return result;
}

lv_t *lisp_let_star(lexec_t *exec, lv_t *args, lv_t *expr) {
    lv_t *argp = args;
    lv_t *newenv;
    lv_t *result;

    newenv = lisp_create_pair(lisp_create_hash(), exec->env);

    rt_assert(args->type == l_null ||
              args->type == l_pair, le_type,
              "let arg type");

    lisp_exec_push_env(exec, newenv);

    if(args->type == l_pair) {
        /* walk through each element of the list,
           evaling k/v pairs and assigning them
           to an environment to run the expr in */
        while(argp && L_CAR(argp)) {
            rt_assert(c_list_length(L_CAR(argp)) == 2, le_arity,
                      "let arg arity");
            c_hash_insert(L_CAR(newenv), L_CAAR(argp),
                          lisp_eval(exec, L_CADAR(argp)));
            argp=L_CDR(argp);
        }
    }

    result = lisp_eval(exec, expr);
    lisp_exec_pop_env(exec);

    return result;
}

/**
 * quasiquote a term
 */
lv_t *lisp_quasiquote(lexec_t *exec, lv_t *v) {
    lv_t *res;
    lv_t *vptr;
    lv_t *rptr;
    lv_t *v2, *v2ptr;

    /* strategy: walk through the list, expanding
       unquote and unquote-splicing terms */
    if(v->type == l_pair) {
        if (L_CAR(v)->type == l_sym &&
            !strcmp(L_SYM(L_CAR(v)), "unquote")) {
            rt_assert(c_list_length(L_CDR(v)) == 1, le_arity,
                      "unquote arity");
            return lisp_eval(exec, L_CADR(v));
        }

        /* quasi-quote and unquote-splice stuff */
        res = lisp_create_pair(NULL, NULL);
        rptr = res;
        vptr = v;
        while(vptr && L_CAR(vptr)) {
            if(L_CAR(vptr)->type == l_pair &&
               L_CAAR(vptr)->type == l_sym &&
               !strcmp(L_SYM(L_CAAR(vptr)), "unquote-splicing")) {
                /* splice this into result */
                rt_assert(c_list_length(L_CDAR(vptr)) == 1, le_arity,
                          "unquote-splicing arity");

                v2 = lisp_eval(exec, L_CAR(L_CDAR(vptr)));
                rt_assert(v2->type == l_pair || v2->type == l_null, le_type,
                          "unquote-splicing expects list");

                if(v2->type != l_null) {
                    v2ptr = v2;
                    while(v2ptr && L_CAR(v2ptr)) {
                        L_CAR(rptr) = L_CAR(v2ptr);
                        v2ptr = L_CDR(v2ptr);
                        if(v2ptr) {
                            L_CDR(rptr) = lisp_create_pair(NULL, NULL);
                            rptr = L_CDR(rptr);
                        }
                    }
                }
            } else {
                L_CAR(rptr) = lisp_quasiquote(exec, L_CAR(vptr));
            }

            vptr = L_CDR(vptr);
            if(vptr) {
                L_CDR(rptr) = lisp_create_pair(NULL, NULL);
                rptr = L_CDR(rptr);
            }
        }

        return res;
    } else {
        return v;
    }
}


/**
 * evaluate a lisp value
 */
lv_t *lisp_eval(lexec_t *exec, lv_t *v) {
    lv_t *env;
    lv_t *fn;
    lv_t *args;
    lv_t *result;
    lv_t *a0, *a1, *a2, *a3;

    assert(exec);

    if(v->type == l_sym) {
        result = c_env_lookup(exec->env, v);
        if(result)
            return result;
    } /* otherwise, return symbol... */

    if(v->type != l_pair) {  // atom?
        return v;
    }

    /* check for special forms and functions */

    if(v->type == l_pair) {
	/* test special forms first */
	if(L_CAR(v)->type == l_sym) {
            if(strcmp(L_SYM(L_CAR(v)), "quote") == 0) {
		return lisp_quote(exec, L_CDR(v));
            } else if(!strcmp(L_SYM(L_CAR(v)), "define")) {
                rt_assert(c_list_length(L_CDR(v)) == 2, le_arity,
                          "define arity");
                result = lisp_eval(exec, L_CADDR(v));
                if(!result->bound)
                    result->bound = L_CADR(v);

                return lisp_define(exec, L_CADR(v), result);
            } else if(!strcmp(L_SYM(L_CAR(v)), "lambda")) {
                rt_assert(c_list_length(L_CDR(v)) == 2, le_arity,
                          "lambda arity");
                result = lisp_create_lambda(exec, L_CADR(v), L_CADDR(v));
                lisp_stamp_value(result, v->row, v->col, v->file);
                return result;
            } else if(!strcmp(L_SYM(L_CAR(v)), "defmacro")) {
                rt_assert(c_list_length(L_CDR(v)) == 3, le_arity,
                          "defmacro arity");
                a1 = L_CADR(v);                  // name
                a2 = L_CADDR(v);                 // lambda-list/formals
                a3 = L_CADDDR(v);                // form

                rt_assert(a1->type == l_sym, le_type,
                          "defmacro wrong type for name");

                return lisp_define(exec, a1, lisp_create_macro(exec, a2, a3));
            } else if(!strcmp(L_SYM(L_CAR(v)), "begin")) {
                rt_assert(L_CADR(v), le_arity, "begin arity");
                return lisp_begin(exec, L_CDR(v));
            } else if(!strcmp(L_SYM(L_CAR(v)), "quasiquote")) {
                rt_assert(
                    L_CDR(v)->type == l_null ||
                    (L_CDR(v)->type == l_pair && c_list_length(L_CDR(v)) == 1),
                    le_arity,
                    "quasiquote arity");
                return lisp_quasiquote(exec, L_CADR(v));
            } else if(!strcmp(L_SYM(L_CAR(v)), "if")) {
                rt_assert(c_list_length(L_CDR(v)) == 3, le_arity,
                          "if arity");
                a1 = lisp_eval(exec, L_CADR(v));  // expression
                a2 = L_CADDR(v);                 // value if true
                a3 = L_CADDDR(v);                // value if false

                if(a1->type == l_bool && L_BOOL(a1) == 0)
                    return lisp_eval(exec, a3);
                return lisp_eval(exec, a2);
            } else if(!strcmp(L_SYM(L_CAR(v)), "let")) {
                rt_assert(c_list_length(L_CDR(v)) == 2, le_arity,
                          "let arity");
                a1 = L_CADR(v);                  // tuple assignment list
                a2 = L_CADDR(v);                 // eval under let

                return lisp_let(exec, a1, a2);
            } else if(!strcmp(L_SYM(L_CAR(v)), "let*")) {
                rt_assert(c_list_length(L_CDR(v)) == 2, le_arity,
                          "let arity");
                a1 = L_CADR(v);                  // tuple assignment list
                a2 = L_CADDR(v);                 // eval under let

                return lisp_let_star(exec, a1, a2);
            }
	}

        /* otherwise, eval all the items, and execute */
        result = lisp_map(exec, lisp_create_pair(lisp_create_native_fn(lisp_eval), v));

        /* make sure it's a function */
        fn = L_CAR(result);
        args = L_CDR(result);

        rt_assert(fn->type == l_fn, le_type, "eval a non-function");

        if(!args)
            args = lisp_create_null();

        /* and go. */
        return lisp_exec_fn(exec, fn, args);

	/* /\* test symbols *\/ */
	/* if(v->type == l_fn) */
        /*     fn = v; */
	/* else if(L_CAR(v)->type == l_sym) { */
        /*     lv_t *tmp = c_env_lookup(env, L_CAR(v)); */
        /*     rt_assert(tmp, le_lookup, "unknown function"); */
        /*     rt_assert(tmp->type == l_fn, le_type, "eval a non-function"); */
        /*     fn = tmp; */
	/* } */

        /* lv_t *eval_fn = lisp_create_native_fn(lisp_eval); */

	/* /\* execute the function *\/ */
        /* args = L_CDR(v); */
        /* if(!args) */
        /*     args = lisp_create_null(); */

	/* return L_FN(fn)(env, lisp_map(env, c_make_list(eval_fn, args, NULL))); */
    }

    assert(0);
}

/**
 * return a quoted expression
 */
lv_t *lisp_quote(lexec_t *exec, lv_t *v) {
    assert(exec);
    rt_assert(c_list_length(v) == 1, le_arity, "quote arity");
    return L_CAR(v);
}

/**
 * find the length of a list
 */
int c_list_length(lv_t *v) {
    lv_t *current = v;
    int count = 0;

    assert(v->type == l_pair || v->type == l_null);

    if(v->type == l_null)
        return 0;

    while(current) {
        count++;
        current = L_CDR(current);
    }

    return count;
}

/**
 * map a function onto a list, returning the
 * resulting list
 */
lv_t *lisp_map(lexec_t *exec, lv_t *v) {
    lv_t *vptr;
    lv_t *result = lisp_create_pair(NULL, NULL);
    lv_t *rptr = result;
    lv_t *fn, *list;

    assert(exec);

    fn = L_CAR(v);
    list = L_CDR(v);

    rt_assert(fn->type == l_fn, le_type, "map with non-function");
    rt_assert((list->type == l_pair) || (list->type == l_null),
              le_type, "map to non-list");

    if(list->type == l_null)
        return list;

    vptr = list;

    while(vptr) {
        L_CAR(rptr) = L_FN(fn)(exec, L_CAR(vptr));
        vptr=L_CDR(vptr);
        if(vptr) {
            L_CDR(rptr) = lisp_create_pair(NULL, NULL);
            rptr = L_CDR(rptr);
        }
    }

    return result;
}


/**
 * execute a lisp function with the passed arg list
 */
lv_t *lisp_apply(lexec_t *exec, lv_t *v) {
    lv_t *fn, *list;

    assert(exec);
    rt_assert(c_list_length(v) == 2, le_arity, "apply arity");

    fn = L_CAR(v);
    list = L_CDR(v);

    rt_assert(fn->type == l_fn, le_type, "apply with non-function");
    rt_assert(list->type == l_pair, le_type, "apply to non-list");

    /* here we would check arity, and do arg fixups
     * (&rest, etc) */

    return L_FN(fn)(exec, list);
}

/**
 * hard to get to the rest of the functions without
 * having primitive access to environments, so...
 */

lv_t *null_environment(lexec_t *exec, lv_t *v) {
    lv_t *newenv = lisp_create_hash();

    assert(exec);

    return lisp_create_pair(newenv, NULL);
}

lv_t *c_env_version(int version) {
    environment_list_t *current = s_env_prim;
    lv_t *p_layer = lisp_create_hash();
    lv_t *newenv;
    char filename[40];
    lexec_t *exec;

    newenv = lisp_create_pair(lisp_create_hash(),
                              lisp_create_pair(p_layer, NULL));


    exec = safe_malloc(sizeof(lexec_t));
    memset(exec, 0, sizeof(lexec_t));
    exec->env = newenv;

    snprintf(filename, sizeof(filename), "env/r%d.scm", version);

    /* now, load up a primitive environment */
    while(current && current->name) {
        c_hash_insert(p_layer, lisp_create_string(current->name),
                      lisp_create_native_fn(current->fn));
        current++;
    }

    /* now, run the setup environment */
    p_load(exec, lisp_create_pair(lisp_create_string(filename), NULL));

    /* and return just the generated environment */
    return lisp_create_pair(L_CAR(exec->env), NULL);
}

/**
 * given a file, return an ast
 */
lv_t *lisp_parse_file(char *file) {
    void *parser = ParseAlloc(safe_malloc);
    YY_BUFFER_STATE buffer;
    int yv;
    void *scanner;
    YYLTYPE yyl = {0, 0, 0, 0};
    YYLTYPE yyl_error;
    YYSTYPE yys;
    lexer_shared_t lst = { NULL, &yyl, file, &yyl_error, 0 };
    FILE *f;

    f = fopen(file, "r");

    yylex_init(&scanner);
    buffer = yy_create_buffer(f, 16384, scanner);
    yy_switch_to_buffer(buffer, scanner);

    while((yv = yylex(&yys, &yyl, scanner)) != 0) {
        Parse(parser, yv, yys, &lst);
    }

    Parse(parser, 0, yys, &lst);
    yylex_destroy(scanner);

    fclose(f);

    if(lst.error) {
        return NULL;
    }
    return lst.result;
}

/**
 * given a string, return an ast
 */
lv_t *lisp_parse_string(char *string) {
    YY_BUFFER_STATE buffer;
    void *parser = ParseAlloc(safe_malloc);
    int yv;
    void *scanner;
    YYLTYPE yyl = {0, 0, 0, 0 };
    YYSTYPE yys;
    YYLTYPE yyl_error;
    lexer_shared_t lst = { NULL, &yyl, "<stdin>", &yyl_error, 0 };

    yylex_init(&scanner);
    buffer = yy_scan_string(string, scanner);

    while((yv = yylex(&yys, &yyl, scanner)) != 0) {
        Parse(parser, yv, yys, &lst);
    }

    Parse(parser, 0, yys, &lst);
    yylex_destroy(scanner);

    if(lst.error) {
        return NULL;
    }
    return lst.result;
}

lv_t *lisp_define(lexec_t *exec, lv_t *sym, lv_t *v) {
    assert(exec);

    /* this is probably not a good or completely safe
     * check of an environment */
    rt_assert(exec->env->type == l_pair &&
              L_CAR(exec->env) &&
              L_CAR(exec->env)->type == l_hash, le_type,
              "Not a valid environment");

    rt_assert(sym->type == l_sym, le_type, "cannot define non-symbol");

    rt_assert(c_hash_insert(L_CAR(exec->env), sym, v), le_internal,
        "error inserting hash element");

    return lisp_create_null();
}

/**
 * make a sequence of lv_t into a list, terminated by NULL
 */
lv_t *c_make_list(lv_t *item, ...) {
    va_list argp;
    lv_t *result;
    lv_t *head;
    lv_t *arg;

    if(!item)
        return lisp_create_null();

    va_start(argp, item);

    result = lisp_create_pair(item, NULL);
    head = result;

    while((arg = va_arg(argp, lv_t *))) {
        L_CDR(head) = lisp_create_pair(arg, NULL);
        head = L_CDR(head);
    }

    va_end(argp);

    return result;
}

lv_t *c_env_lookup(lv_t *env, lv_t *key) {
    lv_t *current;
    lv_t *result;

    assert(env->type == l_pair &&
           L_CAR(env) &&
           L_CAR(env)->type == l_hash);

    current=env;
    while(current) {
        if((result = c_hash_fetch(L_CAR(current), key)))
            return result;
        current = L_CDR(current);
    }

    return NULL;
}

/**
 * begin special form
 *
 * (begin (expr1 expr2 expr3))
 */
lv_t *lisp_begin(lexec_t *exec, lv_t *v) {
    lv_t *current;
    lv_t *retval;

    assert(exec);
    rt_assert(v->type == l_pair, le_type, "cannot begin non-list");

    current = v;
    while(v && (L_CAR(v))) {
        retval = lisp_eval(exec, L_CAR(v));
        v = L_CDR(v);
    }

    return retval;
}

/**
 * eval a list of items, one after the other, returning the
 * value of the last eval
 */
lv_t *c_sequential_eval(lexec_t *exec, lv_t *v) {
    lv_t *current = v;
    lv_t *result;

    assert(exec);
    assert(v->type == l_pair || v->type == l_null);

    if(v->type == l_null)
        return v;

    while(current && L_CAR(current)) {
        result = lisp_eval(exec, L_CAR(current));
        current = L_CDR(current);
    }

    return result;
}

/**
 * wrap a lisp value in a symbol (quote, dequote, etc)
 */
lv_t *lisp_wrap_type(char *symv, lv_t *v) {
    lv_t *cdr = lisp_create_pair(v, NULL);
    lv_t *car = lisp_create_pair(lisp_create_symbol(symv), cdr);
    return car;
}

/**
 * copy a list to a new list
 */
lv_t *lisp_copy_list(lv_t *v) {
    lv_t *vptr = v;
    lv_t *result = lisp_create_pair(NULL, NULL);
    lv_t *rptr = result;

    if(v->type == l_null)
        return v;

    assert(v->type == l_pair);

    while(vptr && L_CAR(vptr)) {
        L_CAR(rptr) = L_CAR(vptr);
        vptr = L_CDR(vptr);
        if(vptr) {
            L_CDR(rptr) = lisp_create_pair(NULL, NULL);
            rptr = L_CDR(rptr);
        }
    }

    return result;
}

/**
 * create a new execution context
 */
lexec_t *lisp_context_new(int scheme_revision) {
    lexec_t *ret = safe_malloc(sizeof(lexec_t));

    memset(ret, 0, sizeof(lexec_t));
    ret->env = c_env_version(scheme_revision);
    ret->ehandler = default_ehandler;

    maybe_initialize_gmp();

    return ret;
}

/**
 * push a value into a stack type
 */
void lisp_exec_push(lexec_t *exec, exec_stack_t which, void *value) {
    lstack_t *new_frame;
    void *ptr = (void*)exec + stack_offsets[which];

    new_frame = (lstack_t*)safe_malloc(sizeof(lstack_t));
    new_frame->data = value;

    new_frame->next = *(lstack_t**)ptr;
    *(lstack_t**)ptr = new_frame;
}

/**
 * and... pop a value from an exec stack type
 */
void *lisp_exec_pop(lexec_t *exec, exec_stack_t which) {
    void *ptr = (void*)exec + stack_offsets[which];
    void *retval;

    retval = (*(lstack_t**)ptr)->data;
    *(lstack_t**)ptr = (*(lstack_t**)ptr)->next;
    return retval;
}

/**
 * given an execution context, push the existing environment
 * on the environment stack, replacing the current environment
 * with the supplied one.
 */
void lisp_exec_push_env(lexec_t *exec, lv_t *env) {
    assert(exec);
    assert(env);

    lisp_exec_push(exec, es_env, exec->env);
    exec->env = env;
}

/**
 * given an execution context, pop the top env stack back
 * into the current env
 */
void lisp_exec_pop_env(lexec_t *exec) {
    lv_t *new_env_stack;

    assert(exec);
    assert(exec->env_stack);

    exec->env = lisp_exec_pop(exec, es_env);
}

/**
 * given an execution context, push an exception
 * handler on the exception stack
 */
void lisp_exec_push_ex(lexec_t *exec, jmp_buf *pjb) {
    assert(exec);
    assert(pjb);

    lisp_exec_push(exec, es_ex, pjb);
}

/**
 * pop an exception handlr
 */
void lisp_exec_pop_ex(lexec_t *exec) {
    assert(exec);
    assert(exec->ex_stack);

    lisp_exec_pop(exec, es_ex);
}

/**
 * push an evaluation stack item.  This could
 * (should?) be a native lisp type -- extract out
 * line, file, binding, etc, but it's way more convenient
 * to just shove the evaluated fn.  Meh.
 */
void lisp_exec_push_eval(lexec_t *exec, lv_t *ev) {
    assert(exec);
    assert(ev);

    lisp_exec_push(exec, es_eval, ev);
}

/**
 * and the matching pop
 */
void lisp_exec_pop_eval(lexec_t *exec) {
    assert(exec);
    assert(exec->eval_stack);

    lisp_exec_pop(exec, es_eval);
}

/**
 * reset an existing context for start of evaluation
 */
void lisp_context_reset(lexec_t *exec) {
    exec->ex_stack = NULL;
    exec->eval_stack = NULL;
    exec->exc = le_success;
    exec->msg = NULL;
}

/**
 * top level exec
 */
lv_t *lisp_execute(lexec_t *exec, lv_t *v) {
    jmp_buf jb;
    lv_t *result;

    lisp_context_reset(exec);

    lisp_exec_push_ex(exec, &jb);

    if(setjmp(jb) == 0) {
        result = c_sequential_eval(exec, v);
        assert(exec->eval_stack == NULL);
        return result;
    }

    if(exec->ehandler)
        exec->ehandler(exec);

    return NULL;
}
