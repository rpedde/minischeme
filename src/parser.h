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

#ifndef _PARSER_H_
#define _PARSER_H_

/* testing */
extern lv_t *p_toktest(lexec_t *exec, lv_t *v);
extern lv_t *p_parsetest(lexec_t *exec, lv_t *v);
extern lv_t *p_read(lexec_t *exec, lv_t *v);

/* FIXME: this shouldn't be here */
extern lv_t *c_parse(lexec_t *exec, lv_t *port);
extern lv_t *c_parse_string(lexec_t *exec, char *str);

#endif /* _PARSER_H_ */
