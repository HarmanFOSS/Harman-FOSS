// SPDX-License-Identifier: GPL-2.0
/**
 * Test driver to test endpoint functionality
 *
 * Copyright (C) 2017 Texas Instruments
 * Author: Kishon Vijay Abraham I <kishon@ti.com>
 */

#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/pci_ids.h>
#include <linux/random.h>

#include <linux/pci-epc.h>
#include <linux/pci-epf.h>
#include <linux/pci_regs.h>

#define COMMAND_RAISE_LEGACY_IRQ	BIT(0)
#define COMMAND_RAISE_MSI_IRQ		BIT(1)
#define MSI_NUMBER_SHIFT		2
#define MSI_NUMBER_MASK			(0x3f << MSI_NUMBER_SHIFT)
#define COMMAND_READ			BIT(8)
#define COMMAND_WRITE			BIT(9)
#define COMMAND_COPY			BIT(10)
#define COMMAND_DMAREAD			BIT(11)
#define COMMAND_DMAWRITE		BIT(12)

#define STATUS_READ_SUCCESS		BIT(0)
#define STATUS_READ_FAIL		BIT(1)
#define STATUS_WRITE_SUCCESS		BIT(2)
#define STATUS_WRITE_FAIL		BIT(3)
#define STATUS_COPY_SUCCESS		BIT(4)
#define STATUS_COPY_FAIL		BIT(5)
#define STATUS_IRQ_RAISED		BIT(6)
#define STATUS_SRC_ADDR_INVALID		BIT(7)
#define STATUS_DST_ADDR_INVALID		BIT(8)

#define TIMER_RESOLUTION		1

static struct workqueue_struct *kpcitest_workqueue;

struct pci_epf_test {
	void			*reg[6];
	struct pci_epf		*epf;
	enum pci_barno		test_reg_bar;
	bool			linkup_notifier;
	struct delayed_work	cmd_handler;
};

struct pci_epf_test_reg {
	u32	magic;
	u32	command;
	u32	status;
	u64	src_addr;
	u64	dst_addr;
	u32	size;
	u32	checksum;
} __packed;

static struct pci_epf_header test_header = {
	.vendorid	= PCI_ANY_ID,
	.deviceid	= PCI_ANY_ID,
	.baseclass_code = PCI_CLASS_OTHERS,
	.interrupt_pin	= PCI_INTERRUPT_INTA,
};

struct pci_epf_test_data {
	enum pci_barno	test_reg_bar;
	bool		linkup_notifier;
};

static size_t bar_size[] = { 512, 512, 1024, 16384, 131072, 1048576 };

static int pci_epf_test_copy(struct pci_epf_test *epf_test)
{
	enum pci_barno test_reg_bar = epf_test->test_reg_bar;
	struct pci_epf_test_reg *reg = epf_test->reg[test_reg_bar];
	struct pci_epf *epf = epf_test->epf;
	struct device *dev = &epf->dev;
	struct pci_epc *epc = epf->epc;
	phys_addr_t src_phys_addr;
	phys_addr_t dst_phys_addr;
	void __iomem *src_addr;
	void __iomem *dst_addr;
	size_t size;
	int ret;

	size = reg->size;
	src_addr = pci_epc_mem_alloc_addr(epc, &src_phys_addr, size);
	if (!src_addr) {
		dev_err(dev, "failed to allocate source address\n");
		reg->status = STATUS_SRC_ADDR_INVALID;
		ret = -ENOMEM;
		goto err;
	}

	ret = pci_epc_map_addr(epc, epf->func_no, src_phys_addr,
						   reg->src_addr, size);
	if (ret) {
		dev_err(dev, "failed to map source address\n");
		reg->status = STATUS_SRC_ADDR_INVALID;
		goto err_src_addr;
	}

	dst_addr = pci_epc_mem_alloc_addr(epc, &dst_phys_addr, size);
	if (!dst_addr) {
		dev_err(dev, "failed to allocate destination address\n");
		reg->status = STATUS_DST_ADDR_INVALID;
		ret = -ENOMEM;
		goto err_src_map_addr;
	}

	ret = pci_epc_map_addr(epc, epf->func_no, dst_phys_addr, reg->dst_addr,
						   size);
	if (ret) {
		dev_err(dev, "failed to map destination address\n");
		reg->status = STATUS_DST_ADDR_INVALID;
		goto err_dst_addr;
	}

	memcpy(dst_addr, src_addr, size);

	pci_epc_unmap_addr(epc, epf->func_no, dst_phys_addr);

err_dst_addr:
	pci_epc_mem_free_addr(epc, dst_phys_addr, dst_addr, size);

err_src_map_addr:
	pci_epc_unmap_addr(epc, epf->func_no, src_phys_addr);

err_src_addr:
	pci_epc_mem_free_addr(epc, src_phys_addr, src_addr, size);

err:
	return ret;
}

