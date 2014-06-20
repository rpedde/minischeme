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

#include <gc.h>

#include "lisp-types.h"
#include "primitives.h"
#include "builtins.h"
#include "murmurhash.h"
#include "grammar.h"
#include "tokenizer.h"
#include "redblack.h"

typedef struct hash_node_t {
    uint32_t key;
    lv_t *key_item;
    lv_t *value;
} hash_node_t;

typedef struct environment_list_t {
    char *name;
    lv_t *(*fn)(lv_t *, lv_t *);
} environment_list_t;

static jmp_buf *assert_handler = NULL;
static int emit_on_error = 1;

static environment_list_t s_r5_list[] = {
    { "null?", p_nullp },
    { "symbol?", p_symbolp },
    { "atom?", p_atomp },
    { "cons?", p_consp },
    { "list?", p_listp },
    { "pair?", p_pairp },
    { "equal?", p_equalp },
    { "+", p_plus },
    { "null-environment", null_environment },
    { "scheme-report-environment", scheme_report_environment },
    { "set-cdr!", p_set_cdr },
    { "set-car!", p_set_car },
    { "inspect", p_inspect },
    { "load", p_load },
    { "length", p_length },
    { "assert", p_assert },
    { "warn", p_warn },
    { "not", p_not },
    { "cons", p_cons },
    { "car", p_car },
    { "cdr", p_cdr },
    { "gensym", p_gensym },
    { "display", p_display },
    { NULL, NULL }
};

void c_set_top_context(jmp_buf *pjb) {
    assert_handler = pjb;
}

void c_set_emit_on_error(int v) {
    emit_on_error = v;
}

void c_rt_assert(lisp_exception_t etype, char *msg) {
    if(emit_on_error)
        fprintf(stderr, "Error: %s\n", msg);

    if(assert_handler) {
        longjmp(*assert_handler, (int)etype);
    } else {
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

    rt_assert(0, le_type, "unhashable data type");
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

    rt_assert(hash->type == l_hash, le_type, "hash operation on non-hash");
    rt_assert(key->type == l_str || key->type == l_sym,
              le_type, "invalid hash key type");

    pnew=safe_malloc(sizeof(hash_node_t));
    pnew->key = s_hash_item(key);
    pnew->key_item = key;
    pnew->value = value;

    result = (hash_node_t *)rbsearch((void*)pnew, L_HASH(hash));

    rt_assert(result, le_internal, "memory error");

    result->value = value;
    result->key_item = key;

    return(result != NULL);
}

int c_hash_delete(lv_t *hash, lv_t *key) {
    hash_node_t node_key;
    hash_node_t *result;

    rt_assert(hash->type == l_hash, le_type, "hash operation on non-hash");
    rt_assert(key->type == l_str || key->type == l_sym,
              le_type, "invalid hash key type");

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

    rt_assert(L_HASH(result), le_internal, "memory error");

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
    case l_int:
        L_INT(result) = *((int64_t*)value);
        break;
    case l_float:
        L_FLOAT(result) = *((double*)value);
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
    default:
        assert(0);
        fprintf(stderr, "Bad type");
        exit(EXIT_FAILURE);
    }

    return result;
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
 * typechecked wrapper around lisp_create_type for floats
 */
lv_t *lisp_create_float(double value) {
    return lisp_create_type((void*)&value, l_float);
}

/**
 * typechecked wrapper around lisp_create_type for ints
 */
lv_t *lisp_create_int(int64_t value) {
    return lisp_create_type((void*)&value, l_int);
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
 * defmacro
 */
lv_t *lisp_create_macro(lv_t *env, lv_t *formals, lv_t *form) {
    rt_assert(formals->type == l_pair || formals->type == l_null, le_type,
              "formals must be a list");

    lv_t *fn = lisp_create_type(NULL, l_fn);
    L_FN_FTYPE(fn) = lf_macro;
    L_FN_ENV(fn) = env;
    L_FN_ARGS(fn) = formals;
    L_FN_BODY(fn) = form;

    return fn;
}


/**
 * lambda-style function, not builtin
 */
lv_t *lisp_create_lambda(lv_t *env, lv_t *formals, lv_t *body) {
    rt_assert(formals->type == l_pair || formals->type == l_null, le_type,
              "formals must be a list");

    lv_t *fn = lisp_create_type(NULL, l_fn);
    L_FN_FTYPE(fn) = lf_lambda;
    L_FN_ENV(fn) = env;
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
        return snprintf(buf, len, "%" PRIu64, L_INT(v));
    case l_float:
        return snprintf(buf, len, "%0.16g", L_FLOAT(v));
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

lv_t *lisp_args_overlay(lv_t *formals, lv_t *args) {
    lv_t *pf, *pa;
    lv_t *env_layer;

    assert(formals->type == l_pair || formals->type == l_null);
    assert(args->type == l_pair || args->type == l_null);

    env_layer = lisp_create_hash();
    pf = formals;
    pa = args;

    rt_assert(formals->type == l_pair ||
              (formals->type == l_null && c_list_length(args) == 0),
              le_arity, "arity");

    while(pf && L_CAR(pf)) {
        if(L_CAR(pf)->type == l_sym &&
           !strcmp(L_SYM(L_CAR(pf)), "&rest")) {
            /* assign the rest to a list and done! */
            c_hash_insert(env_layer, L_CADR(pf), lisp_copy_list(pa));
            return env_layer;
        }
        rt_assert(pa && L_CAR(pa), le_arity, "arity");
        c_hash_insert(env_layer, L_CAR(pf), L_CAR(pa));
        pf = L_CDR(pf);
        pa = L_CDR(pa);
    }

    return env_layer;
}

lv_t *lisp_exec_fn(lv_t *env, lv_t *fn, lv_t *args) {
    lv_t *parsed_args;
    lv_t *layer, *newenv;
    lv_t *macrofn;

    assert(env && fn && args);
    rt_assert(fn->type == l_fn, le_type, "not a function");

    switch(L_FN_FTYPE(fn)) {
    case lf_native:
        return L_FN(fn)(env, args);
    case lf_lambda:
        layer = lisp_args_overlay(L_FN_ARGS(fn), args);
        newenv = lisp_create_pair(layer, L_FN_ENV(fn));
        return lisp_eval(newenv, L_FN_BODY(fn));
    case lf_macro:
        layer = lisp_args_overlay(L_FN_ARGS(fn), args);
        newenv = lisp_create_pair(layer, L_FN_ENV(fn));
        macrofn = lisp_eval(newenv, L_FN_BODY(fn));
        return lisp_eval(newenv, macrofn);
    default:
        assert(0);
    }

    return NULL;
}

/**
 * evaluate an expression under let
 */
lv_t *lisp_let(lv_t *env, lv_t *args, lv_t *expr) {
    lv_t *argp = args;
    lv_t *newenv;

    newenv = lisp_create_hash();

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
            c_hash_insert(newenv, L_CAAR(argp),
                          lisp_eval(env, L_CADAR(argp)));
            argp=L_CDR(argp);
        }
    }

    return lisp_eval(lisp_create_pair(newenv, env), expr);
}

lv_t *lisp_let_star(lv_t *env, lv_t *args, lv_t *expr) {
    lv_t *argp = args;
    lv_t *newenv;

    newenv = lisp_create_pair(lisp_create_hash(), env);

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
                          lisp_eval(newenv, L_CADAR(argp)));
            argp=L_CDR(argp);
        }
    }

    return lisp_eval(newenv, expr);
}

