#ifndef CACHE_H
#define CACHE_H

#include <stdint.h> 

#ifdef __cplusplus
extern "C" {
#endif

uint64_t get_cache_tag_generic(uint64_t addr);
uint64_t get_cache_set_generic(uint64_t addr);
uint64_t get_cache_offset_generic(uint64_t addr);
uint64_t get_cache_set_m1(uint64_t addr);
uint64_t get_l1_cache_set_m1(uint64_t addr);
uint64_t get_cache_offset_m1(uint64_t addr);

#ifdef __cplusplus
}
#endif

#endif // CACHE_H