static int pci_epf_test_read(struct pci_epf_test *epf_test, bool dma)
{
	enum pci_barno test_reg_bar = epf_test->test_reg_bar;
	struct pci_epf_test_reg *reg = epf_test->reg[test_reg_bar];
	struct pci_epf *epf = epf_test->epf;
	dma_addr_t local_addr, remote_addr;
	struct device *dev = &epf->dev;
	struct pci_epc *epc = epf->epc;
	void __iomem *src_addr;
	size_t size;
	u32 crc32;
	int ret;

	if (dma && pci_epc_dma_supported(epc) < 0)
		return -EINVAL;

	size = reg->size;
	src_addr = pci_epc_mem_alloc_addr(epc, &remote_addr, size);
	if (!src_addr) {
		dev_err(dev, "failed to allocate address\n");
		reg->status = STATUS_SRC_ADDR_INVALID;
		return -ENOMEM;
	}

	ret = pci_epc_map_addr(epc, epf->func_no, remote_addr, reg->src_addr,
						   size);
	if (ret) {
		dev_err(dev, "failed to map address\n");
		reg->status = STATUS_SRC_ADDR_INVALID;
		goto err_addr;
	}

	if (!dma) {
		void *buf;

		buf = kzalloc(size, GFP_KERNEL);
		if (!buf) {
			ret = -ENOMEM;
			goto err_map_addr;
		}
		memcpy(buf, src_addr, size);
		crc32 = crc32_le(~0, buf, size);
		kfree(buf);
	} else {
		struct device *dev = epf->epc->dev.parent;
		void *buf;

		buf = dma_alloc_coherent(dev, size, &local_addr, GFP_KERNEL);
		if (!buf) {
			ret = -ENOMEM;
			goto err_map_addr;
		}

		if (pci_epc_dma(epc, epf->func_no, local_addr, remote_addr, size,
						DMA_FROM_DEVICE))
			ret = -EIO;
		else
			crc32 = crc32_le(~0, buf, size);

		dma_free_coherent(dev, size, buf, local_addr);
	}

err_map_addr:
	pci_epc_unmap_addr(epc, epf->func_no, remote_addr);

err_addr:
	pci_epc_mem_free_addr(epc, remote_addr, src_addr, size);

	if (!ret && crc32 != reg->checksum)
		ret = -EIO;

	return ret;
}

