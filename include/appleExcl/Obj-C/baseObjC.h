#ifndef BASE_OBJC_H
#define BASE_OBJC_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__OBJC__) || defined(__BLOCKS__)
    typedef void (^os_block_t)(void);
#else
    typedef void* os_block_t;
#endif

#ifdef __cplusplus
}
#endif

#endif /* BASE_OBJC_H */
