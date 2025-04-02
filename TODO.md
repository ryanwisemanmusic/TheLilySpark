Here are a few of the things I need to work on with regards to these libraries:

#include <IOKit/IOKitLib.h>
#include <mach/mach.h>


As we work on bringing over the abstract and rewriting it, there are a few
areas that will cause us problems if we don't handle it. I've kept the code as
minimum as it can be since I want to see where things break given I am staying
as barebones as possible

An area of potential problem is:
osport (which was initially: mach_port_t)


The next problematic part is this:
typedef unsigned int sys_return_t    (changed from typedef unsigned int kern_return_t);

This is a call stored in IOKit/IOKitLib.h, and I've only defined it in
systypes.h so I don't get an error.

The last area of concern is:
IO_Call_Invoke (IOConnectCallMethod)

This is also relevant to IOKit/IOKitLib.h, so these are two areas that will need
some massive addressing
