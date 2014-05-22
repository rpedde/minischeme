#ifndef __SELFCHECK_H__
#define __SELFCHECK_H__

#define assert(a) { \
    if(!(a)) { \
        enqueue_error(__STRING((a)), __FILE__, __LINE__);       \
        return 0; \
    } \
}

#endif /* __SELFCHECK_H__ */
