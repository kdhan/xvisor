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
 * @file vmm_vcpu_irq.c
 * @author Anup Patel (anup@brainfault.org)
 * @brief source code for vcpu irq processing
 */

#include <arch_vcpu.h>
#include <vmm_error.h>
#include <vmm_heap.h>
#include <vmm_stdio.h>
#include <vmm_timer.h>
#include <vmm_scheduler.h>
#include <vmm_devtree.h>
#include <vmm_vcpu_irq.h>
#include <libs/stringlib.h>

void vmm_vcpu_irq_process(struct vmm_vcpu *vcpu, arch_regs_t *regs)
{
	int rc;
	int irq_no = -1;
	irq_flags_t flags;
	u32 i, state, tmp_prio, irq_count;
	u32 irq_prio = 0;

	/* For non-normal vcpu dont do anything */
	if (!vcpu || !vcpu->is_normal) {
		return;
	}

	/* If vcpu is not in interruptible state then dont do anything */
	state = vmm_manager_vcpu_get_state(vcpu);
	if (!(state & VMM_VCPU_STATE_INTERRUPTIBLE)) {
		return;
	}

	/* Lock VCPU irqs */
	vmm_spin_lock_irqsave_lite(&vcpu->irqs.lock, flags);

	/* Proceed only if we have pending execute */
	if (vcpu->irqs.execute_pending < 1) {
		vmm_spin_unlock_irqrestore_lite(&vcpu->irqs.lock, flags);
		return;
	}

	/* Get saved irq count */
	irq_count = vcpu->irqs.irq_count;

	/* Find the irq number to process */
	for (i = 0; i < irq_count; i++) {
		if (vcpu->irqs.assert[i]) {
			tmp_prio = arch_vcpu_irq_priority(vcpu, i);
			if (tmp_prio > irq_prio) {
				irq_no = i;
				irq_prio = tmp_prio;
			}
		}
	}

	/* If irq number found then execute it */
	if (irq_no != -1) {
		rc = arch_vcpu_irq_execute(vcpu, regs, irq_no, vcpu->irqs.reason[irq_no]);
		if (rc == VMM_OK) {
			vcpu->irqs.assert[irq_no] = FALSE;
			vcpu->irqs.execute_pending--;
			vcpu->irqs.execute_count++;
		}
	}

	/* Unlock VCPU irqs */
	vmm_spin_unlock_irqrestore_lite(&vcpu->irqs.lock, flags);
}

static void vcpu_irq_wfi_resume(struct vmm_vcpu *vcpu)
{
	bool wfi_state;
	irq_flags_t flags;

	if (!vcpu) {
		return;
	}

	/* Lock VCPU WFI */
	vmm_spin_lock_irqsave_lite(&vcpu->irqs.wfi.lock, flags);

	/* If VCPU was in wfi state then update state. */
	wfi_state = vcpu->irqs.wfi.state;
	if (wfi_state) {
		/* Clear wait for irq state */
		vcpu->irqs.wfi.state = FALSE;

		/* Stop wait for irq timeout event */
		vmm_timer_event_stop(vcpu->irqs.wfi.priv);
	}	

	/* Unlock VCPU WFI */
	vmm_spin_unlock_irqrestore_lite(&vcpu->irqs.wfi.lock, flags);

	/* Try to resume the VCPU */
	if (wfi_state) {
		vmm_manager_vcpu_resume(vcpu);
	}
}

static void vcpu_irq_wfi_timeout(struct vmm_timer_event *ev)
{
	vcpu_irq_wfi_resume(ev->priv);
}

