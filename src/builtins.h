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

#ifndef __BUILTINS_H__
#define __BUILTINS_H__

extern lv_t *p_nullp(lexec_t *exec, lv_t *v);
extern lv_t *p_symbolp(lexec_t *exec, lv_t *v);
extern lv_t *p_charp(lexec_t *exec, lv_t * v);
extern lv_t *p_atomp(lexec_t *exec, lv_t *v);
extern lv_t *p_consp(lexec_t *exec, lv_t *v);
extern lv_t *p_listp(lexec_t *exec, lv_t *v);
extern lv_t *p_pairp(lexec_t *exec, lv_t *v);
extern lv_t *p_plus(lexec_t *exec, lv_t *v);
extern lv_t *p_equalp(lexec_t *exec, lv_t *v);
extern lv_t *p_set_cdr(lexec_t *exec, lv_t *v);
extern lv_t *p_set_car(lexec_t *exec, lv_t *v);
extern lv_t *p_inspect(lexec_t *exec, lv_t *v);
extern lv_t *p_load(lexec_t *exec, lv_t *v);
extern lv_t *p_length(lexec_t *exec, lv_t *v);
extern lv_t *p_assert(lexec_t *exec, lv_t *v);
extern lv_t *p_warn(lexec_t *exec, lv_t *v);
extern lv_t *p_not(lexec_t *exec, lv_t *v);
extern lv_t *p_car(lexec_t *exec, lv_t *v);
extern lv_t *p_cdr(lexec_t *exec, lv_t *v);
extern lv_t *p_cons(lexec_t *exec, lv_t *v);
extern lv_t *p_gensym(lexec_t *exec, lv_t *v);
extern lv_t *p_display(lexec_t *exec, lv_t *v);
extern lv_t *p_format(lexec_t *exec, lv_t *v);

#endif /* __BUILTINS_H__ */
