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

#ifndef _CHAR_H_
#define _CHAR_H_

extern lv_t *p_charp(lexec_t *exec, lv_t *v);        // char?
extern lv_t *p_charequalp(lexec_t *exec, lv_t *v);   // char=?
extern lv_t *p_charltp(lexec_t *exec, lv_t *v);      // char<?
extern lv_t *p_chargtp(lexec_t *exec, lv_t *v);      // char>?
extern lv_t *p_charltep(lexec_t *exec, lv_t *v);     // char<=?
extern lv_t *p_chargtep(lexec_t *exec, lv_t *v);     // char>=?
extern lv_t *p_char_integer(lexec_t *exec, lv_t *v); // char->integer

#endif /* _CHAR_H_ */