void vmm_vcpu_irq_assert(struct vmm_vcpu *vcpu, u32 irq_no, u32 reason)
{
	int rc;
	u32 state;
	irq_flags_t flags;

	/* For non-normal VCPU dont do anything */
	if (!vcpu || !vcpu->is_normal) {
		return;
	}

	/* If VCPU is not in interruptible state then dont do anything */
	state = vmm_manager_vcpu_get_state(vcpu);
	if (!(state & VMM_VCPU_STATE_INTERRUPTIBLE)) {
		return;
	}

	/* Lock VCPU irqs */
	vmm_spin_lock_irqsave_lite(&vcpu->irqs.lock, flags);

	/* Check irq number */
	if (irq_no > vcpu->irqs.irq_count) {
		vmm_spin_unlock_irqrestore_lite(&vcpu->irqs.lock, flags);
		return;
	}

	/* Assert the irq */
	if (!vcpu->irqs.assert[irq_no]) {
		rc = arch_vcpu_irq_assert(vcpu, irq_no, reason);
		if (rc == VMM_OK) {
			vcpu->irqs.reason[irq_no] = reason;
			vcpu->irqs.assert[irq_no] = TRUE;
			vcpu->irqs.execute_pending++;
			vcpu->irqs.assert_count++;
		}
	}

	/* Unlock VCPU irqs */
	vmm_spin_unlock_irqrestore_lite(&vcpu->irqs.lock, flags);

	/* Resume VCPU from wfi */
	vcpu_irq_wfi_resume(vcpu);
}

void vmm_vcpu_irq_deassert(struct vmm_vcpu *vcpu, u32 irq_no)
{
	int rc;
	u32 reason;
	irq_flags_t flags;

	/* For non-normal vcpu dont do anything */
	if (!vcpu || !vcpu->is_normal) {
		return;
	}

	/* Lock VCPU irqs */
	vmm_spin_lock_irqsave_lite(&vcpu->irqs.lock, flags);

	/* Check irq number */
	if (irq_no > vcpu->irqs.irq_count) {
		vmm_spin_unlock_irqrestore_lite(&vcpu->irqs.lock, flags);
		return;
	}

	/* Get reason for irq */
	reason = vcpu->irqs.reason[irq_no];

	/* Call arch specific deassert */
	rc = arch_vcpu_irq_deassert(vcpu, irq_no, reason);
	if (rc == VMM_OK) {
		vcpu->irqs.deassert_count++;
	}

	/* Adjust assert pending count */
	if (vcpu->irqs.assert[irq_no] &&
	    (vcpu->irqs.execute_pending > 0)) {
		vcpu->irqs.execute_pending--;
	}

	/* Ensure irq is not asserted */
	vcpu->irqs.assert[irq_no] = FALSE;

	/* Ensure irq reason is zeroed */
	vcpu->irqs.reason[irq_no] = 0x0;

	/* Unlock VCPU irqs */
	vmm_spin_unlock_irqrestore_lite(&vcpu->irqs.lock, flags);
}

int vmm_vcpu_irq_wait_resume(struct vmm_vcpu *vcpu)
{
	/* Sanity Checks */
	if (!vcpu || !vcpu->is_normal) {
		return VMM_EFAIL;
	}

	/* Resume VCPU from wfi */
	vcpu_irq_wfi_resume(vcpu);

	return VMM_OK;
}

int vmm_vcpu_irq_wait_timeout(struct vmm_vcpu *vcpu, u64 nsecs)
{
	irq_flags_t flags;

	/* Sanity Checks */
	if (!vcpu || !vcpu->is_normal) {
		return VMM_EFAIL;
	}

	/* Try to pause the VCPU */
	vmm_manager_vcpu_pause(vcpu);

	/* Lock VCPU WFI */
	vmm_spin_lock_irqsave_lite(&vcpu->irqs.wfi.lock, flags);

	/* Set wait for irq state */
	vcpu->irqs.wfi.state = TRUE;

	/* Start wait for irq timeout event */
	if (!nsecs) {
		nsecs = CONFIG_WFI_TIMEOUT_SECS*1000000000ULL;
	}
	vmm_timer_event_start(vcpu->irqs.wfi.priv, nsecs);

	/* Unlock VCPU WFI */
	vmm_spin_unlock_irqrestore_lite(&vcpu->irqs.wfi.lock, flags);

	return VMM_OK;
}

