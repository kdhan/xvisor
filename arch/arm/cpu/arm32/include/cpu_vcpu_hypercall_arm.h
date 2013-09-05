/**
 * Copyright (c) 2011 Anup Patel.
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
 * @file cpu_vcpu_hypercall_arm.h
 * @author Anup Patel (anup@brainfault.org)
 * @brief header file to emulate ARM hypercall instructions
 */
#ifndef _CPU_VCPU_HYPERCALL_ARM_H__
#define _CPU_VCPU_HYPERCALL_ARM_H__

#include <vmm_types.h>

#define ARM_HYPERCALL_CPS_ID			0
#define ARM_HYPERCALL_CPS_SUBID			0
#define ARM_HYPERCALL_CPS_IMOD_END		16
#define ARM_HYPERCALL_CPS_IMOD_START		15
#define ARM_HYPERCALL_CPS_M_END			14
#define ARM_HYPERCALL_CPS_M_START		14
#define ARM_HYPERCALL_CPS_A_END			13
#define ARM_HYPERCALL_CPS_A_START		13
#define ARM_HYPERCALL_CPS_I_END			12
#define ARM_HYPERCALL_CPS_I_START		12
#define ARM_HYPERCALL_CPS_F_END			11
#define ARM_HYPERCALL_CPS_F_START		11
#define ARM_HYPERCALL_CPS_MODE_END		10
#define ARM_HYPERCALL_CPS_MODE_START		6

#define ARM_HYPERCALL_MRS_ID			0
#define ARM_HYPERCALL_MRS_SUBID			1
#define ARM_HYPERCALL_MRS_RD_END		16
#define ARM_HYPERCALL_MRS_RD_START		13
#define ARM_HYPERCALL_MRS_R_END			12
#define ARM_HYPERCALL_MRS_R_START		12

#define ARM_HYPERCALL_MSR_I_ID			0
#define ARM_HYPERCALL_MSR_I_SUBID		2
#define ARM_HYPERCALL_MSR_I_MASK_END		16
#define ARM_HYPERCALL_MSR_I_MASK_START		13
#define ARM_HYPERCALL_MSR_I_IMM12_END		12
#define ARM_HYPERCALL_MSR_I_IMM12_START		1
#define ARM_HYPERCALL_MSR_I_R_END		0
#define ARM_HYPERCALL_MSR_I_R_START		0

#define ARM_HYPERCALL_MSR_R_ID			0
#define ARM_HYPERCALL_MSR_R_SUBID		3
#define ARM_HYPERCALL_MSR_R_MASK_END		16
#define ARM_HYPERCALL_MSR_R_MASK_START		13
#define ARM_HYPERCALL_MSR_R_RN_END		12
#define ARM_HYPERCALL_MSR_R_RN_START		9
#define ARM_HYPERCALL_MSR_R_R_END		8
#define ARM_HYPERCALL_MSR_R_R_START		8

#define ARM_HYPERCALL_RFE_ID			0
#define ARM_HYPERCALL_RFE_SUBID			4
#define ARM_HYPERCALL_RFE_P_END			16
#define ARM_HYPERCALL_RFE_P_START		16
#define ARM_HYPERCALL_RFE_U_END			15
#define ARM_HYPERCALL_RFE_U_START		15
#define ARM_HYPERCALL_RFE_W_END			14
#define ARM_HYPERCALL_RFE_W_START		14
#define ARM_HYPERCALL_RFE_RN_END		13
#define ARM_HYPERCALL_RFE_RN_START		10

#define ARM_HYPERCALL_SRS_ID			0
#define ARM_HYPERCALL_SRS_SUBID			5
#define ARM_HYPERCALL_SRS_P_END			16
#define ARM_HYPERCALL_SRS_P_START		16
#define ARM_HYPERCALL_SRS_U_END			15
#define ARM_HYPERCALL_SRS_U_START		15
#define ARM_HYPERCALL_SRS_W_END			14
#define ARM_HYPERCALL_SRS_W_START		14
#define ARM_HYPERCALL_SRS_MODE_END		13
#define ARM_HYPERCALL_SRS_MODE_START		10

#define ARM_HYPERCALL_WFI_ID			0
#define ARM_HYPERCALL_WFI_SUBID			6

#define ARM_HYPERCALL_LDM_UE_ID0		1
#define ARM_HYPERCALL_LDM_UE_ID1		2
#define ARM_HYPERCALL_LDM_UE_ID2		3
#define ARM_HYPERCALL_LDM_UE_ID3		4
#define ARM_HYPERCALL_LDM_UE_ID4		5
#define ARM_HYPERCALL_LDM_UE_ID5		6
#define ARM_HYPERCALL_LDM_UE_ID6		7
#define ARM_HYPERCALL_LDM_UE_ID7		8
#define ARM_HYPERCALL_LDM_UE_RN_END		19
#define ARM_HYPERCALL_LDM_UE_RN_START		16
#define ARM_HYPERCALL_LDM_UE_REGLIST_END	15
#define ARM_HYPERCALL_LDM_UE_REGLIST_START	0

#define ARM_HYPERCALL_STM_U_ID0			9
#define ARM_HYPERCALL_STM_U_ID1			10
#define ARM_HYPERCALL_STM_U_ID2			11
#define ARM_HYPERCALL_STM_U_ID3			12
#define ARM_HYPERCALL_STM_U_RN_END		19
#define ARM_HYPERCALL_STM_U_RN_START		16
#define ARM_HYPERCALL_STM_U_REGLIST_END		15
#define ARM_HYPERCALL_STM_U_REGLIST_START	0

#define ARM_HYPERCALL_SUBS_REL_ID0		13
#define ARM_HYPERCALL_SUBS_REL_ID1		14
#define ARM_HYPERCALL_SUBS_REL_OPCODE_END	19
#define ARM_HYPERCALL_SUBS_REL_OPCODE_START	16
#define ARM_HYPERCALL_SUBS_REL_RN_END		15
#define ARM_HYPERCALL_SUBS_REL_RN_START		12
#define ARM_HYPERCALL_SUBS_REL_IMM12_END	11
#define ARM_HYPERCALL_SUBS_REL_IMM12_START	0
#define ARM_HYPERCALL_SUBS_REL_IMM5_END		11
#define ARM_HYPERCALL_SUBS_REL_IMM5_START	7
#define ARM_HYPERCALL_SUBS_REL_TYPE_END		6
#define ARM_HYPERCALL_SUBS_REL_TYPE_START	5
#define ARM_HYPERCALL_SUBS_REL_RM_END		3
#define ARM_HYPERCALL_SUBS_REL_RM_START		0

#define ARM_INST_HYPERCALL_ID_MASK		0x00F00000
#define ARM_INST_HYPERCALL_ID_SHIFT		20
#define ARM_INST_HYPERCALL_SUBID_MASK		0x000E0000
#define ARM_INST_HYPERCALL_SUBID_SHIFT		17
#define ARM_INST_HYPERCALL_WFX_MASK		0x00018000
#define ARM_INST_HYPERCALL_WFX_SHIFT		15

/** Emulate ARM hypercall instruction */
int cpu_vcpu_hypercall_arm(struct vmm_vcpu *vcpu, arch_regs_t *regs, u32 inst);

#endif
