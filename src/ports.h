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

#ifndef _PORTS_H_
#define _PORTS_H_

/* ports */
extern lv_t *p_input_portp(lexec_t *exec, lv_t *v);
extern lv_t *p_output_portp(lexec_t *exec, lv_t *v);
extern lv_t *p_current_input_port(lexec_t *exec, lv_t *v);
extern lv_t *p_current_output_port(lexec_t *exec, lv_t *v);
extern lv_t *p_with_input_from_file(lexec_t *exec, lv_t *v);
extern lv_t *p_with_output_to_file(lexec_t *exec, lv_t *v);
extern lv_t *p_open_input_file(lexec_t *exec, lv_t *v);
extern lv_t *p_open_output_file(lexec_t *exec, lv_t *v);
extern lv_t *p_close_input_port(lexec_t *exec, lv_t *v);
extern lv_t *p_close_output_port(lexec_t *exec, lv_t *v);

extern ssize_t c_read(lexec_t *exec, lv_t *port, char *buffer, size_t len);
extern ssize_t c_write(lexec_t *exec, lv_t *port, char *buffer, size_t len);


/* input */
extern lv_t *p_read_char(lexec_t *exec, lv_t *v);
extern lv_t *p_peek_char(lexec_t *exec, lv_t *v);

/* testing */
extern lv_t *p_toktest(lexec_t *exec, lv_t *v);

#endif /* _PORTS_H_ */
