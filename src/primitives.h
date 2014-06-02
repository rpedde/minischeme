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

#ifndef __PRIMITIVES_H__
#define __PRIMITIVES_H__

#include <stdint.h>
#include <setjmp.h>

#include "lisp-types.h"

/**
 * gc malloc replacements
 */
extern void *safe_malloc(size_t size);
extern char *safe_strdup(char *str);

/**
 * utilities to build primitive types
 */
extern lv_t *lisp_create_type(void *value, lisp_type_t type);
extern lv_t *lisp_create_pair(lv_t *car, lv_t *cdr);
extern lv_t *lisp_create_string(char *value);
extern lv_t *lisp_create_symbol(char *value);
extern lv_t *lisp_create_float(double value);
extern lv_t *lisp_create_int(int64_t value);
extern lv_t *lisp_create_bool(int value);
extern lv_t *lisp_create_hash(void);
extern lv_t *lisp_create_null(void);
extern lv_t *lisp_create_native_fn(lisp_method_t value);
extern lv_t *lisp_create_lambda(lv_t *env, lv_t *formals, lv_t *body);

/**
 * misc utilities
 */
extern lv_t *lisp_parse_string(char *string);
extern lv_t *lisp_exec_fn(lv_t *env, lv_t *fn, lv_t *args);
extern lv_t *lisp_begin(lv_t *env, lv_t *v);

/**
 * hash utilities
 */
extern lv_t *c_hash_fetch(lv_t *hash, lv_t *key);
extern int c_hash_delete(lv_t *hash, lv_t *key);
extern int c_hash_insert(lv_t *hash, lv_t *key, lv_t *value);
extern lv_t *c_env_lookup(lv_t *env, lv_t *key);

/**
 * inspection utilities
 */
extern void lisp_dump_value(int fd, lv_t *value, int level);
extern int c_list_length(lv_t *v);
extern lv_t *c_make_list(lv_t *item, ...);

/**
 * actual language items
 */
extern lv_t *lisp_eval(lv_t *env, lv_t *v);
extern lv_t *lisp_map(lv_t *env, lv_t *v);
extern lv_t *lisp_apply(lv_t *env, lv_t *v);

/**
 * special forms
 */
extern lv_t *lisp_quote(lv_t *env, lv_t *v);
extern lv_t *lisp_define(lv_t *env, lv_t *sym, lv_t *v);

/**
 * runtime asserts
 */
#define rt_assert(a, type, msg) { \
    if(!(a)) { \
        c_rt_assert(type, msg); \
    } \
}

extern void c_rt_assert(lisp_exception_t etype, char *msg);
extern void c_set_top_context(jmp_buf *pjb);
extern void c_set_emit_on_error(int v);

/**
 * environment stuff
 */
lv_t *null_environment(lv_t *env, lv_t *v);
lv_t *scheme_report_environment(lv_t *env, lv_t *v);

#endif /* __PRIMITIVES_H__ */
