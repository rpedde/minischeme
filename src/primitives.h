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
 * execution context, including execution stack
 * manipulation
 */
extern lexec_t *lisp_context_new(int scheme_revision);
extern void lisp_context_reset(lexec_t *exec);
extern void lisp_exec_push_env(lexec_t *exec, lv_t *env);   // environment
extern void lisp_exec_pop_env(lexec_t *exec);               // environment
extern void lisp_exec_push_ex(lexec_t *exec, jmp_buf *pjb); // longjmp/exception
extern void lisp_exec_pop_ex(lexec_t *exec);                // longjmp/exception
extern void lisp_exec_push_eval(lexec_t *exec, lv_t *ev);   // eval context
extern void lisp_exec_pop_eval(lexec_t *exec);              // eval context

extern lv_t *lisp_execute(lexec_t *exec, lv_t *v);

/**
 * utilities to build primitive types
 */
extern lv_t *lisp_create_type(void *value, lisp_type_t type);
extern lv_t *lisp_create_pair(lv_t *car, lv_t *cdr);
extern lv_t *lisp_create_string(char *value);
extern lv_t *lisp_create_symbol(char *value);
extern lv_t *lisp_create_int(int64_t value);
extern lv_t *lisp_create_int_str(char *value);
extern lv_t *lisp_create_rational(int64_t n, int64_t d);
extern lv_t *lisp_create_rational_str(char *value);
extern lv_t *lisp_create_float(double value);
extern lv_t *lisp_create_float_str(char *value);
extern lv_t *lisp_create_char(char value);
extern lv_t *lisp_create_bool(int value);
extern lv_t *lisp_create_hash(void);
extern lv_t *lisp_create_null(void);
extern lv_t *lisp_create_native_fn(lisp_method_t value);
extern lv_t *lisp_create_port(FILE *fp, lv_t *filename, lv_t *mode);
extern lv_t *lisp_create_lambda(lexec_t *exec, lv_t *formals, lv_t *body);
extern lv_t *lisp_create_macro(lexec_t *exec, lv_t *formals, lv_t *form);
extern lv_t *lisp_create_formatted_string(char *fmt, ...)
    __attribute__((format (printf, 1, 2)));
extern lv_t *lisp_wrap_type(char *symv, lv_t *v);

/**
 * misc utilities
 */
extern lv_t *lisp_parse_string(char *string);
extern lv_t *lisp_parse_file(char *file);
extern lv_t *lisp_exec_fn(lexec_t *exec, lv_t *fn, lv_t *args);
extern lv_t *lisp_begin(lexec_t *exec, lv_t *v);
extern void lisp_stamp_value(lv_t *v, int row, int col, char *file);
extern lv_t *lisp_copy_list(lv_t *v);
extern lv_t *lisp_args_overlay(lexec_t *exec, lv_t *formals, lv_t *args);

/**
 * hash utilities
 */
extern lv_t *c_hash_fetch(lv_t *hash, lv_t *key);
extern int c_hash_delete(lv_t *hash, lv_t *key);
extern int c_hash_insert(lv_t *hash, lv_t *key, lv_t *value);
extern lv_t *c_env_lookup(lv_t *env, lv_t *key);
extern void c_hash_walk(lv_t *hash, void(*callback)(lv_t *key, lv_t *value));

/**
 * inspection utilities
 */
extern void lisp_dump_value(int fd, lv_t *value, int level);
extern int c_list_length(lv_t *v);
extern lv_t *c_make_list(lv_t *item, ...);
extern lv_t *lisp_str_from_value(lv_t *v);
extern int lisp_snprintf(char *buf, int len, lv_t *v);

/**
 * actual language items
 */
extern lv_t *lisp_eval(lexec_t *exec, lv_t *v);
extern lv_t *lisp_map(lexec_t *exec, lv_t *v);
extern lv_t *lisp_apply(lexec_t *exec, lv_t *v);
extern lv_t *c_sequential_eval(lexec_t *exec, lv_t *v);


/**
 * special form helpers
 */
extern lv_t *lisp_quote(lexec_t *exec, lv_t *v);
extern lv_t *lisp_define(lexec_t *exec, lv_t *sym, lv_t *v);
extern lv_t *lisp_quasiquote(lexec_t *exec, lv_t *v);
extern lv_t *lisp_let(lexec_t *exec, lv_t *args, lv_t *expr);

/**
 * runtime asserts
 */
#define rt_assert(a, type, msg) { \
    if(!(a)) { \
        c_rt_assert(exec, type, msg);            \
    } \
}

extern void null_ehandler(lexec_t *exec);
extern void simple_ehandler(lexec_t *exec);
extern void default_ehandler(lexec_t *exec);
extern void lisp_set_ehandler(lexec_t *exec, void(*handler)(lexec_t *exec));

extern void c_rt_assert(lexec_t *exec, lisp_exception_t etype, char *msg);


//extern void c_set_top_context(jmp_buf *pjb);
//extern void c_set_emit_on_error(int v);

/**
 * environment stuff
 */
lv_t *null_environment(lexec_t *exec, lv_t *v);
lv_t *c_env_version(int version);

#endif /* __PRIMITIVES_H__ */
