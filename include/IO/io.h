#ifndef __AGX_IO_H
#define __AGX_IO_H

#include <stdbool.h>

/*
This is one of the foundational headers we need to work with and
remove the abstractions from/make my own. The reason we don't want this abstraction
has more to do with the fact that I hate abstraction not defined as my
own. 
*/

#include "systypes.h"

#include <IOKit/IOKitLib.h>

enum agx_alloc_type
{
    AGX_ALLOC_REGULAR = 0,
    AGX_ALLOC_MEMMAP = 1,
    AGX_ALLOC_CMDBUF = 2,
    AGX_NUM_ALLOC,
};

static const char *agx_alloc_types[AGX_NUM_ALLOC] = { "mem", "map", "cmd" };

struct agx_allocation
{
    enum agx_alloc_type type;
    size_t size; 

    unsigned index;

    void *map;

    uint64_t gpu_va;
};

struct agx_allocation agx_alloc_mem(osport connection, size_t size);
struct agx_allocation agx_alloc_cmdbuf(osport connection, size_t size, bool cmdbuf);

 
#endif
