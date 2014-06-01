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

lv_t *nullp(lv_t *env, lv_t *v);
lv_t *symbolp(lv_t *env, lv_t *v);
lv_t *atomp(lv_t *env, lv_t *v);
lv_t *consp(lv_t *env, lv_t *v);
lv_t *listp(lv_t *env, lv_t *v);
lv_t *pairp(lv_t *env, lv_t *v);
lv_t *plus(lv_t *env, lv_t *v);
lv_t *equalp(lv_t *env, lv_t *v);
lv_t *set_cdr(lv_t *env, lv_t *v);
lv_t *set_car(lv_t *env, lv_t *v);

#endif /* __BUILTINS_H__ */
