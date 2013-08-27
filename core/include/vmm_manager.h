/**
 * Copyright (c) 2010 Anup Patel.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @file vmm_manager.h
 * @author Anup Patel (anup@brainfault.org)
 * @brief header file for hypervisor manager
 */
#ifndef _VMM_MANAGER_H__
#define _VMM_MANAGER_H__

#include <arch_regs.h>
#include <vmm_limits.h>
#include <vmm_types.h>
#include <vmm_spinlocks.h>
#include <vmm_devtree.h>
#include <vmm_cpumask.h>
#include <libs/list.h>

enum vmm_region_flags {
	VMM_REGION_REAL=0x00000001,
	VMM_REGION_VIRTUAL=0x00000002,
	VMM_REGION_ALIAS=0x00000004,
	VMM_REGION_MEMORY=0x00000008,
	VMM_REGION_IO=0x00000010,
	VMM_REGION_CACHEABLE=0x00000020,
	VMM_REGION_BUFFERABLE=0x00000040,
	VMM_REGION_READONLY=0x00000080,
	VMM_REGION_ISRAM=0x00000100,
	VMM_REGION_ISROM=0x00000200,
	VMM_REGION_ISDEVICE=0x00000400,
};

#define VMM_REGION_MANIFEST_MASK	(VMM_REGION_REAL | \
					 VMM_REGION_VIRTUAL | \
					 VMM_REGION_ALIAS)

struct vmm_region;
struct vmm_guest_aspace;
struct vmm_vcpu_irqs;
struct vmm_vcpu;
struct vmm_guest;

struct vmm_region {
	struct dlist head;
	struct vmm_devtree_node *node;
	struct vmm_guest_aspace *aspace;
	physical_addr_t gphys_addr;
	physical_addr_t hphys_addr;
	physical_size_t phys_size;
	u32 flags;
	void *devemu_priv;
};

struct vmm_guest_aspace {
	struct vmm_devtree_node *node;
	struct vmm_guest *guest;
	struct dlist reg_list;
	void *devemu_priv;
};

#define list_for_each_region(curr, aspace)	\
			list_for_each(curr, &(aspace->reg_list))

struct vmm_vcpu_irqs {
	vmm_spinlock_t lock;
	u32 irq_count;
	bool *assert;
	u32 *reason;
	int execute_pending;
	u64 assert_count;
	u64 execute_count;
	u64 deassert_count;
	struct {
		vmm_spinlock_t lock;
		bool state;
		void *priv;
	} wfi;
};

struct vmm_guest {
	struct dlist head;

	/* General information */
	u32 id;
	char name[VMM_FIELD_NAME_SIZE];
	struct vmm_devtree_node *node;
	bool is_big_endian;
	u32 reset_count;

	/* VCPU instances belonging to this Guest */
	vmm_rwlock_t vcpu_lock;
	u32 vcpu_count;
	struct dlist vcpu_list;

	/* Guest address space */
	struct vmm_guest_aspace aspace;

	/* Architecture specific context */
	void *arch_priv;
};

#define list_for_each_vcpu(curr, guest)	\
			list_for_each(curr, &(guest->vcpu_list))

enum vmm_vcpu_states {
	VMM_VCPU_STATE_UNKNOWN = 0x01,
	VMM_VCPU_STATE_RESET = 0x02,
	VMM_VCPU_STATE_READY = 0x04,
	VMM_VCPU_STATE_RUNNING = 0x08,
	VMM_VCPU_STATE_PAUSED = 0x10,
	VMM_VCPU_STATE_HALTED = 0x20
};

#define VMM_VCPU_STATE_SAVEABLE		( VMM_VCPU_STATE_RUNNING | \
					  VMM_VCPU_STATE_PAUSED | \
					  VMM_VCPU_STATE_HALTED )

#define VMM_VCPU_STATE_INTERRUPTIBLE	( VMM_VCPU_STATE_RUNNING | \
					  VMM_VCPU_STATE_READY | \
					  VMM_VCPU_STATE_PAUSED )

#define VMM_VCPU_MIN_PRIORITY		0
#define VMM_VCPU_MAX_PRIORITY		7
#define VMM_VCPU_DEF_PRIORITY		3
#define VMM_VCPU_DEF_TIME_SLICE		(CONFIG_TSLICE_MS * 1000000)

struct vmm_vcpu {
	struct dlist head;

	/* General information */
	u32 id;
	u32 subid;
	char name[VMM_FIELD_NAME_SIZE];
	struct vmm_devtree_node *node;
	bool is_normal;
	struct vmm_guest *guest;

	/* Start PC and stack */
	virtual_addr_t start_pc;
	virtual_addr_t stack_va;
	virtual_size_t stack_sz;

	/* Scheduling & load balancing context */
	vmm_rwlock_t sched_lock;
	u32 hcpu;
	const struct vmm_cpumask *cpu_affinity;
	u32 state;
	u64 state_tstamp;
	u64 state_ready_nsecs;
	u64 state_running_nsecs;
	u64 state_paused_nsecs;
	u64 state_halted_nsecs;
	u32 reset_count;
	u64 reset_tstamp;
	u8 priority;
	u32 preempt_count;
	u64 time_slice;
	void *sched_priv;

	/* Architecture specific context */
	arch_regs_t regs;
	void *arch_priv;

	/* Virtual IRQ context */
	struct vmm_vcpu_irqs irqs;

