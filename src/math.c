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
#include "math.h"

typedef enum math_comp_t { MC_EQ, MC_GT, MC_LT, MC_GTE, MC_LTE } math_comp_t;
typedef enum math_op_t { MO_ADD, MO_SUB, MO_MUL, MO_DIV } math_op_t;


static void math_promote(lv_t **a, lisp_type_t what) {
    lv_t *new_val;

    assert(a && *a);
    assert((*a)->type != what);

    switch((*a)->type) {
    case l_int:
        switch(what) {
        case l_rational:
            new_val = lisp_create_rational(1, 1);
            mpq_set_z(L_RAT(new_val), L_INT(*a));
            *a = new_val;
            break;
        case l_float:
            new_val = lisp_create_float(0);
            mpfr_set_z(L_FLOAT(new_val), L_INT(*a), MPFR_ROUND_TYPE);
            *a = new_val;
            break;
        default:
            assert(0);
        }
        break;
    case l_rational:
        switch(what) {
        case l_float:
            new_val = lisp_create_float(0);
            mpfr_set_q(L_FLOAT(new_val), L_RAT(*a), MPFR_ROUND_TYPE);
            *a = new_val;
            break;
        default:
            assert(0);
        }
        break;
    case l_float:
        assert(0);
    default:
        assert(0);
    }

    return;
}

static void math_maybe_promote(lv_t **a0, lv_t **a1) {
    assert(a0 && a1);
    assert(*a0 && *a1);

    if ((*a0)->type == (*a1)->type)
        return;

    if((*a0)->type > (*a1)->type) {
        math_promote(a1, (*a0)->type);
    } else {
        math_promote(a0, (*a1)->type);
    }
}

static int math_numeric(lv_t *v) {
    return (v->type == l_int ||
            v->type == l_rational ||
            v->type == l_float);

}

static lv_t *math_copy_value(lv_t *v) {
    lv_t *pnew;

    assert(v);

    switch(v->type) {
    case l_int:
        pnew = lisp_create_int(0);
        mpz_set(L_INT(pnew), L_INT(v));
        break;
    case l_rational:
        pnew = lisp_create_rational(0, 0);
        mpq_set(L_RAT(pnew), L_RAT(v));
        break;
    case l_float:
        pnew = lisp_create_float(0);
        mpfr_set(L_FLOAT(pnew), L_FLOAT(pnew), MPFR_ROUND_TYPE);
        break;
    default:
        assert(0);
    }

    return pnew;
}


/**
 * is the value in question an integer?
 */
