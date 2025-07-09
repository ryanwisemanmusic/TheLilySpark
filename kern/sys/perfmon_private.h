/*
This is potentially the perfmon_private.h file that is
completely unknown. Since this is only for the sake
of peering into the Apple Software so we can reverse engineer
direct talking to the graphics card, this won't even be required
in an actual application build.
*/
#ifndef SYS_PERFMON_PRIVATE_H
#define SYS_PERFMON_PRIVATE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/ioctl.h>  // For _IOR, _IOW macros


// I/O Control Commands
#define PERFMON_IOCTL_BASE  'P'
#define PERFMON_CTL_GET_LAYOUT    _IOR(PERFMON_IOCTL_BASE, 1, struct perfmon_layout)
#define PERFMON_CTL_SPECIFY       _IOW(PERFMON_IOCTL_BASE, 2, struct perfmon_spec)
#define PERFMON_CTL_LIST_REGS     _IOR(PERFMON_IOCTL_BASE, 3, struct perfmon_name)
#define PERFMON_CTL_SAMPLE_REGS   _IOR(PERFMON_IOCTL_BASE, 4, uint64_t)
#define PERFMON_CTL_ADD_EVENT     _IOW(PERFMON_IOCTL_BASE, 5, struct perfmon_event)

#define PERFMON_SPEC_MAX_EVENT_COUNT   16
#define PERFMON_SPEC_MAX_ATTR_COUNT    32

typedef enum 
{
    PERFMON_KIND_CORE = 0,
    PERFMON_KIND_UNCORE = 1,
    PERFMON_KIND_MAX
} perfmon_kind_t;

// Explicit struct definition
struct perfmon_layout {
    uint32_t pl_reg_count;
    uint32_t pl_unit_count;
};
typedef struct perfmon_layout perfmon_layout_t;

// Explicit struct definition
struct perfmon_name {
    const char *pn_name;
    uint32_t    pn_code;
};
typedef struct perfmon_name perfmon_name_t;

// Explicit struct definition with corrected field name
struct perfmon_event {
    const char *pe_name;
    uint32_t    pe_number;  // Changed from pe_code to match usage
    uint32_t    pe_counter;
};
typedef struct perfmon_event perfmon_event_t;

// Explicit struct definition
struct perfmon_attr {
    const char *pa_name; 
    uint32_t    pa_id;
    uint64_t    pa_value;
};
typedef struct perfmon_attr perfmon_attr_t;

// Explicit struct definition
struct perfmon_spec {
    uint32_t              ps_event_count;
    perfmon_event_t       pc_sevents[PERFMON_SPEC_MAX_EVENT_COUNT];
    uint32_t              pc_attr_count;
    perfmon_attr_t        ps_attrs[PERFMON_SPEC_MAX_ATTR_COUNT];
    uint32_t              ps_attrs_mask;
};
typedef struct perfmon_spec perfmon_spec_t;

typedef struct perfmon_source perfmon_source_t;
typedef struct perfmon_config perfmon_config_t;

typedef struct perfmon_counter {
    uint64_t pc_number;
} perfmon_counter_t;

struct perfmon_config
{
    perfmon_source_t     *pc_sources;
    perfmon_spec_t        pc_spec;
    unsigned short        pc_attr_ids[PERFMON_SPEC_MAX_ATTR_COUNT];
    perfmon_counter_t    *pc_counters;
    uint64_t              pc_attrs_used;
    bool                  pc_configured : 1;
};

void perfmon_machine_sample_regs(perfmon_kind_t kind, 
                                 uint64_t *regs, 
                                 size_t regs_len);

int perfmon_machine_configure(perfmon_kind_t kind,
                              perfmon_config_t *config);

void perfmon_machine_reset(perfmon_kind_t kind);
#endif