/*
 *
 */

#ifndef __LISP_TYPES_H__
#define __LISP_TYPES_H__

typedef enum lisp_type_t { l_int, l_float, l_bool, l_symbol, l_string, l_pair } lisp_type_t;

struct lisp_value_t;

typedef struct lisp_int_t {
    int64_t value;
} lisp_int_t;

typedef struct lisp_float_t {
    double value;
} lisp_float_t;

typedef struct lisp_bool_t {
    int value;
} lisp_bool_t;

typedef struct lisp_symbol_t {
    char *value;
} lisp_symbol_t;

typedef struct lisp_string_t {
    char *value;
} lisp_string_t;

typedef struct lisp_pair_t {
    struct lisp_value_t *car;
    struct lisp_value_t *cdr;
} lisp_pair_t;

typedef struct lisp_value_t {
    lisp_type_t type;
    union {
        lisp_int_t i;
        lisp_float_t f;
        lisp_bool_t b;
        lisp_symbol_t s;
        lisp_string_t c;
        lisp_pair_t p;
    } value;
} lisp_value_t;

/** given a native c type, box it into a lisp type struct */
extern lisp_value_t *lisp_create_type(void *value, lisp_type_t type);

/** given a string, parse it and return the parsed lisp_value */
extern lisp_value_t *lisp_parse_string(char *string);

/** helper for reading parser input from strings */
extern int parser_read_input(char *buffer, int *read, int max);

#endif /* __LISP_TYPES_H__ */
