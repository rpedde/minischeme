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
#include "char.h"

typedef enum char_comp_t { CC_EQ, CC_GT, CC_LT, CC_GTE, CC_LTE } char_comp_t;

lv_t *p_charp(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "wrong arity");
    lv_t *a0 = L_CAR(v);

    if(a0->type != l_char)
        return lisp_create_bool(0);
    return lisp_create_bool(1);
}

lv_t *c_charcomp(lexec_t *exec, lv_t *v, char_comp_t how) {
    int res;

    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 2, le_arity, "wrong arity");

    lv_t *a0 = L_CAR(v);
    lv_t *a1 = L_CADR(v);

    rt_assert(a0->type == l_char && a1->type == l_char, le_type,
              "expecting char types");

    switch(how) {
    case CC_EQ:
        res = (L_CHAR(a0) == L_CHAR(a1));
        break;
    case CC_LT:
        res = (L_CHAR(a0) < L_CHAR(a1));
        break;
    case CC_GT:
        res = (L_CHAR(a0) > L_CHAR(a1));
        break;
    case CC_LTE:
        res = (L_CHAR(a0) <= L_CHAR(a1));
        break;
    case CC_GTE:
        res = (L_CHAR(a0) >= L_CHAR(a1));
        break;
    default:
        assert(0);
    }

    return lisp_create_bool(res);
}


extern lv_t *p_charequalp(lexec_t *exec, lv_t *v) {
    return c_charcomp(exec, v, CC_EQ);
}

extern lv_t *p_charltp(lexec_t *exec, lv_t *v) {
    return c_charcomp(exec, v, CC_LT);
}

extern lv_t *p_chargtp(lexec_t *exec, lv_t *v) {
    return c_charcomp(exec, v, CC_GT);
}

extern lv_t *p_charltep(lexec_t *exec, lv_t *v) {
    return c_charcomp(exec, v, CC_LTE);
}

extern lv_t *p_chargtep(lexec_t *exec, lv_t *v) {
    return c_charcomp(exec, v, CC_GTE);
}

extern lv_t *p_char_integer(lexec_t *exec, lv_t *v) {
    assert(v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "wrong arity");

    lv_t *a0 = L_CAR(v);

    rt_assert(a0->type == l_char, le_type, "expecting char type");

    return lisp_create_int((int)(L_CHAR(a0)));
}