static int pci_epf_test_write(struct pci_epf_test *epf_test, bool dma)
{
	enum pci_barno test_reg_bar = epf_test->test_reg_bar;
	struct pci_epf_test_reg *reg = epf_test->reg[test_reg_bar];
	struct pci_epf *epf = epf_test->epf;
	dma_addr_t local_addr, remote_addr;
	struct device *dev = &epf->dev;
	struct pci_epc *epc = epf->epc;
	void __iomem *dst_addr;
	size_t size;
	int ret;

	if (dma && pci_epc_dma_supported(epc) < 0)
		return -EINVAL;

	size = reg->size;
	dst_addr = pci_epc_mem_alloc_addr(epc, &remote_addr, size);
	if (!dst_addr) {
		dev_err(dev, "failed to allocate address\n");
		reg->status = STATUS_DST_ADDR_INVALID;
		return -ENOMEM;
	}

	ret = pci_epc_map_addr(epc, epf->func_no, remote_addr, reg->dst_addr,
						   size);
	if (ret) {
		dev_err(dev, "failed to map address\n");
		reg->status = STATUS_DST_ADDR_INVALID;
		goto err_addr;
	}

	if (!dma) {
		void *buf;

		buf = kzalloc(size, GFP_KERNEL);
		if (!buf) {
			ret = -ENOMEM;
			goto err_map_addr;
		}
		get_random_bytes(buf, size);
		reg->checksum = crc32_le(~0, buf, size);
		memcpy(dst_addr, buf, size);

		kfree(buf);

		/*
		 * wait 1ms inorder for the write to complete. Without this delay L3
		 * error in observed in the host system.
		 */
		mdelay(1);
	} else {
		struct device *dev = epf->epc->dev.parent;
		void *buf;

		buf = dma_alloc_coherent(dev, size, &local_addr, GFP_KERNEL);
		if (!buf) {
			ret = -ENOMEM;
			goto err_map_addr;
		}

		get_random_bytes(buf, size);
		reg->checksum = crc32_le(~0, buf, size);

		if (pci_epc_dma(epc, epf->func_no, local_addr, remote_addr, size,
					    DMA_TO_DEVICE))
			ret = -EIO;

		dma_free_coherent(dev, size, buf, local_addr);
	}

err_map_addr:
	pci_epc_unmap_addr(epc, epf->func_no, remote_addr);

err_addr:
	pci_epc_mem_free_addr(epc, remote_addr, dst_addr, size);

	return ret;
}

static void pci_epf_test_raise_irq(struct pci_epf_test *epf_test, u8 irq)
{
	u8 msi_count;
	struct pci_epf *epf = epf_test->epf;
	struct pci_epc *epc = epf->epc;
	enum pci_barno test_reg_bar = epf_test->test_reg_bar;
	struct pci_epf_test_reg *reg = epf_test->reg[test_reg_bar];

	reg->status |= STATUS_IRQ_RAISED;
	msi_count = pci_epc_get_msi(epc, epf->func_no);
	if (irq > msi_count || msi_count <= 0)
		pci_epc_raise_irq(epc, epf->func_no, PCI_EPC_IRQ_LEGACY, 0);
	else
		pci_epc_raise_irq(epc, epf->func_no, PCI_EPC_IRQ_MSI, irq);
}

static void pci_epf_test_cmd_handler(struct work_struct *work)
{
	int ret;
	u8 irq;
	u8 msi_count;
	u32 command;
	struct pci_epf_test *epf_test = container_of(work, struct pci_epf_test,
						     cmd_handler.work);
	struct pci_epf *epf = epf_test->epf;
	struct pci_epc *epc = epf->epc;
	enum pci_barno test_reg_bar = epf_test->test_reg_bar;
	struct pci_epf_test_reg *reg = epf_test->reg[test_reg_bar];

	command = reg->command;
	if (!command)
		goto reset_handler;

	reg->command = 0;
	reg->status = 0;

	irq = (command & MSI_NUMBER_MASK) >> MSI_NUMBER_SHIFT;

	if (command & COMMAND_RAISE_LEGACY_IRQ) {
		reg->status = STATUS_IRQ_RAISED;
		pci_epc_raise_irq(epc, epf->func_no, PCI_EPC_IRQ_LEGACY, 0);
		goto reset_handler;
	}

	if (command & COMMAND_WRITE) {
		ret = pci_epf_test_write(epf_test, false);
		if (ret)
			reg->status |= STATUS_WRITE_FAIL;
		else
			reg->status |= STATUS_WRITE_SUCCESS;
		pci_epf_test_raise_irq(epf_test, irq);
		goto reset_handler;
	}

	if (command & COMMAND_READ) {
		ret = pci_epf_test_read(epf_test, false);
		if (!ret)
			reg->status |= STATUS_READ_SUCCESS;
		else
			reg->status |= STATUS_READ_FAIL;
		pci_epf_test_raise_irq(epf_test, irq);
		goto reset_handler;
	}

	if (command & COMMAND_COPY) {
		ret = pci_epf_test_copy(epf_test);
		if (!ret)
			reg->status |= STATUS_COPY_SUCCESS;
		else
			reg->status |= STATUS_COPY_FAIL;
		pci_epf_test_raise_irq(epf_test, irq);
		goto reset_handler;
	}

	if (command & COMMAND_DMAWRITE) {
		ret = pci_epf_test_write(epf_test, true);
		if (ret)
			reg->status |= STATUS_WRITE_FAIL;
		else
			reg->status |= STATUS_WRITE_SUCCESS;
		pci_epf_test_raise_irq(epf_test, irq);
		goto reset_handler;
	}

	if (command & COMMAND_DMAREAD) {
		ret = pci_epf_test_read(epf_test, true);
		if (!ret)
			reg->status |= STATUS_READ_SUCCESS;
		else
			reg->status |= STATUS_READ_FAIL;
		pci_epf_test_raise_irq(epf_test, irq);
		goto reset_handler;
	}

	if (command & COMMAND_RAISE_MSI_IRQ) {
		msi_count = pci_epc_get_msi(epc, epf->func_no);
		if (irq > msi_count || msi_count <= 0)
			goto reset_handler;
		reg->status = STATUS_IRQ_RAISED;
		pci_epc_raise_irq(epc, epf->func_no, PCI_EPC_IRQ_MSI, irq);
		goto reset_handler;
	}

reset_handler:
	queue_delayed_work(kpcitest_workqueue, &epf_test->cmd_handler,
			   msecs_to_jiffies(1));
}

