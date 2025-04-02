#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

#include <IOKit/IOKitLib.h>
#include <stdint.h>
#include <stddef.h>

#ifdef KERNEL
#include <mach/mach.h>
typedef mach_port_t osport;
#else
typedef unsigned int osport;
#endif

typedef unsigned int sys_return_t;

#ifdef KERNEL
#include <mach/kern_return.h>
typedef kern_return_t sys_return_t;
#endif

#define IO_Call_Invoke IOConnectCallMethod

#ifndef size_t
typedef __SIZE_TYPE__ size_t;
#endif

#endif /* _SYS_TYPES_H_ */
