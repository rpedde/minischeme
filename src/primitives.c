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
#include <search.h>
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

typedef struct hash_node_t {
    uint32_t key;
    lv_t *value;
} hash_node_t;

typedef struct environment_list_t {
    char *name;
    lv_t *(*fn)(lv_t *, lv_t *);
} environment_list_t;

static jmp_buf *assert_handler = NULL;
static int emit_on_error = 1;

static environment_list_t s_r5_list[] = {
    { "null?", nullp },
    { "symbol?", symbolp },
    { "atom?", atomp },
    { "cons?", consp },
    { "list?", listp },
    { "pair?", pairp },
    { "equal?", equalp },
    { "+", plus },
    { "null-environment", null_environment },
    { "scheme-report-environment", scheme_report_environment },
    { "eval", lisp_eval },
    { "map", lisp_map },
    { "apply", lisp_apply },
    { "set-cdr!", set_cdr },
    { "set-car!", set_car },
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

static void s_node_free(void *nodep) {
    /* do nothing... everything is gc'd */
}

static void s_hash_finalizer(void *v_obj, void *v_env) {
    lv_t *obj = (lv_t*)v_obj;

    tdestroy(L_HASH(obj), s_node_free);
}

static int s_hash_cmp(const void *a, const void *b) {
    hash_node_t *v1 = (hash_node_t*)a;
    hash_node_t *v2 = (hash_node_t*)b;

    if(v1->key > v2->key)
        return 1;
    if(v1->key < v2->key)
        return -1;
    return 0;
}

static int s_hash_item(lv_t *item) {
    if(item->type == l_str)
	return murmurhash2(L_STR(item), strlen(L_STR(item)), 0);
    if(item->type == l_sym)
	return murmurhash2(L_SYM(item), strlen(L_SYM(item)), 0);

    rt_assert(0, le_type, "unhashable data type");
}

lv_t *c_hash_fetch(lv_t *hash, lv_t *key) {
    hash_node_t node_key;
    void *result;

    rt_assert(hash->type == l_hash, le_type, "hash operation on non-hash");
    rt_assert(key->type == l_str || key->type == l_sym,
              le_type, "invalid hash key type");

    node_key.key = s_hash_item(key);

    result = tfind(&node_key, &L_HASH(hash), s_hash_cmp);
    if(!result)
        return NULL;

    return (*(hash_node_t **)result)->value;
}

int c_hash_insert(lv_t *hash,
                  lv_t *key,
                  lv_t *value) {

    hash_node_t *pnew;
    void *result;

    rt_assert(hash->type == l_hash, le_type, "hash operation on non-hash");
    rt_assert(key->type == l_str || key->type == l_sym,
              le_type, "invalid hash key type");

    pnew=safe_malloc(sizeof(hash_node_t));
    pnew->key = s_hash_item(key);
    pnew->value = value;

    result = tsearch(pnew, &L_HASH(hash), s_hash_cmp);
    if((*(hash_node_t **)result)->value != value)
        (*(hash_node_t **)result)->value = value;

    return(result != NULL);
}

int c_hash_delete(lv_t *hash, lv_t *key) {
    hash_node_t node_key;
    void *result;

    rt_assert(hash->type == l_hash, le_type, "hash operation on non-hash");
    rt_assert(key->type == l_str || key->type == l_sym,
              le_type, "invalid hash key type");

    node_key.key = s_hash_item(key);
    result = tdelete(&node_key, &L_HASH(hash), s_hash_cmp);
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
    L_HASH(result) = NULL;

    /* set up a finalizer */
    GC_register_finalizer(result, s_hash_finalizer, NULL, NULL, NULL);
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
lv_t *lisp_create_fn(lisp_method_t value) {
    return lisp_create_type(value, l_fn);
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

    default:
        // missing a type check.
        assert(0);
    }
}

/**
 * evaluate a lisp value
 */
lv_t *lisp_eval(lv_t *env, lv_t *v) {
    lv_t *fn;
    lv_t *args;
    lv_t *result;

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
            if(strcmp(L_SYM(L_CAR(v)), "quote") == 0)
		return lisp_quote(env, L_CDR(v));
            else if(strcmp(L_SYM(L_CAR(v)), "define") == 0) {
                rt_assert(c_list_length(L_CDR(v)) == 2, le_arity,
                          "define arity");
                return lisp_define(env, L_CADR(v), lisp_eval(env, L_CADDR(v)));
            }
	}

	/* test symbols */
	if(v->type == l_fn)
            fn = v;
	else if(L_CAR(v)->type == l_sym) {
            lv_t *tmp = c_env_lookup(env, L_CAR(v));
            rt_assert(tmp, le_lookup, "unknown function");
            rt_assert(tmp->type == l_fn, le_type, "eval a non-function");
            fn = tmp;
	}

        lv_t *eval_fn = lisp_create_fn(lisp_eval);

	/* execute the function */
        args = L_CDR(v);
        if(!args)
            args = lisp_create_null();

	return L_FN(fn)(env, lisp_map(env, c_make_list(eval_fn, args, NULL)));
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

    assert(v->type == l_pair);
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

    rt_assert(c_list_length(v) == 2, le_arity, "map arity");

    fn = L_CAR(v);
    list = L_CADR(v);

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
    list = L_CADR(v);

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
                      lisp_create_fn(current->fn));
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
 * given a string, return an ast
 */
lv_t *lisp_parse_string(char *string) {
    YY_BUFFER_STATE buffer;
    void *parser = ParseAlloc(safe_malloc);
    int yv;
    lv_t *result = NULL;
    void *scanner;

    yylex_init(&scanner);
    buffer = yy_scan_string(string, scanner);

    while((yv = yylex(scanner)) != 0) {
        Parse(parser, yv, yylval, &result);
    }
    yylex_destroy(scanner);
    Parse(parser, 0, yylval, &result);
    return result;
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
