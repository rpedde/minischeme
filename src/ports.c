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
} port_info_t;

/**
 * open a file system file for either read or write (or both).
 *
 * there are unspecified cases around here regarding
 * O_CREAT, O_APPEND, and O_TRUNC.  Not sure how this
 * should be handled.
 */
lv_t *c_open_file(lexec_t *exec, lv_t *v, port_dir_t dir) {
    assert(exec);
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 2, le_arity, "expecting 1 argument");

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
lv_t *c_close_file(lexec_t *exec, lv_t *port) {
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
extern lv_t *p_input_portp(lexec_t *exec, lv_t *v) {
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
extern lv_t *p_output_portp(lexec_t *exec, lv_t *v) {
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
extern lv_t *p_current_input_port(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}

extern lv_t *p_current_output_port(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}

extern lv_t *p_with_input_from_file(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}

extern lv_t *p_with_output_to_file(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}
