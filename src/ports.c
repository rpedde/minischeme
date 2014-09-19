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

#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#define MIN(a, b) ((a) > (b)) ? (b) : (a)

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
    int eof;
    int peek;
    int peeked_char;
} port_info_t;

/* Forwards */
ssize_t c_read_fd(lexec_t *exec, int fd, char *buffer, size_t len);
ssize_t c_write_fd(lexec_t *exec, int fd, char *buffer, size_t len);
ssize_t c_read(lexec_t *exec, lv_t *port, char *buffer, size_t len);
ssize_t c_write(lexec_t *exec, lv_t *port, char *buffer, size_t len);

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
    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

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
 * open a string for either read or write (or both).
 */
lv_t *c_open_string(lexec_t *exec, lv_t *v, port_dir_t dir) {
    assert(exec);
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *str = L_CAR(v);
    rt_assert(str->type == l_str, le_type, "expecting string");

    port_info_t *pi = (port_info_t *)safe_malloc(sizeof(port_info_t));
    memset(pi, 0, sizeof(port_info_t));

    pi->type = PT_STRING;
    pi->dir = dir;

    pi->info.si.buffer = L_STR(str);
    pi->info.si.pos = 0;

    return lisp_create_port(pi);
}

lv_t *p_open_input_string(lexec_t *exec, lv_t *v) {
    return c_open_string(exec, v, PD_INPUT);
}

/**
 * c helper for p_close_input_port and p_close_output_port
 */
static lv_t *c_close_file(lexec_t *exec, lv_t *port) {
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

/**
 * read handler for bare unbuffered fds
 */
ssize_t c_read_fd(lexec_t *exec, int fd, char *buffer, size_t len) {
    ssize_t bytes_read;

    bytes_read = read(fd, buffer, len);
    rt_assert(bytes_read != -1, le_system, strerror(errno));

    return bytes_read;
}

/**
 * write handler for bare unbuffered fds
 */
ssize_t c_write_fd(lexec_t *exec, int fd, char *buffer, size_t len) {
    ssize_t bytes_written;

    bytes_written = write(fd, buffer, len);
    rt_assert(bytes_written != -1, le_system, strerror(errno));

    /* is bytes_written != len an error?  If not, how do we pass
       it back to the interpreter? */

    return bytes_written;
}

port_dir_t c_port_direction(lexec_t *exec, lv_t *port) {
    assert(exec && port);
    assert(port->type == l_port);

    return(L_PORT(port)->dir);
}

int c_port_eof(lexec_t *exec, lv_t *port) {
    assert(exec && port);
    assert(port->type == l_port);

    return(L_PORT(port)->eof);
}

/**
 * c dispatch function for reading an arbitrary port type
 */
ssize_t c_port_read(lexec_t *exec, lv_t *port, char *buffer, size_t len) {
    ssize_t res;
    size_t pos;
    char *sbuff;
    size_t actual_len;

    assert(exec && port);
    assert(port->type == l_port);

    switch(L_PORT(port)->type) {
    case PT_FILE:
        /* will assert in c_read_fd on error */
        res = c_read_fd(exec, L_PORT(port)->info.fi.fd, buffer, len);
        if(!res)
            L_PORT(port)->eof = 1;
        break;
    case PT_STRING:
        pos = L_PORT(port)->info.si.pos;
        sbuff = L_PORT(port)->info.si.buffer;
        actual_len = MIN(strlen(&sbuff[pos]), len);

        memcpy(buffer, &sbuff[pos], actual_len);
        L_PORT(port)->info.si.pos += actual_len;
        L_PORT(port)->eof = !strlen(&sbuff[L_PORT(port)->info.si.pos]);
        res = actual_len;
        break;
    default:
        /* unhandled port type */
        assert(0);
    }

    return res;
}

/**
 * c dispatch function for writing an arbitrary port type
 */
ssize_t c_write(lexec_t *exec, lv_t *port, char *buffer, size_t len) {
    assert(exec && port);
    assert(port->type == l_port);

    switch(L_PORT(port)->type) {
    case PT_FILE:
        return c_write_fd(exec, L_PORT(port)->info.fi.fd, buffer, len);
        break;
    default:
        /* unhandled port type */
        assert(0);
    }
}

/**
 * c_read_char - return a character, or -1 on eof,
 * asserting on any read errors
 */
int c_read_char(lexec_t *exec, lv_t *port) {
    int result;
    char data;

    assert(exec && port && port->type == l_port);

    if(L_PORT(port)->peek) {
        L_PORT(port)->peek = 0;
        return L_PORT(port)->peeked_char;
    }

    result = c_port_read(exec, port, &data, 1);
    if(result == 1)
        return data;

    return -1;
}

/**
 * c_peek_char - returns a character, or -1 on eof of
 * the next char to be read
 */
int c_peek_char(lexec_t *exec, lv_t *port) {
    int result;
    char data;

    assert(exec && port && port->type == l_port);
    if(L_PORT(port)->peek)
        return L_PORT(port)->peeked_char;

    /* otherwise, advance the read */
    L_PORT(port)->peek = 0;
    L_PORT(port)->peeked_char = c_read_char(exec, port);
    L_PORT(port)->peek = 1;

    return L_PORT(port)->peeked_char;
}


/**
 * (read-char port)
 */
lv_t *p_read_char(lexec_t *exec, lv_t *v) {
    int result;

    assert(exec);
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *port = L_CAR(v);
    rt_assert(port->type == l_port && L_PORT(port)->dir == PD_INPUT,
              le_type, "expecting input port");

    result = c_read_char(exec, port);

    if(result == -1)
        return lisp_create_err(les_eof);  /* eof */

    return lisp_create_char(result);
}

/**
 * (peek-char port)
 */
lv_t *p_peek_char(lexec_t *exec, lv_t *v) {
    int result;

    assert(exec);
    assert(v && v->type == l_pair);

    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *port = L_CAR(v);
    rt_assert(port->type == l_port && L_PORT(port)->dir == PD_INPUT,
              le_type, "expecting input port");

    result = c_peek_char(exec, port);

    if(result == -1)
        return lisp_create_err(les_eof);  /* eof */
    return lisp_create_char(result);
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
lv_t *p_input_portp(lexec_t *exec, lv_t *v) {
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
lv_t *p_output_portp(lexec_t *exec, lv_t *v) {
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
lv_t *p_current_input_port(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}

lv_t *p_current_output_port(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}

lv_t *p_with_input_from_file(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}

lv_t *p_with_output_to_file(lexec_t *exec, lv_t *v) {
    return lisp_create_null();
}