int vmm_vcpu_irq_init(struct vmm_vcpu *vcpu)
{
	int rc;
	u32 ite, irq_count;
	irq_flags_t flags;
	struct vmm_timer_event *ev;

	/* Sanity Checks */
	if (!vcpu) {
		return VMM_EFAIL;
	}

	/* For Orphan VCPU just return */
	if (!vcpu->is_normal) {
		return VMM_OK;
	}

	/* Get irq count */
	irq_count = arch_vcpu_irq_count(vcpu);

	/* Only first time */
	if (!vcpu->reset_count) {
		/* Clear the memory of irq */
		memset(&vcpu->irqs, 0, sizeof(struct vmm_vcpu_irqs));

		/* Initialize irq lock */
		INIT_SPIN_LOCK(&vcpu->irqs.lock);

		/* Allocate memory for flags */
		vcpu->irqs.assert = vmm_zalloc(sizeof(bool) * irq_count);
		if (!vcpu->irqs.assert) {
			return VMM_ENOMEM;
		}
		vcpu->irqs.reason = vmm_zalloc(sizeof(u32) * irq_count);
		if (!vcpu->irqs.reason) {
			vmm_free(vcpu->irqs.assert);
			vcpu->irqs.assert = NULL;
			return VMM_ENOMEM;
		}

		/* Create wfi_timeout event */
		ev = vmm_zalloc(sizeof(struct vmm_timer_event));
		if (!ev) {
			vmm_free(vcpu->irqs.assert);
			vcpu->irqs.assert = NULL;
			vmm_free(vcpu->irqs.reason);
			vcpu->irqs.reason = NULL;
			return VMM_ENOMEM;
		}
		vcpu->irqs.wfi.priv = ev;

		/* Initialize wfi lock */
		INIT_SPIN_LOCK(&vcpu->irqs.wfi.lock);

		/* Initialize wfi timeout event */
		INIT_TIMER_EVENT(ev, vcpu_irq_wfi_timeout, vcpu);
	}

	/* Lock VCPU irqs */
	vmm_spin_lock_irqsave_lite(&vcpu->irqs.lock, flags);

	/* Save irq count */
	vcpu->irqs.irq_count = irq_count;

	/* Set execute pending to zero */
	vcpu->irqs.execute_pending = 0;

	/* Set default assert & deassert counts */
	vcpu->irqs.assert_count = 0;
	vcpu->irqs.execute_count = 0;
	vcpu->irqs.deassert_count = 0;

	/* Reset irq processing data structures for VCPU */
	for (ite = 0; ite < irq_count; ite++) {
		vcpu->irqs.reason[ite] = 0;
		vcpu->irqs.assert[ite] = FALSE;
	}

	/* Setup wait for irq context */
	vcpu->irqs.wfi.state = FALSE;
	rc = vmm_timer_event_stop(vcpu->irqs.wfi.priv);
	if (rc != VMM_OK) {
		vmm_free(vcpu->irqs.assert);
		vcpu->irqs.assert = NULL;
		vmm_free(vcpu->irqs.reason);
		vcpu->irqs.reason = NULL;
		vmm_free(vcpu->irqs.wfi.priv);
		vcpu->irqs.wfi.priv = NULL;
	}

	/* Unlock VCPU irqs */
	vmm_spin_unlock_irqrestore_lite(&vcpu->irqs.lock, flags);

	return rc;
}

int vmm_vcpu_irq_deinit(struct vmm_vcpu *vcpu)
{
	irq_flags_t flags;

	/* Sanity Checks */
	if (!vcpu) {
		return VMM_EFAIL;
	}

	/* For Orphan VCPU just return */
	if (!vcpu->is_normal) {
		return VMM_OK;
	}

	/* Lock VCPU irqs */
	vmm_spin_lock_irqsave_lite(&vcpu->irqs.lock, flags);

	/* Stop wfi_timeout event */
	vmm_timer_event_stop(vcpu->irqs.wfi.priv);

	/* Free wfi_timeout event */
	vmm_free(vcpu->irqs.wfi.priv);
	vcpu->irqs.wfi.priv = NULL;

	/* Free flags */
	vmm_free(vcpu->irqs.assert);
	vcpu->irqs.assert = NULL;
	vmm_free(vcpu->irqs.reason);
	vcpu->irqs.reason = NULL;

	/* Unlock VCPU irqs */
	vmm_spin_unlock_irqrestore_lite(&vcpu->irqs.lock, flags);

	return VMM_OK;
}
