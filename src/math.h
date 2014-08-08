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

#ifndef _MATH_H_
#define _MATH_H_

extern lv_t *p_integerp(lexec_t *exec, lv_t *v);
extern lv_t *p_rationalp(lexec_t *exec, lv_t *v);
extern lv_t *p_floatp(lexec_t *exec, lv_t *v);

extern lv_t *p_exactp(lexec_t *exec, lv_t *v);
extern lv_t *p_inexactp(lexec_t *exec, lv_t *v);

extern lv_t *p_gt(lexec_t *exec, lv_t *v);
extern lv_t *p_lt(lexec_t *exec, lv_t *v);
extern lv_t *p_gte(lexec_t *exec, lv_t *v);
extern lv_t *p_lte(lexec_t *exec, lv_t *v);
extern lv_t *p_eq(lexec_t *exec, lv_t *v);

extern lv_t *p_plus(lexec_t *exec, lv_t *v);
extern lv_t *p_minus(lexec_t *exec, lv_t *v);
extern lv_t *p_mul(lexec_t *exec, lv_t *v);
extern lv_t *p_div(lexec_t *exec, lv_t *v);

extern lv_t *p_quotient(lexec_t *exec, lv_t *v);
extern lv_t *p_remainder(lexec_t *exec, lv_t *v);
extern lv_t *p_modulo(lexec_t *exec, lv_t *v);

extern lv_t *p_floor(lexec_t *exec, lv_t *v);
extern lv_t *p_ceiling(lexec_t *exec, lv_t *v);
extern lv_t *p_truncate(lexec_t *exec, lv_t *v);
extern lv_t *p_round(lexec_t *exec, lv_t *v);

extern lv_t *p_exp(lexec_t *exec, lv_t *v);
extern lv_t *p_log(lexec_t *exec, lv_t *v);
extern lv_t *p_sin(lexec_t *exec, lv_t *v);
extern lv_t *p_cos(lexec_t *exec, lv_t *v);
extern lv_t *p_tan(lexec_t *exec, lv_t *v);
extern lv_t *p_asin(lexec_t *exec, lv_t *v);
extern lv_t *p_acos(lexec_t *exec, lv_t *v);
extern lv_t *p_atan(lexec_t *exec, lv_t *v);
extern lv_t *p_sqrt(lexec_t *exec, lv_t *v);
extern lv_t *p_expt(lexec_t *exec, lv_t *v);

extern lv_t *p_number2string(lexec_t *exec, lv_t *v);
extern lv_t *p_string2number(lexec_t *exec, lv_t *v);

#endif /* _MATH_H_ */
