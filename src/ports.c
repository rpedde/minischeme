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

#include "lisp-types.h"
#include "primitives.h"
#include "ports.h"

lv_t *p_open_file(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 2, le_arity, "expecting 2 arguments");

    lv_t *filename = L_CAR(v);
    lv_t *mode = L_CADR(v);

    rt_assert(filename->type == l_str, le_type, "filename requires string");
    rt_assert(mode->type == l_str, le_type, "mode requirest string");

    rt_assert(strlen(L_STR(mode)), le_syntax, "mode must be r, w, or a");
    rt_assert(*L_STR(mode) == 'r' ||
              *L_STR(mode) == 'w' ||
              *L_STR(mode) == 'a', le_syntax, "mode must be r, w, or a");

    /* meh.  rather than parsing out the 0 and l, we'll just
       pass it straight through to fopen */

    FILE *fp;
    fp = fopen(L_STR(filename), L_STR(mode));
    rt_assert(fp, le_system, strerror(errno));

    return lisp_create_port(fp, filename, mode);
}

lv_t *p_port_filename(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *port = L_CAR(v);

    rt_assert(port->type == l_port, le_type, "expecting port type");

    return L_P_FN(port);
}

lv_t *p_set_port_filename(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 2, le_arity, "expecting 2 arguments");

    lv_t *port = L_CAR(v);
    lv_t *filename = L_CADR(v);

    rt_assert(port->type == l_port, le_type, "expecting port type");
    rt_assert(filename->type == l_str, le_type, "expecting string filename");

    L_P_FN(port) = filename;
    return lisp_create_null();
}

lv_t *p_port_mode(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *port = L_CAR(v);

    rt_assert(port->type == l_port, le_type, "expecting port type");

    return L_P_MODE(port);
}

lv_t *p_file_port_p(lexec_t *exec, lv_t *v) {
    assert(exec);
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *port = L_CAR(v);

    if (port->type == l_port)
        return lisp_create_bool(1);
    return lisp_create_bool(0);
}