static void pci_epf_test_linkup(struct pci_epf *epf)
{
	struct pci_epf_test *epf_test = epf_get_drvdata(epf);

	queue_delayed_work(kpcitest_workqueue, &epf_test->cmd_handler,
			   msecs_to_jiffies(1));
}

static void pci_epf_test_unbind(struct pci_epf *epf)
{
	struct pci_epf_test *epf_test = epf_get_drvdata(epf);
	struct pci_epc *epc = epf->epc;
	struct pci_epf_bar *epf_bar;
	int bar;

	cancel_delayed_work(&epf_test->cmd_handler);
	pci_epc_stop(epc);
	for (bar = BAR_0; bar <= BAR_5; bar++) {
		epf_bar = &epf->bar[bar];

		if (epf_test->reg[bar]) {
			pci_epf_free_space(epf, epf_test->reg[bar], bar);
			pci_epc_clear_bar(epc, epf->func_no, epf_bar);
		}
	}
}

static int pci_epf_test_set_bar(struct pci_epf *epf)
{
	int bar;
	int ret;
	struct pci_epf_bar *epf_bar;
	struct pci_epc *epc = epf->epc;
	struct device *dev = &epf->dev;
	struct pci_epf_test *epf_test = epf_get_drvdata(epf);
	enum pci_barno test_reg_bar = epf_test->test_reg_bar;

	for (bar = BAR_0; bar <= BAR_5; bar++) {
		epf_bar = &epf->bar[bar];
		/* Add function specific bar mask (bar boundary size) value.
		 * e.g. for 4KB mask will be 0xFFF
		 * This can be changed as per function driver requirement.
		 */
		epf_bar->mask = roundup_pow_of_two(epf_bar->size) - 1;
		if (upper_32_bits(epf_bar->size))
			epf_bar->flags |= PCI_BASE_ADDRESS_MEM_TYPE_64;
		else
			epf_bar->flags &= ~PCI_BASE_ADDRESS_MEM_TYPE_MASK;

		ret = pci_epc_set_bar(epc, epf->func_no, epf_bar);
		if (ret) {
			pci_epf_free_space(epf, epf_test->reg[bar], bar);
			dev_err(dev, "failed to set BAR%d\n", bar);
			if (bar == test_reg_bar)
				return ret;
		}
		/*
		 * pci_epc_set_bar() sets PCI_BASE_ADDRESS_MEM_TYPE_64
		 * if the specific implementation required a 64-bit BAR,
		 * even if we only requested a 32-bit BAR.
		 */
		if (epf_bar->flags & PCI_BASE_ADDRESS_MEM_TYPE_64)
			bar++;
	}

	return 0;
}

