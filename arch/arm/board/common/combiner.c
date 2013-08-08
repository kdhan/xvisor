#include <vmm_error.h>
#include <vmm_macros.h>
#include <vmm_smp.h>
#include <vmm_cpumask.h>
#include <vmm_stdio.h>
#include <vmm_host_io.h>
#include <vmm_host_irq.h>
#include <arch_barrier.h>
#include <gic.h>
#include <exynos/irqs.h>
/* by kordo */
#include <vmm_host_aspace.h>

#define COMBINER_ENABLE_SET     0x0
#define COMBINER_ENABLE_CLEAR   0x4
#define COMBINER_INT_STATUS     0xC

struct combiner_chip_data {
	u32 irq_offset;
	u32 irq_mask;
	virtual_addr_t base;
	u32 parent_irq;
};

#define combiner_write(val, addr)	vmm_writel((val), (void *)(addr))
#define combiner_read(addr)		vmm_readl((void *)(addr))

static struct combiner_chip_data combiner_data[MAX_COMBINER_NR];

static inline virtual_addr_t combiner_base(struct vmm_host_irq *irq)
{
	struct combiner_chip_data *combiner_data = vmm_host_irq_get_chip_data(irq);

	return combiner_data->base;
}

static void combiner_mask_irq(struct vmm_host_irq *irq)
{
	u32 mask = 1 << (irq->num % 32);

	combiner_write(mask, combiner_base(irq) + COMBINER_ENABLE_CLEAR);
}

static void combiner_unmask_irq(struct vmm_host_irq *irq)
{
	u32 mask = 1 << (irq->num % 32);

	combiner_write(mask, combiner_base(irq) + COMBINER_ENABLE_SET);
}

static vmm_irq_return_t combiner_handle_cascade_irq(int irq, void *dev)
{
	struct combiner_chip_data *chip_data = dev;
	u32 cascade_irq, combiner_irq;

	combiner_irq = combiner_read(chip_data->base + COMBINER_INT_STATUS);
	combiner_irq &= chip_data->irq_mask;

	if (combiner_irq == 0)
		return VMM_IRQ_NONE;

	combiner_irq = __ffs(combiner_irq);

	cascade_irq = combiner_irq + (chip_data->irq_offset & ~31);

//	if (likely(cascade_irq > NR_IRQS)) {
	if (likely(cascade_irq > 520)) {
		vmm_host_generic_irq_exec(cascade_irq);
	}

	return VMM_IRQ_HANDLED;
}

#ifdef CONFIG_SMP
static int combiner_set_affinity(struct vmm_host_irq *d,
				 const struct cpumask *mask_val,
				 bool force)
{
	struct combiner_chip_data *cd = vmm_host_irq_get_irq_chip_data(d);
	struct irq_chip *chip = irq_get_chip(cd->parent_irq);
	struct vmm_host_irq *pd = irq_get_vmm_host_irq(cd->parent_irq);

	if (chip && chip->irq_set_affinity)
		return chip->irq_set_affinity(pd, mask_val, force);
	else
		return -EINVAL;
}
#endif

static struct vmm_host_irq_chip combiner_chip = {
	.name			= "COMBINER",
	.irq_mask		= combiner_mask_irq,
	.irq_unmask		= combiner_unmask_irq,
#ifdef CONFIG_SMP
	.irq_set_affinity	= combiner_set_affinity,
#endif
};

void __init combiner_cascade_irq(unsigned int combiner_nr, unsigned int irq)
{
	if (combiner_nr >= EXYNOS5_MAX_COMBINER_NR)
		BUG();

	if (vmm_host_irq_register(irq, "COMBINER-CHILD", 
				  combiner_handle_cascade_irq, 
				  &combiner_data[combiner_nr])) {
		BUG();
	}
	combiner_data[combiner_nr].parent_irq = irq;
}

void __init combiner_init(unsigned int combiner_nr, virtual_addr_t base, unsigned int irq_start)
{
	unsigned int i;

//	vmm_printf("%s [%d], base 0x%x\n", __func__, __LINE__, base);

	if (combiner_nr >= EXYNOS5_MAX_COMBINER_NR)
		BUG();

	combiner_data[combiner_nr].base = base;
	combiner_data[combiner_nr].irq_offset = irq_start;
	combiner_data[combiner_nr].irq_mask = 0xff << ((combiner_nr % 4) << 3);
	/* Disable all interrupts */
	
	combiner_write(combiner_data[combiner_nr].irq_mask,
		     base + COMBINER_ENABLE_CLEAR);

	/* Setup the Linux IRQ subsystem */

	for (i = irq_start; i < combiner_data[combiner_nr].irq_offset
				+ MAX_IRQ_IN_COMBINER; i++) {
		vmm_host_irq_set_chip(i, &combiner_chip);
		vmm_host_irq_set_chip_data(i, &combiner_data[combiner_nr]);
		vmm_host_irq_set_handler(i, vmm_handle_level_irq);
	}
//	vmm_printf("%s [%d]\n", __func__, __LINE__);
}

int __init combiner_devtree_init(struct vmm_devtree_node *node, 
			    struct vmm_devtree_node *parent)
{
	int rc;
	u32 *aval, irq;
	virtual_addr_t combiner_base;

	if (WARN_ON(!node)) {
		return VMM_ENODEV;
	}

	rc = vmm_devtree_regmap(node, &combiner_base, 0);

	vmm_printf("%s [%d] base [0x%x]\n", __func__, __LINE__, combiner_base);
	vmm_printf("%s [%d] base [0x%x]\n", __func__, __LINE__, MAX_COMBINER_NR);
	for (irq = 0; irq < MAX_COMBINER_NR; irq ++)
	{
		combiner_init(irq, combiner_base, COMBINER_IRQ(irq, 0));
		combiner_cascade_irq(irq, IRQ_SPI(irq));

	}
	WARN(rc, "unable to map gic dist registers\n");

	return VMM_OK;
}

