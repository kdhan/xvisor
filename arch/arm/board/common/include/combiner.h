#ifndef __COMBINER_H__
#define __COMBINER_H__

#include <vmm_types.h>
#include <vmm_cpumask.h>
#include <vmm_devtree.h>

int combiner_devtree_init(struct vmm_devtree_node *node, 
		     struct vmm_devtree_node *parent);

#endif /* __COMBINER_H__ */
