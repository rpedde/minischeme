#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lisp-types.h"
#include "primitives.h"
#include "builtins.h"
#include "selfcheck.h"

int test_hash_functions(void *scaffold) {
    lv_t *hash = lisp_create_hash();
    lv_t *key1 = lisp_create_string("key1");
    lv_t *key2 = lisp_create_string("key2");
    lv_t *result;

    /* insert key1 */
    assert(c_hash_insert(hash, key1, key1));
    result = c_hash_fetch(hash, key1);

    /* make sure it exists and is correct */
    assert(result);
    assert(result == key1);

    /* make sure key2 isn't in there */
    assert(!c_hash_fetch(hash, key2));

    /* poke in key2 */
    assert(c_hash_insert(hash, key2, key2));

    /* make sure it's there */
    result = c_hash_fetch(hash, key2);
    assert(result != NULL);
    assert(result == key2);

    /* now delete key1 */
    assert(c_hash_delete(hash, key1));

    /* and make sure it's gone */
    assert(!c_hash_fetch(hash, key1));

    return 1;
}

int test_environment(void *scaffold) {
    lv_t *fn;
    lv_t *fn2;

    /* get an env */
    lv_t *env = scheme_report_environment(NULL, NULL);

    /* make sure it's generally working */
    assert(c_env_lookup(env, lisp_create_symbol("asdlfjkasf")) == NULL);

    /* check string and symbol interop */
    fn = c_env_lookup(env, lisp_create_string("null?"));
    assert(fn);

    fn2 = c_env_lookup(env, lisp_create_symbol("null?"));
    assert(fn2);

    assert(fn == fn2);

    /* make sure it actually points to the right thing */
    assert(L_FN(fn) == nullp);

    return 1;
}
