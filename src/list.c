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

/**
 * (list-tail list k)
 *
 * return the sublist of list obtained by omitting the first k elements
 */
lv_t *p_list_tail(lexec_t *exec, lv_t *v) {
    lv_t *r, *a0, *a1;
    int k;

    assert(exec && v);
    assert((v->type == l_pair) || (v->type == l_null));

    rt_assert(c_list_length(v) == 2, le_arity, "expecting 2 args");

    a0 = L_CAR(v);
    a1 = L_CADR(v);

    rt_assert(a0->type == l_pair, le_type, "expecting list as arg0");
    rt_assert(a1->type == l_int, le_type, "expecting int as arg1");

    k = mpz_get_si(L_INT(a1));

    r = lisp_get_kth(a0, k);

    rt_assert(r, le_type, "list too short");

    return r;
}


/**
 * (list-ref list k)
 *
 * returns the kth element of list (car (list-tail list k))
 */
lv_t *p_list_ref(lexec_t *exec, lv_t *v) {
    lv_t *r, *a0, *a1;
    int k;

    assert(exec && v);
    assert((v->type == l_pair) || (v->type == l_null));

    rt_assert(c_list_length(v) == 2, le_arity, "expecting 2 args");

    a0 = L_CAR(v);
    a1 = L_CADR(v);

    rt_assert(a0->type == l_pair, le_type, "expecting list as arg0");
    rt_assert(a1->type == l_int, le_type, "expecting int as arg1");

    k = mpz_get_si(L_INT(a1));

    r = lisp_get_kth(a0, k);

    rt_assert(r, le_type, "list too short");

    rt_assert(r->type == l_pair, le_type, "improper list");
    return L_CAR(r);
}