/**
 * quasiquote a term
 */
lv_t *lisp_quasiquote(lv_t *env, lv_t *v) {
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
            return lisp_eval(env, L_CADR(v));
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

                v2 = lisp_eval(env, L_CAR(L_CDAR(vptr)));
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
                L_CAR(rptr) = lisp_quasiquote(env, L_CAR(vptr));
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
lv_t *lisp_eval(lv_t *env, lv_t *v) {
    lv_t *fn;
    lv_t *args;
    lv_t *result;
    lv_t *a0, *a1, *a2, *a3;

    if(v->type == l_sym) {
        result = c_env_lookup(env, v);
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
		return lisp_quote(env, L_CDR(v));
            } else if(!strcmp(L_SYM(L_CAR(v)), "define")) {
                rt_assert(c_list_length(L_CDR(v)) == 2, le_arity,
                          "define arity");
                result = lisp_eval(env, L_CADDR(v));
                if(!result->bound)
                    result->bound = L_CADR(v);

                return lisp_define(env, L_CADR(v), result);
            } else if(!strcmp(L_SYM(L_CAR(v)), "lambda")) {
                rt_assert(c_list_length(L_CDR(v)) == 2, le_arity,
                          "lambda arity");
                result = lisp_create_lambda(env, L_CADR(v), L_CADDR(v));
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

                return lisp_define(env, a1, lisp_create_macro(env, a2, a3));
            } else if(!strcmp(L_SYM(L_CAR(v)), "begin")) {
                rt_assert(L_CADR(v), le_arity, "begin arity");
                return lisp_begin(env, L_CADR(v));
            } else if(!strcmp(L_SYM(L_CAR(v)), "quasiquote")) {
                rt_assert(
                    L_CDR(v)->type == l_null ||
                    (L_CDR(v)->type == l_pair && c_list_length(L_CDR(v)) == 1),
                    le_arity,
                    "quasiquote arity");
                return lisp_quasiquote(env, L_CADR(v));
            } else if(!strcmp(L_SYM(L_CAR(v)), "if")) {
                rt_assert(c_list_length(L_CDR(v)) == 3, le_arity,
                          "if arity");
                a1 = lisp_eval(env, L_CADR(v));  // expression
                a2 = L_CADDR(v);                 // value if true
                a3 = L_CADDDR(v);                // value if false

                if(a1->type == l_bool && L_BOOL(a1) == 0)
                    return lisp_eval(env, a3);
                return lisp_eval(env, a2);
            } else if(!strcmp(L_SYM(L_CAR(v)), "let")) {
                rt_assert(c_list_length(L_CDR(v)) == 2, le_arity,
                          "let arity");
                a1 = L_CADR(v);                  // tuple assignment list
                a2 = L_CADDR(v);                 // eval under let

                return lisp_let(env, a1, a2);
            } else if(!strcmp(L_SYM(L_CAR(v)), "let*")) {
                rt_assert(c_list_length(L_CDR(v)) == 2, le_arity,
                          "let arity");
                a1 = L_CADR(v);                  // tuple assignment list
                a2 = L_CADDR(v);                 // eval under let

                return lisp_let_star(env, a1, a2);
            }
	}

        /* otherwise, eval all the items, and execute */
        result = lisp_map(env, lisp_create_pair(lisp_create_native_fn(lisp_eval), v));

        /* make sure it's a function */
        fn = L_CAR(result);
        args = L_CDR(result);

        rt_assert(fn->type == l_fn, le_type, "eval a non-function");

        if(!args)
            args = lisp_create_null();

        /* and go. */
        return lisp_exec_fn(env, fn, args);

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
lv_t *lisp_quote(lv_t *env, lv_t *v) {
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
lv_t *lisp_map(lv_t *env, lv_t *v) {
    lv_t *vptr;
    lv_t *result = lisp_create_pair(NULL, NULL);
    lv_t *rptr = result;
    lv_t *fn, *list;

    fn = L_CAR(v);
    list = L_CDR(v);

    rt_assert(fn->type == l_fn, le_type, "map with non-function");
    rt_assert((list->type == l_pair) || (list->type == l_null),
              le_type, "map to non-list");

    if(list->type == l_null)
        return list;

    vptr = list;

    while(vptr) {
        L_CAR(rptr) = L_FN(fn)(env, L_CAR(vptr));
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
lv_t *lisp_apply(lv_t *env, lv_t *v) {
    lv_t *fn, *list;

    rt_assert(c_list_length(v) == 2, le_arity, "apply arity");

    fn = L_CAR(v);
    list = L_CDR(v);

    rt_assert(fn->type == l_fn, le_type, "apply with non-function");
    rt_assert(list->type == l_pair, le_type, "apply to non-list");

    /* here we would check arity, and do arg fixups
     * (&rest, etc) */

    return L_FN(fn)(env, list);
}

/**
 * hard to get to the rest of the functions without
 * having primitive access to environments, so...
 */

lv_t *null_environment(lv_t *env, lv_t *v) {
    lv_t *newenv = lisp_create_hash();
    return lisp_create_pair(newenv, NULL);
}

lv_t *scheme_report_environment(lv_t *env, lv_t *v) {
    environment_list_t *current = s_r5_list;
    lv_t *newenv = lisp_create_hash();

    while(current && current->name) {
        c_hash_insert(newenv, lisp_create_string(current->name),
                      lisp_create_native_fn(current->fn));
        current++;
    }

    return lisp_create_pair(newenv, NULL);
}

/**
 * promote a numeric to a higher-order numeric
 */
lv_t *numeric_promote(lv_t *v, lisp_type_t type) {
    /* only valid promotion right now */
    assert(v->type == l_int && type == l_float);

    return lisp_create_float((float)L_INT(v));
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
        c_rt_assert(le_syntax, "syntax error");
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
        c_rt_assert(le_syntax, "syntax error");
    }
    return lst.result;
}

lv_t *lisp_define(lv_t *env, lv_t *sym, lv_t *v) {
    /* this is probably not a good or completely safe
     * check of an environment */
    rt_assert(env->type == l_pair &&
              L_CAR(env) &&
              L_CAR(env)->type == l_hash, le_type,
              "Not a valid environment");

    rt_assert(sym->type == l_sym, le_type, "cannot define non-symbol");

    rt_assert(c_hash_insert(L_CAR(env), sym, v), le_internal,
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

    rt_assert(env->type == l_pair &&
              L_CAR(env) &&
              L_CAR(env)->type == l_hash, le_type,
              "Not a valid environment");

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
lv_t *lisp_begin(lv_t *env, lv_t *v) {
    lv_t *current;
    lv_t *retval;

    rt_assert(v->type == l_pair, le_type, "cannot begin non-list");

    current = v;
    while(v && (L_CAR(v))) {
        retval = lisp_eval(env, L_CAR(v));
        v = L_CDR(v);
    }

    return retval;
}

/**
 * eval a list of items, one after the other, returning the
 * value of the last eval
 */
lv_t *c_sequential_eval(lv_t *env, lv_t *v) {
    lv_t *current = v;
    lv_t *result;

    assert(v->type == l_pair || v->type == l_null);

    if(v->type == l_null)
        return v;

    while(current && L_CAR(current)) {
        result = lisp_eval(env, L_CAR(current));
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
