// Copyright (c) 2020 Apple Inc. All rights reserved.
//
// @APPLE_OSREFERENCE_LICENSE_HEADER_START@
//
// This file contains Original Code and/or Modifications of Original Code
// as defined in and that are subject to the Apple Public Source License
// Version 2.0 (the 'License'). You may not use this file except in
// compliance with the License. The rights granted to you under the License
// may not be used to create, or enable the creation or redistribution of,
// unlawful or unlicensed copies of an Apple operating system, or to
// circumvent, violate, or enable the circumvention or violation of, any
// terms of an Apple operating system software license agreement.
//
// Please obtain a copy of the License at
// http://www.opensource.apple.com/apsl/ and read it before using this file.
//
// The Original Code and all software distributed under the License are
// distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
// EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
// INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
// Please see the License for the specific language governing rights and
// limitations under the License.
//
// @APPLE_OSREFERENCE_LICENSE_HEADER_END@

#ifndef KERN_PERFMON_H
#define KERN_PERFMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>
//#include <sys/perfmon_private.h>

__BEGIN_DECLS

/**
 * perfmon_kind_t
 *
 * Describes which hardware performance monitor to use:
 *   perfmon_cpmu  — core PMU
 *   perfmon_upmu  — uncore PMU
 */
enum perfmon_kind {
    perfmon_cpmu      = 0,
    perfmon_upmu      = 1,
    perfmon_kind_max
};

#if CONFIG_PERFMON

/**
 * perfmon_acquire   — grab exclusive access to the PMU
 * @kind:     which monitor
 * @name:     caller identifier (for debugging)
 *
 * Returns true if successful, false otherwise.
 */
__result_use_check
bool perfmon_acquire(enum perfmon_kind kind, const char *name);

/**
 * perfmon_in_use    — check if PMU is currently held
 * @kind:     which monitor
 *
 * Returns true if someone holds it, false otherwise.
 */
__result_use_check
bool perfmon_in_use(enum perfmon_kind kind);

/**
 * perfmon_release   — release a previously acquired PMU
 * @kind:     which monitor
 * @name:     caller identifier (for debugging)
 *
 * Panics if not currently held.
 */
void perfmon_release(enum perfmon_kind kind, const char *name);

/**
 * struct perfmon_source
 *
 * Describes an available set of PMU registers/attributes.
 */
struct perfmon_source {
    const char           *ps_name;
    const perfmon_name_t *ps_register_names;
    const perfmon_name_t *ps_attribute_names;
    struct perfmon_layout ps_layout;
    enum perfmon_kind     ps_kind;
    bool                  ps_supported;
};

/* Global array of available sources, indexed by perfmon_kind */
extern struct perfmon_source perfmon_sources[perfmon_kind_max];

/**
 * Reserve a source for use (increments internal refcount).
 */
struct perfmon_source *
perfmon_source_reserve(enum perfmon_kind kind);

/**
 * Sample all registers from a reserved source.
 * @src:       pointer returned by perfmon_source_reserve()
 * @out_regs:  user buffer (length >= pl_reg_count * pl_unit_count)
 */
void perfmon_source_sample_regs(struct perfmon_source *src,
                                uint64_t *out_regs, size_t regs_len);

/* Maximums for event and attribute counts */
#define PERFMON_SPEC_MAX_EVENT_COUNT  (16)
#define PERFMON_SPEC_MAX_ATTR_COUNT   (32)

/* Opaque handle for building configurations */
typedef struct perfmon_config *perfmon_config_t;

/**
 * Create a new configuration object for the given source.
 */
perfmon_config_t
perfmon_config_create(struct perfmon_source *source);

/**
 * Add an event or attribute to the config.
 */
int perfmon_config_add_event(perfmon_config_t, const struct perfmon_event *event);
int perfmon_config_set_attr(perfmon_config_t, const struct perfmon_attr *attr);

/**
 * Pushes the built config down to hardware.
 */
int perfmon_configure(perfmon_config_t);

/**
 * Returns a pointer to the filled-in perfmon_spec after CONFIGURE.
 */
struct perfmon_spec *
perfmon_config_specify(perfmon_config_t);

/**
 * Destroy/free a perfmon_config_t.
 */
void perfmon_config_destroy(perfmon_config_t);

#else  /* !CONFIG_PERFMON */

/* Stubs when performance monitoring is disabled */
#define perfmon_acquire(...)       (true)
#define perfmon_in_use(...)        (false)
#define perfmon_release(...)       do { } while (0)

#endif /* CONFIG_PERFMON */

__END_DECLS

#endif /* KERN_PERFMON_H */
