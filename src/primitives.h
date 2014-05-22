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
#include "lisp-types.h"

/**
 * gc malloc replacements
 */
extern void *safe_malloc(size_t size);
extern char *safe_strdup(char *str);

/**
 * utilities to build primitive types
 */
extern lisp_value_t *lisp_create_type(void *value, lisp_type_t type);
extern lisp_value_t *lisp_create_pair(lisp_value_t *car, lisp_value_t *cdr);
extern lisp_value_t *lisp_create_string(char *value);
extern lisp_value_t *lisp_create_symbol(char *value);
extern lisp_value_t *lisp_create_float(double value);
extern lisp_value_t *lisp_create_int(int64_t value);
extern lisp_value_t *lisp_create_bool(int value);
extern lisp_value_t *lisp_create_hash(void);

/**
 * hash utilities
 */
extern lisp_value_t *c_hash_fetch(lisp_value_t *hash, 
                                  lisp_value_t *key);
extern int c_hash_delete(lisp_value_t *hash, 
                         lisp_value_t *key);
extern int c_hash_insert(lisp_value_t *hash, 
                         lisp_value_t *key, 
                         lisp_value_t *value);


/**
 * inspection utilities
 */
extern void lisp_dump_value(int fd, lisp_value_t *value, int level);


/**
 * actual language items
 */
extern lisp_value_t *lisp_eval(lisp_value_t *v);

/**
 * runtime asserts
 */
#define rt_assert(a, msg) assert((a))

#endif /* __PRIMITIVES_H__ */
