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

static lisp_type_t *s_is_type(lisp_value_t *v, lisp_type_t t) {
    if(v->type == t)
        return lisp_create_bool(1);
    return lisp_create_bool(0);
}

lisp_type_t *nullp(lisp_type_t *v) {
    if((v->type == l_pair) &&
       (L_CAR(v) == NULL) &&
       (L_CDR(v) == NULL))
        return lisp_create_bool(1);
    return lisp_create_bool(0);
}

lisp_type_t *symbolp(lisp_type_t *v) {
    return s_is_type(v, l_symbol);
}

lisp_type_t *atomp(lisp_type_t *v) {
    if(v->type != l_pair)
        return lisp_create_bool(1);
    return lisp_create_bool(0);
}

lisp_type_t *consp(lisp_type_t *v) {
    return s_is_type(v, l_pair);
}

lisp_type_t *listp(lisp_type_t *v) {
    if((v->type == l_pair) &&
       (L_CAR(v) == NULL) &&
       (L_CDR(v) == NULL))
        return lisp_creat_bool(1);
    return consp(lisp_type_t
}