	/* Waitqueue parameters */
	struct dlist wq_head;
	void *wq_priv;

	/* Device Emulation Context */
	void *devemu_priv;
};

/** Maximum number of VCPUs */
u32 vmm_manager_max_vcpu_count(void);

/** Current number of VCPUs (orphan + normal) */
u32 vmm_manager_vcpu_count(void);

/** Retrieve VCPU with given ID. 
 *  Returns NULL if there is no VCPU associated with given ID.
 */
struct vmm_vcpu *vmm_manager_vcpu(u32 vcpu_id);

/** Iterate over each VCPU */
int vmm_manager_vcpu_iterate(int (*iter)(struct vmm_vcpu *, void *), 
			     void *priv);

/** Retrive general VCPU statistics */
int vmm_manager_vcpu_stats(struct vmm_vcpu *vcpu,
			   u32 *state,
			   u8  *priority,
			   u32 *hcpu,
			   u32 *reset_count,
			   u64 *last_reset_nsecs,
			   u64 *ready_nsecs,
			   u64 *running_nsecs,
			   u64 *paused_nsecs,
			   u64 *halted_nsecs);

/** Retriver VCPU state */
u32 vmm_manager_vcpu_get_state(struct vmm_vcpu *vcpu);

/** Update VCPU state 
 *  Note: Avoid calling this function directly
 */
int vmm_manager_vcpu_set_state(struct vmm_vcpu *vcpu, u32 state);

/** Reset a VCPU */
#define vmm_manager_vcpu_reset(vcpu)	\
		vmm_manager_vcpu_set_state((vcpu), VMM_VCPU_STATE_RESET)

/** Kick a VCPU out of reset state */
#define vmm_manager_vcpu_kick(vcpu)	\
		vmm_manager_vcpu_set_state((vcpu), VMM_VCPU_STATE_READY)

/** Pause a VCPU */
#define vmm_manager_vcpu_pause(vcpu)	\
		vmm_manager_vcpu_set_state((vcpu), VMM_VCPU_STATE_PAUSED)

/** Resume a VCPU */
#define vmm_manager_vcpu_resume(vcpu)	\
		vmm_manager_vcpu_set_state((vcpu), VMM_VCPU_STATE_READY)

/** Halt a VCPU */
#define vmm_manager_vcpu_halt(vcpu)	\
		vmm_manager_vcpu_set_state((vcpu), VMM_VCPU_STATE_HALTED)

/** Retrive host CPU assigned to given VCPU */
int vmm_manager_vcpu_get_hcpu(struct vmm_vcpu *vcpu, u32 *hcpu);

/** Update host CPU assigned to given VCPU */
int vmm_manager_vcpu_set_hcpu(struct vmm_vcpu *vcpu, u32 hcpu);

/** Retrive host CPU affinity of given VCPU */
const struct vmm_cpumask *vmm_manager_vcpu_get_affinity(struct vmm_vcpu *vcpu);

/** Update host CPU affinity of given VCPU */
int vmm_manager_vcpu_set_affinity(struct vmm_vcpu *vcpu, 
				  const struct vmm_cpumask *cpu_mask);

/** Create an orphan VCPU */
struct vmm_vcpu *vmm_manager_vcpu_orphan_create(const char *name,
					    virtual_addr_t start_pc,
					    virtual_size_t stack_sz,
					    u8 priority,
					    u64 time_slice_nsecs);

/** Destroy an orphan VCPU */
int vmm_manager_vcpu_orphan_destroy(struct vmm_vcpu *vcpu);

/** Maximum number of Guests */
u32 vmm_manager_max_guest_count(void);

/** Current number of Guests */
u32 vmm_manager_guest_count(void);

/** Retrieve Guest with given ID. 
 *  Returns NULL if there is no Guest associated with given ID.
 */
struct vmm_guest *vmm_manager_guest(u32 guest_id);

/** Find Guest with given name. 
 *  Returns NULL if there is no Guest with given name.
 */
struct vmm_guest *vmm_manager_guest_find(const char *guest_name);

/** Number of VCPUs belonging to a given Guest */
u32 vmm_manager_guest_vcpu_count(struct vmm_guest *guest);

/** Retrieve VCPU belonging to a given Guest with particular subid */
struct vmm_vcpu *vmm_manager_guest_vcpu(struct vmm_guest *guest, u32 subid);

/** Iterate over each VCPU of a given Guest */
int vmm_manager_guest_vcpu_iterate(struct vmm_guest *guest,
				   int (*iter)(struct vmm_vcpu *, void *), 
				   void *priv);

/** Reset a Guest */
int vmm_manager_guest_reset(struct vmm_guest *guest);

/** Kick a Guest out of reset state */
int vmm_manager_guest_kick(struct vmm_guest *guest);

/** Pause a Guest */
int vmm_manager_guest_pause(struct vmm_guest *guest);

/** Resume a Guest */
int vmm_manager_guest_resume(struct vmm_guest *guest);

/** Halt a Guest */
int vmm_manager_guest_halt(struct vmm_guest *guest);

/** Create a Guest based on device tree configuration */
struct vmm_guest *vmm_manager_guest_create(struct vmm_devtree_node *gnode);

/** Destroy a Guest */
int vmm_manager_guest_destroy(struct vmm_guest *guest);

/** Initialize manager */
int vmm_manager_init(void);

#endif