lv_t *p_integerp(lexec_t *exec, lv_t *v) {
    assert(exec && v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *a0 = L_CAR(v);
    return lisp_create_bool(a0->type == l_int);
}

/**
 * is the value in question a rational?
 */
lv_t *p_rationalp(lexec_t *exec, lv_t *v) {
    assert(exec && v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *a0 = L_CAR(v);
    return lisp_create_bool(a0->type == l_rational);
}

/**
 * is the value in question a float?
 */
lv_t *p_floatp(lexec_t *exec, lv_t *v) {
    assert(exec && v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *a0 = L_CAR(v);
    return lisp_create_bool(a0->type == l_float);
}

/**
 * is the value in question exactly represented?
 */
lv_t *p_exactp(lexec_t *exec, lv_t *v) {
    int res = 1;

    assert(exec && v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *a0 = L_CAR(v);

    switch(a0->type) {
    case l_int:
    case l_rational:
        break;
    case l_float:
        res  = 0;
        break;
    default:
        assert(0);
    }

    return lisp_create_bool(res);
}

/**
 * is the value in question inexactly represented?
 */
lv_t *p_inexactp(lexec_t *exec, lv_t *v) {
    int res = 0;

    assert(exec && v && v->type == l_pair);
    rt_assert(c_list_length(v) == 1, le_arity, "expecting 1 argument");

    lv_t *a0 = L_CAR(v);

    switch(a0->type) {
    case l_int:
    case l_rational:
        break;
    case l_float:
        res = 1;
        break;
    default:
        assert(0);
    }

    return lisp_create_bool(res);
}

/**
 * compare two numeric types
 */
static lv_t *comp_op(lexec_t *exec, lv_t *v, math_comp_t op) {
    int result;

    assert(exec && v && v->type == l_pair);
    rt_assert(c_list_length(v) == 2, le_arity, "expecting 2 arguments");

    lv_t *a0 = L_CAR(v);
    lv_t *a1 = L_CADR(v);

    rt_assert(math_numeric(a0),
              le_type, "expecting numeric arguments");

    rt_assert(math_numeric(a1),
              le_type, "expecting numeric arguments");

    math_maybe_promote(&a0, &a1);
    assert(a0->type == a1->type);

    switch(a0->type) {
    case l_int:
        switch(op) {
        case MC_EQ:
            result = (mpz_cmp(L_INT(a0), L_INT(a1)) == 0);
            break;
        case MC_GT:
            result = (mpz_cmp(L_INT(a0), L_INT(a1)) > 0);
            break;
        case MC_LT:
            result = (mpz_cmp(L_INT(a0), L_INT(a1)) < 0);
            break;
        case MC_GTE:
            result = (mpz_cmp(L_INT(a0), L_INT(a1)) >= 0);
            break;
        case MC_LTE:
            result = (mpz_cmp(L_INT(a0), L_INT(a1)) <= 0);
            break;
        default:
            assert(0);
        }
        break;
    case l_rational:
        switch(op) {
        case MC_EQ:
            result = mpq_cmp(L_RAT(a0), L_RAT(a1)) == 0;
            break;
        case MC_GT:
            result = mpq_cmp(L_RAT(a0), L_RAT(a1)) > 0;
            break;
        case MC_LT:
            result = mpq_cmp(L_RAT(a0), L_RAT(a1)) < 0;
            break;
        case MC_GTE:
            result = mpq_cmp(L_RAT(a0), L_RAT(a1)) >= 0;
            break;
        case MC_LTE:
            result = mpq_cmp(L_RAT(a0), L_RAT(a1)) <= 0;
            break;
        default:
            assert(0);
        }
        break;
    case l_float:
        switch(op) {
        case MC_EQ:
            result = mpfr_cmp(L_FLOAT(a0), L_FLOAT(a1)) == 0;
            break;
        case MC_GT:
            result = mpfr_cmp(L_FLOAT(a0), L_FLOAT(a1)) > 0;
            break;
        case MC_LT:
            result = mpfr_cmp(L_FLOAT(a0), L_FLOAT(a1)) < 0;
            break;
        case MC_GTE:
            result = mpfr_cmp(L_FLOAT(a0), L_FLOAT(a1)) >= 0;
            break;
        case MC_LTE:
            result = mpfr_cmp(L_FLOAT(a0), L_FLOAT(a1)) <= 0;
            break;
        default:
            assert(0);
        }
        break;
    default:
        assert(0);
    }

    return lisp_create_bool(result);
}


lv_t *p_gt(lexec_t *exec, lv_t *v) {
    return comp_op(exec, v, MC_GT);
}

lv_t *p_lt(lexec_t *exec, lv_t *v) {
    return comp_op(exec, v, MC_LT);
}

lv_t *p_gte(lexec_t *exec, lv_t *v) {
    return comp_op(exec, v, MC_GTE);
}

lv_t *p_lte(lexec_t *exec, lv_t *v) {
    return comp_op(exec, v, MC_LTE);
}

lv_t *p_eq(lexec_t *exec, lv_t *v) {
    return comp_op(exec, v, MC_EQ);
}

/**
 * perform a rolling accumulated function
 */
static lv_t *accum_op(lexec_t *exec, lv_t *v, math_op_t op) {
    lv_t *a;
    lv_t *current;
    lv_t *arg;

    assert(exec);
    assert(v && (v->type == l_pair || v->type == l_null));

    if(op == MO_SUB || op == MO_DIV)
        rt_assert(c_list_length(v) >= 1, le_arity, "expecting more arguments");

    switch(op) {
    case MO_MUL:
        a = lisp_create_int(1);
        break;
    case MO_DIV:
        a = lisp_create_rational(1, 1);
        break;
    case MO_ADD:
    case MO_SUB:
        a = lisp_create_int(0);
        break;
    default:
        assert(0);
    }

    if(c_list_length(v) == 0)
        return a;

    current = v;

    if((op == MO_DIV || op == MO_SUB) && c_list_length(v) > 1) {
        /* seed accumulator with first item in list */
        rt_assert(math_numeric(L_CAR(current)), le_type, "expecting numeric");

        a = math_copy_value(L_CAR(current));
        current = L_CDR(current);

        assert(current);
        rt_assert(current->type == l_pair, le_type, "expecting proper list");
    }

    /* we have accumulator, walk through all the params */
    while(current && L_CAR(current)) {
        arg = L_CAR(current);

        rt_assert(math_numeric(arg), le_type, "expecting numeric");

        math_maybe_promote(&arg, &a);
        assert(arg->type == a->type);

        switch(arg->type) {
        case l_int:
            switch(op) {
            case MO_ADD:
                mpz_add(L_INT(a), L_INT(a), L_INT(arg));
                break;
            case MO_SUB:
                mpz_sub(L_INT(a), L_INT(a), L_INT(arg));
                break;
            case MO_MUL:
                mpz_mul(L_INT(a), L_INT(a), L_INT(arg));
                break;
            case MO_DIV:
                rt_assert(mpz_cmp_ui(L_INT(arg), 0) != 0, le_div,
                          "attempt to divide by zero");
                /* should we promote? */
                if(!mpz_divisible_p(L_INT(a), L_INT(arg))) {
                    /* yes! we must promote! */
                    math_promote(&a, l_rational);
                    math_promote(&arg, l_rational);
                    mpq_div(L_RAT(a), L_RAT(a), L_RAT(arg));
                } else {
                    mpz_tdiv_q(L_INT(a), L_INT(a), L_INT(arg));
                }
                break;
            default:
                assert(0);
            }
            break;
        case l_rational:
            switch(op) {
            case MO_ADD:
                mpq_add(L_RAT(a), L_RAT(a), L_RAT(arg));
                break;
            case MO_SUB:
                mpq_sub(L_RAT(a), L_RAT(a), L_RAT(arg));
                break;
            case MO_MUL:
                mpq_mul(L_RAT(a), L_RAT(a), L_RAT(arg));
                break;
            case MO_DIV:
                mpq_div(L_RAT(a), L_RAT(a), L_RAT(arg));
                break;
            default:
                assert(0);
            }
            break;
        case l_float:
            switch(op) {
            case MO_ADD:
                mpfr_add(L_FLOAT(a), L_FLOAT(a), L_FLOAT(arg), MPFR_ROUND_TYPE);
                break;
            case MO_SUB:
                mpfr_sub(L_FLOAT(a), L_FLOAT(a), L_FLOAT(arg), MPFR_ROUND_TYPE);
                break;
            case MO_MUL:
                mpfr_mul(L_FLOAT(a), L_FLOAT(a), L_FLOAT(arg), MPFR_ROUND_TYPE);
                break;
            case MO_DIV:
                mpfr_div(L_FLOAT(a), L_FLOAT(a), L_FLOAT(arg), MPFR_ROUND_TYPE);
                break;
            default:
                assert(0);
            }
            break;
        default:
            assert(0);
        }

        current = L_CDR(current);
        if(current)
            rt_assert(current->type == l_pair, le_type, "expecting proper list");
    }

    return a;
}


lv_t *p_plus(lexec_t *exec, lv_t *v) {
    return accum_op(exec, v, MO_ADD);
}

lv_t *p_minus(lexec_t *exec, lv_t *v) {
    return accum_op(exec, v, MO_SUB);
}

lv_t *p_mul(lexec_t *exec, lv_t *v) {
    return accum_op(exec, v, MO_MUL);
}

lv_t *p_div(lexec_t *exec, lv_t *v) {
    return accum_op(exec, v, MO_DIV);
}


lv_t *p_quotient(lexec_t *exec, lv_t *v) {
}

lv_t *p_remainder(lexec_t *exec, lv_t *v) {
}

lv_t *p_modulo(lexec_t *exec, lv_t *v) {
}


lv_t *p_floor(lexec_t *exec, lv_t *v) {
}

lv_t *p_ceiling(lexec_t *exec, lv_t *v) {
}

lv_t *p_truncate(lexec_t *exec, lv_t *v) {
}

lv_t *p_round(lexec_t *exec, lv_t *v) {
}


lv_t *p_exp(lexec_t *exec, lv_t *v) {
}

lv_t *p_log(lexec_t *exec, lv_t *v) {
}

lv_t *p_sin(lexec_t *exec, lv_t *v) {
}

lv_t *p_cos(lexec_t *exec, lv_t *v) {
}

lv_t *p_tan(lexec_t *exec, lv_t *v) {
}

lv_t *p_asin(lexec_t *exec, lv_t *v) {
}

lv_t *p_acos(lexec_t *exec, lv_t *v) {
}

lv_t *p_atan(lexec_t *exec, lv_t *v) {
}

lv_t *p_sqrt(lexec_t *exec, lv_t *v) {
}

lv_t *p_expt(lexec_t *exec, lv_t *v) {
}


lv_t *p_number2string(lexec_t *exec, lv_t *v) {
}

lv_t *p_string2number(lexec_t *exec, lv_t *v) {
}
