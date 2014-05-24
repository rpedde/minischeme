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

lisp_value_t *nullp(lisp_value_t *env, lisp_value_t *v);
lisp_value_t *symbolp(lisp_value_t *env, lisp_value_t *v);
lisp_value_t *atomp(lisp_value_t *env, lisp_value_t *v);
lisp_value_t *consp(lisp_value_t *env, lisp_value_t *v);
lisp_value_t *listp(lisp_value_t *env, lisp_value_t *v);

#endif /* __BUILTINS_H__ */