static int pci_epf_test_alloc_space(struct pci_epf *epf)
{
	struct pci_epf_test *epf_test = epf_get_drvdata(epf);
	struct device *dev = &epf->dev;
	void *base;
	int bar;
	enum pci_barno test_reg_bar = epf_test->test_reg_bar;

	base = pci_epf_alloc_space(epf, sizeof(struct pci_epf_test_reg),
				   test_reg_bar);
	if (!base) {
		dev_err(dev, "failed to allocated register space\n");
		return -ENOMEM;
	}
	epf_test->reg[test_reg_bar] = base;

	for (bar = BAR_0; bar <= BAR_5; bar++) {
		if (bar == test_reg_bar)
			continue;
		base = pci_epf_alloc_space(epf, bar_size[bar], bar);
		if (!base)
			dev_err(dev, "failed to allocate space for BAR%d\n",
				bar);
		epf_test->reg[bar] = base;
	}

	return 0;
}

static int pci_epf_test_bind(struct pci_epf *epf)
{
	int ret;
	struct pci_epf_test *epf_test = epf_get_drvdata(epf);
	struct pci_epf_header *header = epf->header;
	struct pci_epc *epc = epf->epc;
	struct device *dev = &epf->dev;

	if (WARN_ON_ONCE(!epc))
		return -EINVAL;

	ret = pci_epc_write_header(epc, epf->func_no, header);
	if (ret) {
		dev_err(dev, "configuration header write failed\n");
		return ret;
	}

	ret = pci_epf_test_alloc_space(epf);
	if (ret)
		return ret;

	ret = pci_epf_test_set_bar(epf);
	if (ret)
		return ret;

	ret = pci_epc_set_msi(epc, epf->func_no, epf->msi_interrupts);
	if (ret)
		return ret;

	if (!epf_test->linkup_notifier)
		queue_work(kpcitest_workqueue, &epf_test->cmd_handler.work);

	return 0;
}

static const struct pci_epf_device_id pci_epf_test_ids[] = {
	{
		.name = "pci_epf_test",
	},
	{},
};

static int pci_epf_test_probe(struct pci_epf *epf)
{
	struct pci_epf_test *epf_test;
	struct device *dev = &epf->dev;
	const struct pci_epf_device_id *match;
	struct pci_epf_test_data *data;
	enum pci_barno test_reg_bar = BAR_0;
	bool linkup_notifier = true;

	match = pci_epf_match_device(pci_epf_test_ids, epf);
	data = (struct pci_epf_test_data *)match->driver_data;
	if (data) {
		test_reg_bar = data->test_reg_bar;
		linkup_notifier = data->linkup_notifier;
	}

	epf_test = devm_kzalloc(dev, sizeof(*epf_test), GFP_KERNEL);
	if (!epf_test)
		return -ENOMEM;

	epf->header = &test_header;
	epf_test->epf = epf;
	epf_test->test_reg_bar = test_reg_bar;
	epf_test->linkup_notifier = linkup_notifier;

	INIT_DELAYED_WORK(&epf_test->cmd_handler, pci_epf_test_cmd_handler);

	epf_set_drvdata(epf, epf_test);
	return 0;
}

static struct pci_epf_ops ops = {
	.unbind	= pci_epf_test_unbind,
	.bind	= pci_epf_test_bind,
	.linkup = pci_epf_test_linkup,
};

static struct pci_epf_driver test_driver = {
	.driver.name	= "pci_epf_test",
	.probe		= pci_epf_test_probe,
	.id_table	= pci_epf_test_ids,
	.ops		= &ops,
	.owner		= THIS_MODULE,
};

static int __init pci_epf_test_init(void)
{
	int ret;

	kpcitest_workqueue = alloc_workqueue("kpcitest",
					     WQ_MEM_RECLAIM | WQ_HIGHPRI, 0);
	ret = pci_epf_register_driver(&test_driver);
	if (ret) {
		pr_err("failed to register pci epf test driver --> %d\n", ret);
		return ret;
	}

	return 0;
}
module_init(pci_epf_test_init);

static void __exit pci_epf_test_exit(void)
{
	pci_epf_unregister_driver(&test_driver);
}
module_exit(pci_epf_test_exit);

MODULE_DESCRIPTION("PCI EPF TEST DRIVER");
MODULE_AUTHOR("Kishon Vijay Abraham I <kishon@ti.com>");
MODULE_LICENSE("GPL v2");
