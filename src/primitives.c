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

#include <gc.h>

#include "lisp-types.h"

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

lisp_value_t *lisp_create_pair(lisp_value_t *car, lisp_value_t *cdr) {
    lisp_value_t *result;

    result = safe_malloc(sizeof(lisp_value_t));

    result->type = l_pair;
    result->value.p.car = car;
    result->value.p.cdr = cdr;

    return result;
}

lisp_value_t *lisp_create_type(void *value, lisp_type_t type) {
    lisp_value_t *result;

    result = safe_malloc(sizeof(lisp_value_t));

    result->type = type;
    
    switch(type) {
    case l_int:
        result->value.i.value = *((int64_t*)value);
        break;
    case l_float:
        result->value.f.value = *((double*)value);
        break;
    case l_bool:
        result->value.b.value = *((int*)value);
        break;
    case l_symbol:
        result->value.s.value = safe_strdup((char*)value);
        break;
    case l_string:
        result->value.c.value = safe_strdup((char*)value);
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
    return lisp_create_type((void*)value, l_string);
}

/**
 * typechecked wrapper around lisp_create_type for symbols
 */
lisp_value_t *lisp_create_symbol(char *value) {
    return lisp_create_type((void*)value, l_symbol);
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
 * print a value to a fd, in a debug form
 */
void lisp_dump_value(int fd, lisp_value_t *value, int level) {
    switch(value->type) {
    case l_int:
        dprintf(fd, "%d", value->value.i.value);
        break;
    case l_float:
        dprintf(fd, "%0.16g", value->value.f.value);
        break;
    case l_bool:
        dprintf(fd, "%s", value->value.b.value ? "#t": "#f");
        break;
    case l_symbol:
        dprintf(fd, "%s", value->value.s.value);
        break;
    case l_string:
        dprintf(fd, "\"%s\"", value->value.c.value);
        break;
    case l_pair:
        dprintf(fd, "(");
        lisp_value_t *v = value;
        while(v && v->value.p.car) {
            lisp_dump_value(fd, v->value.p.car, level + 1);
            if(v->value.p.cdr && (v->value.p.cdr->type != l_pair)) {
                dprintf(fd, " . ");
                lisp_dump_value(fd, v->value.p.cdr, level + 1);
                v = NULL;
            } else {
                v = v->value.p.cdr;
                dprintf(fd, "%s", v ? " " : "");
            }
        }
        dprintf(fd, ")");
        break;
        
    default:
        // missing a type check.
        assert(0);
    }
}
