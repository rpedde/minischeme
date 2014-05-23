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

#include <gc.h>

#include "lisp-types.h"
#include "primitives.h"
#include "builtins.h"
#include "murmurhash.h"

typedef struct hash_node_t {
    uint32_t key;
    lisp_value_t *value;
} hash_node_t;

typedef struct environment_list_t {
    char *name;
    lisp_value_t *(*fn)(lisp_value_t *);
} environment_list_t;

static environment_list_t s_r5_list[] = {
    { "null?", nullp },
    { "symbol?", symbolp },
    { "atom?", atomp },
    { "cons?", consp },
    { "list?", listp },
    { "null-environment", null_environment },
    { "scheme-report-environment", scheme_report_environment },
    { NULL, NULL }
};

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
    lisp_value_t *obj = (lisp_value_t*)v_obj;

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

lisp_value_t *c_hash_fetch(lisp_value_t *hash, lisp_value_t *key) {
    hash_node_t node_key;
    void *result;

    rt_assert(hash->type == l_hash, "hash operation on non-hash");
    rt_assert(key->type == l_str, "invalid hash key type");
    
    node_key.key = murmurhash2(L_STR(key), strlen(L_STR(key)), 0);
    result = tfind(&node_key, &L_HASH(hash), s_hash_cmp);
    if(!result)
        return NULL;

    return (*(hash_node_t **)result)->value;
}

int c_hash_insert(lisp_value_t *hash, 
                  lisp_value_t *key, 
                  lisp_value_t *value) {
    
    hash_node_t *pnew;
    void *result;

    rt_assert(hash->type == l_hash, "hash operation on non-hash");
    rt_assert(key->type == l_str, "invalid hash key type");

    pnew=safe_malloc(sizeof(hash_node_t));
    pnew->key = murmurhash2(L_STR(key), strlen(L_STR(key)), 0);
    pnew->value = value;

    result = tsearch(pnew, &L_HASH(hash), s_hash_cmp);
    return(result != NULL);
}

int c_hash_delete(lisp_value_t *hash, lisp_value_t *key) {
    hash_node_t node_key;
    void *result;

    rt_assert(hash->type == l_hash, "hash operation on non-hash");
    rt_assert(key->type == l_str, "invalid hash key type");
    
    node_key.key = murmurhash2(L_STR(key), strlen(L_STR(key)), 0);
    result = tdelete(&node_key, &L_HASH(hash), s_hash_cmp);
    return(result != NULL);
}

lisp_value_t *lisp_create_hash(void) {
    lisp_value_t *result;

    result = safe_malloc(sizeof(lisp_value_t));
    result->type = l_hash;
    L_HASH(result) = NULL;

    /* set up a finalizer */
    GC_register_finalizer(result, s_hash_finalizer, NULL, NULL, NULL);
    return result;
}

lisp_value_t *lisp_create_pair(lisp_value_t *car, lisp_value_t *cdr) {
    lisp_value_t *result;

    result = safe_malloc(sizeof(lisp_value_t));

    result->type = l_pair;
    L_CAR(result) = car;
    L_CDR(result) = cdr;

    return result;
}

lisp_value_t *lisp_create_type(void *value, lisp_type_t type) {
    lisp_value_t *result;

    result = safe_malloc(sizeof(lisp_value_t));

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
        L_FN(result) = (lisp_value_t *(*)(lisp_value_t *))value;
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
lisp_value_t *lisp_create_string(char *value) {
    return lisp_create_type((void*)value, l_str);
}

/**
 * typechecked wrapper around lisp_create_type for symbols
 */
lisp_value_t *lisp_create_symbol(char *value) {
    return lisp_create_type((void*)value, l_sym);
}

/**
 * typechecked wrapper around lisp_create_type for floats
 */
lisp_value_t *lisp_create_float(double value) {
    return lisp_create_type((void*)&value, l_float);
}

/**
 * typechecked wrapper around lisp_create_type for ints
 */
lisp_value_t *lisp_create_int(int64_t value) {
    return lisp_create_type((void*)&value, l_int);
}

/**
 * typechecked wrapper around lisp_create_type for bools
 */
lisp_value_t *lisp_create_bool(int value) {
    return lisp_create_type((void*)&value, l_bool);
}

/**
 * typechecked wrapper around lisp_create_type for functions
 */
lisp_value_t *lisp_create_fn(lisp_value_t *(*value)(lisp_value_t*)) {
    return lisp_create_type((void*)&value, l_fn);
}

/**
 * print a value to a fd, in a debug form
 */
void lisp_dump_value(int fd, lisp_value_t *v, int level) {
    switch(v->type) {
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
        lisp_value_t *vp = v;
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
lisp_value_t *lisp_eval(lisp_value_t *v) {
    if(v->type != l_pair) {  // atom?
        return v;
    }

    // list?
    return v;
}

/**
 * hard to get to the rest of the functions without
 * having primitive access to environments, so...
 */
lisp_value_t *null_environment(lisp_value_t *v) {
    lisp_value_t *env = lisp_create_hash();
    return env;
}

lisp_value_t *scheme_report_environment(lisp_value_t *v) {
    environment_list_t *current = s_r5_list;
    lisp_value_t *env = null_environment(v);

    while(current && current->name) {
        c_hash_insert(env, lisp_create_string(current->name),
                      lisp_create_fn(current->fn));
        current++;
    }

    return env;
}

