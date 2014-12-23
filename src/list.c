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
#include "list.h"

/**
 * (append list ...)
 */
lv_t *p_append(lexec_t *exec, lv_t *v) {
    lv_t *r;
    lv_t *tptr, *vptr;

    assert(exec && v);
    assert((v->type == l_pair) || (v->type == l_null));

    rt_assert(c_list_length(v) > 1, le_arity, "expecting at least 1 arg");

    r = L_CAR(v);
    vptr = L_CDR(v);

    while(vptr) {
        r = lisp_dup_item(r);
        if(r->type == l_null)
            r = L_CAR(vptr);
        else {
            tptr = r;
            while(L_CDR(tptr))
                tptr = L_CDR(tptr);
            L_CDR(tptr) = L_CAR(vptr);
        }
        vptr = L_CDR(vptr);
    }

    return r;
}

/**
 * (list ...)
 */
lv_t *p_list(lexec_t *exec, lv_t *v) {
    assert(exec && v);
    assert((v->type == l_pair) || (v->type == l_null));

    if(v->type == l_null)
        return lisp_create_null();

    return lisp_dup_item(v);
}

/**
 * (reverse list)
 */
lv_t *p_reverse(lexec_t *exec, lv_t *v) {
    lv_t *r, *vptr;

    assert(exec && v);
    assert((v->type == l_pair) || (v->type == l_null));

    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 arg");

    vptr = L_CAR(v);
    if(vptr->type == l_null)
        return lisp_create_null();

    rt_assert(vptr->type == l_pair, le_type, "expecting list");

    r = NULL;
    while(vptr && L_CAR(vptr)) {
        r = lisp_create_pair(lisp_dup_item(L_CAR(vptr)), r);
        vptr = L_CDR(vptr);
        rt_assert(!vptr || (vptr->type == l_pair), le_type,
                  "expecting proper list");
    }

    return r;
}
