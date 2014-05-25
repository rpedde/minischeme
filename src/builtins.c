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

#include "lisp-types.h"
#include "primitives.h"
#include "builtins.h"

static lv_t *s_is_type(lv_t *v, lisp_type_t t) {
    if(v->type == t)
        return lisp_create_bool(1);
    return lisp_create_bool(0);
}

lv_t *nullp(lv_t *env, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, "wrong arity");
    lv_t *a0 = L_CAR(v);

    return s_is_type(a0, l_null);
}

lv_t *symbolp(lv_t *env, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, "wrong arity");
    lv_t *a0 = L_CAR(v);

    return s_is_type(a0, l_sym);
}

lv_t *atomp(lv_t *env, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, "wrong arity");
    lv_t *a0 = L_CAR(v);

    if(a0->type != l_pair)
        return lisp_create_bool(1);
    return lisp_create_bool(0);
}

lv_t *consp(lv_t *env, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, "wrong arity");
    lv_t *a0 = L_CAR(v);

    return s_is_type(a0, l_pair);
}

lv_t *listp(lv_t *env, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, "wrong arity");
    lv_t *a0 = L_CAR(v);

    if((a0->type == l_pair) || (a0->type == l_null))
        return lisp_create_bool(1);
    return lisp_create_bool(0);
}
