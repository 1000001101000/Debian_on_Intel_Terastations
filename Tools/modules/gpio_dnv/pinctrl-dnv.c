/*
 * Pinctrl GPIO driver for Intel Denverton
 * Copyright (c) 2012-2013, Intel Corporation.
 *
 * IRQ feature added by Copyright (c) 2017 Hiro Sugawara
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/io.h>
#include <linux/pm_runtime.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/sizes.h>

#define DNV_PADBAR		0xc
/* memory mapped register offset index. **DO NOT CHANGE ORDER** */
enum reg_type {
	DNV_PADCFGLOCK,		/* 0x90 */

	DNV_GPI_GPE_EN,		/* 0x160 */
	DNV_GPI_GPE_STS,	/* 0x140 */
	DNV_HOSTSW_OWN,		/* 0xc0 */

	DNV_PAD_OWN,		/* 0x2c */

	DNV_PAD_CFG_DW0,	/* 0x0 */
	DNV_PAD_CFG_DW1,	/* 0x4 */

	DNV_MMIO_REG_END
};

/* DNV_PAD_CFG_DW0 register bits */
#define DNV_RXEVCFG_MASK	(BIT(26)|BIT(25))
#define DNV_RXEVCFG_TRIG_LVL	(0 << 25)
#define DNV_RXEVCFG_TRIG_EDGE	(1 << 25)
#define DNV_RXEVCFG_DRIVE0	(2 << 25)

#define DNV_RXINV		BIT(23)
#define DNV_GPIROUTSCI		BIT(19)
#define DNV_GPIROUT_MASK	(BIT(17)|BIT(18)|BIT(19)|BIT(20))
#define DNV_PMODE1		BIT(11)
#define DNV_PMODE0		BIT(10)
#define DNV_INPUT_DIS		BIT(9)  /* 0: input enabled (active low)*/
#define DNV_OUTPUT_DIS		BIT(8)  /* 0: output enabled (active low)*/
#define DNV_RXSTATE		BIT(1)
#define DNV_LEVEL		BIT(0)

/* Restore */
#define DNV_DIR_MASK		(BIT(8) | BIT(9))
#define DNV_CONF0_RESTORE_MASK	\
		(DNV_LEVEL | DNV_DIR_MASK | DNV_PMODE0 | DNV_PMODE1	\
			| DNV_GPIROUTSCI | DNV_RXINV | DNV_RXEVCFG_MASK	\
			| BIT(28) | BIT(29) | BIT(30)	\
			| BIT(31))
#define DNV_CONF1_RESTORE_MASK	(BIT(10) | BIT(11) | BIT(12) | BIT(13))

static int reg_map_offset_dnv(enum reg_type reg, unsigned int gpi)
{
	int offset;

	static const int padcfglock_map_dnv[] = {
		0x0,	/*  0: PADCFGLOCK_NORTH_ALL_0 */
		0x8,	/*  1: PADCFGLOCK_NORTH_ALL_1 */
		0x8,	/*  2: PADCFGLOCK_NORTH_ALL_1 */
		0x20,	/*  3: PADCFGLOCK_SOUTH_GROUP1_1 */
		0x8,	/*  4: PADCFGLOCK_SOUTH_GROUP0_0 */
		0x8,	/*  5: PADCFGLOCK_SOUTH_GROUP0_0 */
		0x8,	/*  6: PADCFGLOCK_SOUTH_GROUP0_0 */
		0x8,	/*  7: PADCFGLOCK_SOUTH_GROUP0_0 */
		0x10,	/*  8: PADCFGLOCK_SOUTH_GROUP0_1 */
		0x10,	/*  9: PADCFGLOCK_SOUTH_GROUP0_1 */
		0x18,	/* 10: PADCFGLOCK_SOUTH_GROUP1_0 */
		0x18,	/* 11: PADCFGLOCK_SOUTH_GROUP1_0 */
		0x8,	/* 12: PADCFGLOCK_SOUTH_GROUP0_0 */
	};

	static const int gpi_gpe_map_dnv[] = {
		0x0,	/*  0: *_NORTH_ALL_0 */
		0x4,	/*  1: *_NORTH_ALL_1 */
		0x4,	/*  2: *_NORTH_ALL_1 */
		0x10,	/*  3: *_SOUTH_GROUP1_1 */
		0x4,	/*  4: *_SOUTH_GROUP0_0 */
		0x4,	/*  5: *_SOUTH_GROUP0_0 */
		0x4,	/*  6: *_SOUTH_GROUP0_0 */
		0x4,	/*  7: *_SOUTH_GROUP0_0 */
		0x8,	/*  8: *_SOUTH_GROUP0_1 */
		0x8,	/*  9: *_SOUTH_GROUP0_1 */
		0xc,	/* 10: *_SOUTH_GROUP1_0 */
		0xc,	/* 11: *_SOUTH_GROUP1_0 */
		0x4,	/* 12: *_SOUTH_GROUP0_0 */
	};

	static const int pad_own_map_dnv[] = {
		0x0,	/*  0: PAD_OWN_NORTH_ALL_3 */
		0x4,	/*  1: PAD_OWN_NORTH_ALL_4 */
		0x4,	/*  2: PAD_OWN_NORTH_ALL_4 */
		0x30,	/*  3: PAD_OWN_SOUTH_GROUP1_5 */
		0xc,	/*  4: PAD_OWN_SOUTH_GROUP0_3 */
		0xc,	/*  5: PAD_OWN_SOUTH_GROUP0_3 */
		0xc,	/*  6: PAD_OWN_SOUTH_GROUP0_3 */
		0xc,	/*  7: PAD_OWN_SOUTH_GROUP0_3 */
		0x10,	/*  8: PAD_OWN_SOUTH_GROUP0_4 */
		0x14,	/*  9: PAD_OWN_SOUTH_GROUP0_5 */
		0x1c,	/* 10: PAD_OWN_SOUTH_GROUP1_3 */
		0x1c,	/* 11: PAD_OWN_SOUTH_GROUP1_3 */
		0x0,	/* 12: PAD_OWN_SOUTH_GROUP0_0 */
	};

	/* PAD_CFG_DW0_GPIO_0 ... PAD_CFG_DW0_GPIO_12
	 * #13 and up are multiplexed. Refer to Table 27-6.
	 */
	static const unsigned int pad_cfg_dw_map_dnv[] = {
		0xd8,  0x108, 0x110, 0x380, 0x168, 0x170, 0x178, 0x180,
		0x1c8, 0x1d0, 0x310, 0x318, 0x90,  0x98,  0x0,   0x8,
		0x10,  0x18,  0x20,  0x28,  0x30,  0x38,  0x40,  0x48,
		0x50,  0x58,  0x60,  0x68,  0x70,  0x78,  0x80,  0x88,
		0x90,  0x98,  0xa0,  0xa8,  0xb0,  0xb8,  0xc0,  0xc8,
		0xd0,  0xe0,  0xe8,  0xf0,  0xf8,  0x100, -1,    0x118,
		0x120, 0x128, 0x130, 0x138, 0x140, 0x0,   0x8,   0x10,
		0x18,  0x20,  0x28,  0x30,  0x38,  0x40,  0x48,  0x50,

		0x2d0, 0x2d8, 0x2e0, 0x2e8, 0x2f0, 0x2f8, 0x300, 0x308,
		0x290, 0x298, 0x2a0, 0x2a8, 0x2b0, 0x2b8, 0x2c0, 0x238,
		0x240, 0x248, 0x250, 0x258, 0x260, 0x268, 0x270, 0x278,
		0x280, 0x288, 0x188, 0x190, 0x198, 0x1a0, 0x1a8, 0x1b0,
		0x1b8, 0x1c0, 0xa0,  0xa8,  0xb0,  0xb8,  0xc0,  0xc8,
		0xd0,  0xd8,  0xe0,  0xe8,  0xf0,  0xf8,  0x100, 0x108,
		0x110, 0x118, 0x120, 0x128, 0x130, 0x138, 0x140, 0x148,
		0x150, 0x158, 0x160, 0x328, 0x330, 0x338, 0x340, 0x348,

		0x350, 0x358, 0x360, 0x368, 0x370, 0x378, 0x58,  0x60,
		0x68,  0x70,  0x78,  0x80,  0x88,  -1,    -1,    -1,
		-1,    -1,    0x200, 0x208, 0x210, 0x218, 0x220, 0x228,
		0x230
	};

	switch (reg) {
	case DNV_PADCFGLOCK:
		offset = padcfglock_map_dnv[gpi];
		break;
	case DNV_GPI_GPE_EN:
	case DNV_GPI_GPE_STS:
	case DNV_HOSTSW_OWN:
		offset = gpi_gpe_map_dnv[gpi];
		break;
	case DNV_PAD_OWN:
		offset = pad_own_map_dnv[gpi];
		break;
	case DNV_PAD_CFG_DW0:
	case DNV_PAD_CFG_DW1:
		offset = pad_cfg_dw_map_dnv[gpi];
		break;
	default:
		return -EINVAL;
	}

	if (offset < 0)
		return -EINVAL;

	switch (gpi) {
	/* C2 */
	case 0 ... 2:
	case 14 ... 45:
	case 47 ... 52:
		return offset;
	/* C5 */
	case 3 ... 13:
	case 53 ... 140:
	case 146 ... 152:
		return offset + 0x30000;
	}
	return -EINVAL;
}

static int reg_bit_shift_dnv(enum reg_type reg, unsigned int gpi)
{
	static const int gpe_bit[] =
		{ 27, 1, 2, 9, 27, 28, 29, 30,  7, 8, 27, 28, 0 };
	static const int pad_own_bit[] =
		{ 12, 4, 8, 4, 12, 16, 20, 24, 28, 0, 12, 16, 12 };

	switch (reg) {
	case DNV_PADCFGLOCK:
	case DNV_GPI_GPE_EN:
	case DNV_GPI_GPE_STS:
	case DNV_HOSTSW_OWN:
		if (gpi < ARRAY_SIZE(gpe_bit))
			return gpe_bit[gpi];
		break;
	case DNV_PAD_OWN:
		if (gpi < ARRAY_SIZE(pad_own_bit))
			return pad_own_bit[gpi];
		break;
	default:
		break;
	}
	return -EINVAL;
}

static void dbg_show_dnv(struct seq_file *s, unsigned int gpi,
				unsigned int gbase, unsigned int reg_map_base,
				const char *label, u32 dw0, u32 dw1)
{
	unsigned long addr = reg_map_offset_dnv(DNV_PAD_CFG_DW0, gpi) +
				reg_map_base + 0xc20000;

	seq_printf(s,
		" gpio-%-3d (%-20.20s) pad-%2lX_%03lx conf0:%08x conf1:%08x\n",
		gpi + gbase, label, addr >> 16, addr & 0xfff, dw0, dw1);
}
#if 0
static int gpio_num_adj_apl(unsigned int gpi)
{
	switch (gpi) {
	case 0 ... 49:
		return gpi;
		break;
	case 62 ... 73:
		return gpi - 12;
		break;
	}
	return -EINVAL;
}

static int reg_map_offset_apl(unsigned int reg, unsigned int gpi)
{
	int shift = gpio_num_adj_apl(gpi);;

	return (shift < 0) ? shift : (shift/32) * 4;
}

static int reg_bit_shift_apl(unsigned int reg, unsigned int gpi)
{
	int shift = gpio_num_adj_apl(gpi);;

	if (shift < 0)
		return shift;
	if (reg == DNV_PAD_OWN)
		shift *= 2;
	return shift % 32;
}

static void dbg_show_apl(struct seq_file *s, unsigned int gpi,
				unsigned int gbase, unsigned int reg_map_base,
				const char *label, u32 dw0, u32 dw1)
{
}
#endif
struct chip_priv {
	unsigned int irq_type;
};

static struct chip_info {
	const char		*name, *gpio_label;
	const unsigned int	padbar;
	const u16	ngpios, npads;
	const u16	pci_id;
	const unsigned int	reg_map_base[DNV_MMIO_REG_END];
	int (*reg_map_offset)(enum reg_type, unsigned int);
	int (*reg_bit_shift)(enum reg_type, unsigned int);
	void (*dbg_show)(struct seq_file *s, unsigned int gpi,
				unsigned int gpio_base,
				unsigned int reg_map_base,
				const char *label, u32 dw0, u32 dw1);
} chip_info[] = {
	{
		.name		= "Denverton",
		.gpio_label	= "gpio_dnv",
		.padbar		= 0x400,
		.ngpios		= 153,
		.npads		= 13,
		.reg_map_base = {0x90, 0x160, 0x140, 0xc0, 0x2c, 0x400, 0x404},
		.reg_map_offset	= reg_map_offset_dnv,
		.reg_bit_shift	= reg_bit_shift_dnv,
		.dbg_show	= dbg_show_dnv,
	},
	{}
};

struct dnv_gpio_pin_context {
	u32 conf0;
	u32 conf1;
};

struct dnv_gpio {
	struct gpio_chip		chip;
	struct platform_device		*pdev;
	raw_spinlock_t			lock;
	void __iomem			*reg_base;
	struct pinctrl_gpio_range	range;
	struct dnv_gpio_pin_context	*saved_context;
	const struct chip_info		*chip_info;
	struct chip_priv		*chip_priv;
};

#define to_dnv_gpio(c)	container_of(c, struct dnv_gpio, chip)

static void __iomem *dnv_gpio_reg(struct gpio_chip *gc, unsigned int gpi,
					enum reg_type reg_type)
{
	struct dnv_gpio *vg = to_dnv_gpio(gc);
	const struct chip_info *chip_info = vg->chip_info;
	u32 gpio_limit = chip_info->npads;
	long offset;

	if (reg_type >= DNV_MMIO_REG_END)
		return NULL;

	offset = chip_info->reg_map_offset(reg_type, gpi);
	if (offset < 0)
		return NULL;
	offset += chip_info->reg_map_base[reg_type];

	if (reg_type == DNV_PAD_CFG_DW0 || reg_type == DNV_PAD_CFG_DW1)
		gpio_limit = gc->ngpio;

	if (gpi >= gpio_limit) {
		WARN(1, "Bad gpio_num=%u\n", gpi);
		return NULL;
	}

	return vg->reg_base + offset;
}

static u32 dnv_gpio_mask(struct gpio_chip *gc, u32 gpi, enum reg_type reg)
{
	struct dnv_gpio *vg = to_dnv_gpio(gc);
	int shift = vg->chip_info->reg_bit_shift(reg, gpi);

	if (gpi >= chip_info->npads || shift < 0)
		return 0;

	switch (reg) {
	case DNV_PADCFGLOCK:
	case DNV_GPI_GPE_EN:
	case DNV_GPI_GPE_STS:
	case DNV_HOSTSW_OWN:
		return 1 << shift;
		break;
	case DNV_PAD_OWN:
		return 3 << shift;
		break;
	default:
		break;
	}
	return 0;
}

static void dnv_gpio_clear_triggering(struct dnv_gpio *vg, unsigned int gpi)
{
	void __iomem *reg = dnv_gpio_reg(&vg->chip, gpi, DNV_PAD_CFG_DW0);
	u32 val, mask = dnv_gpio_mask(&vg->chip, gpi, DNV_GPI_GPE_EN);
	unsigned long flags;

	if (!reg || !mask)
		goto err2;

	raw_spin_lock_irqsave(&vg->lock, flags);
	val = readl(reg);
	writel(val & ~DNV_GPIROUTSCI, reg);

	/* EN */
	reg = dnv_gpio_reg(&vg->chip, gpi, DNV_GPI_GPE_EN);
	if (!reg)
		goto err;
	val = readl(reg);
	writel(val & ~mask, reg);

	/* STS */
	reg = dnv_gpio_reg(&vg->chip, gpi, DNV_GPI_GPE_STS);
	if (!reg)
		goto err;
	val = readl(reg);
	writel(val | mask, reg);
err:
	raw_spin_unlock_irqrestore(&vg->lock, flags);
err2:
	WARN(!reg || !mask, "Invalid gpio_no=%u\n", gpi);
}

static int dnv_gpio_request(struct gpio_chip *gc, unsigned int gpi)
{
	struct dnv_gpio *vg = to_dnv_gpio(gc);
	void __iomem *reg = dnv_gpio_reg(gc, gpi, DNV_PAD_CFG_DW0);
	u32 val;
	unsigned long flags;

	if (!reg)
		return -EINVAL;

	raw_spin_lock_irqsave(&vg->lock, flags);

	/*
	 * In most cases, func pin mux 000 means GPIO function.
	 * But, some pins may have func pin mux 001 represents
	 * GPIO function.
	 *
	 * Because there are devices out there where some pins were not
	 * configured correctly we allow changing the mux value from
	 * request (but print out warning about that).
	 */
	val = readl(reg) & DNV_PMODE0;
	if (WARN_ON(0 != val)) {
		val = readl(reg) & ~DNV_PMODE0;
		writel(val, reg);

		dev_warn(&vg->pdev->dev,
			"pin %u forcibly re-configured as GPIO\n", gpi);
	}

	raw_spin_unlock_irqrestore(&vg->lock, flags);

	pm_runtime_get(&vg->pdev->dev);

	return 0;
}

static void dnv_gpio_free(struct gpio_chip *gc, unsigned int gpi)
{
	struct dnv_gpio *vg = to_dnv_gpio(gc);

	dnv_gpio_clear_triggering(vg, gpi);
	pm_runtime_put(&vg->pdev->dev);
}

#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0)
static inline struct irq_desc *irq_data_to_desc(struct irq_data *data)
{
	return container_of(data, struct irq_desc, irq_data);
}
/**
 * irq_set_handler_locked - Set irq handler from a locked region
 * @data:       Pointer to the irq_data structure which identifies the irq
 * @handler:    Flow control handler function for this interrupt
 *
 * Sets the handler in the irq descriptor associated to @data.
 *
 * Must be called with irq_desc locked and valid parameters. Typical
 * call site is the irq_set_type() callback.
 */
static inline void irq_set_handler_locked(struct irq_data *data,
                                          irq_flow_handler_t handler)
{
	struct irq_desc *desc = irq_data_to_desc(data);

	desc->handle_irq = handler;
}
#endif

static int dnv_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct dnv_gpio *vg = to_dnv_gpio(irq_data_get_irq_chip_data(d));
	struct gpio_chip *gc = &vg->chip;
	u32 gpi = irqd_to_hwirq(d);
	void __iomem *cfgreg = dnv_gpio_reg(gc, gpi, DNV_PAD_CFG_DW0);
	void __iomem *pownreg = dnv_gpio_reg(gc, gpi, DNV_PAD_OWN);
	void __iomem *hownreg = dnv_gpio_reg(gc, gpi, DNV_HOSTSW_OWN);
	void __iomem *enreg = dnv_gpio_reg(gc, gpi, DNV_GPI_GPE_EN);
	u32 enmask = dnv_gpio_mask(gc, gpi, DNV_GPI_GPE_EN);
	u32 pownmask = dnv_gpio_mask(gc, gpi, DNV_PAD_OWN);
	u32 hownmask = dnv_gpio_mask(gc, gpi, DNV_HOSTSW_OWN);
	u32 pad_cfg_dw0, gpe_en, pad_own, hostsw_own;
	unsigned long flags;
	int err = 0;

	if (!cfgreg || !enreg || !pownreg || !enmask || !hownmask || !pownmask)
		return -EINVAL;

	raw_spin_lock_irqsave(&vg->lock, flags);

	pad_own = readl(pownreg) & ~pownmask;
	writel(pad_own, pownreg);
	hostsw_own = readl(hownreg) & ~hownmask;
	writel(hostsw_own, hownreg);

	pad_cfg_dw0 = readl(cfgreg) & ~(DNV_GPIROUT_MASK | DNV_RXEVCFG_MASK);
	gpe_en = readl(enreg);

	if (type & IRQ_TYPE_SENSE_MASK) {
		pad_cfg_dw0 |= DNV_GPIROUTSCI;
		switch (type) {
		case IRQ_TYPE_EDGE_RISING:
			pad_cfg_dw0 |= DNV_RXEVCFG_TRIG_EDGE;
			pad_cfg_dw0 &= ~DNV_RXINV;
			break;
		case IRQ_TYPE_EDGE_FALLING:
			pad_cfg_dw0 |= DNV_RXEVCFG_TRIG_EDGE;
			pad_cfg_dw0 |= DNV_RXINV;
			break;
		case IRQ_TYPE_EDGE_BOTH:
			pad_cfg_dw0 |= DNV_RXEVCFG_TRIG_EDGE;
			pad_cfg_dw0 ^= DNV_RXINV;
			break;
		case IRQ_TYPE_LEVEL_HIGH:
			pad_cfg_dw0 |= DNV_RXEVCFG_TRIG_LVL;
			pad_cfg_dw0 &= ~DNV_RXINV;
			break;
		case IRQ_TYPE_LEVEL_LOW:
			pad_cfg_dw0 |= DNV_RXEVCFG_TRIG_LVL;
			pad_cfg_dw0 |= DNV_RXINV;
			break;
		default:
			err = -EINVAL;
			goto exit;
			break;
		}
		gpe_en |= enmask;
	} else
		gpe_en &= ~enmask;
	writel(pad_cfg_dw0, cfgreg);
	writel(gpe_en, enreg);

	vg->chip_priv[gpi].irq_type = type;
exit:
	raw_spin_unlock_irqrestore(&vg->lock, flags);
	return err;
}

static int dnv_gpio_get(struct gpio_chip *gc, unsigned int gpi)
{
	void __iomem *reg = dnv_gpio_reg(gc, gpi, DNV_PAD_CFG_DW0);
	u32 val;

        if (!reg)
                return -EINVAL;

	val = readl(reg);
	return !!((DNV_INPUT_DIS & val) ? (val & DNV_LEVEL)
					: (val & DNV_RXSTATE));
}

static void dnv_gpio_set(struct gpio_chip *gc, unsigned int gpi, int val)
{
	struct dnv_gpio *vg = to_dnv_gpio(gc);
	void __iomem *reg = dnv_gpio_reg(gc, gpi, DNV_PAD_CFG_DW0);
	unsigned long flags;
	u32 old_val;

        if (!reg)
                return;

	raw_spin_lock_irqsave(&vg->lock, flags);

	old_val = readl(reg);

	if (val)
		writel(old_val | DNV_LEVEL, reg);
	else
		writel(old_val & ~DNV_LEVEL, reg);

	raw_spin_unlock_irqrestore(&vg->lock, flags);
}

static int dnv_gpio_direction_input(struct gpio_chip *gc, unsigned int gpi)
{
	struct dnv_gpio *vg = to_dnv_gpio(gc);
	void __iomem *reg = dnv_gpio_reg(gc, gpi, DNV_PAD_CFG_DW0);
	unsigned long flags;
	u32 val;

        if (!reg)
                return -EINVAL;

	raw_spin_lock_irqsave(&vg->lock, flags);

	val = (readl(reg) | DNV_DIR_MASK) & ~DNV_INPUT_DIS;	/* active low */
	writel(val, reg);

	raw_spin_unlock_irqrestore(&vg->lock, flags);

	return 0;
}

static int dnv_gpio_direction_output(struct gpio_chip *gc,
				     unsigned int gpio, int val)
{
	struct dnv_gpio *vg = to_dnv_gpio(gc);
	void __iomem *reg = dnv_gpio_reg(gc, gpio, DNV_PAD_CFG_DW0);
	unsigned long flags;
	u32 reg_val;

        if (!reg)
                return -EINVAL;

	raw_spin_lock_irqsave(&vg->lock, flags);

	reg_val = readl(reg) | DNV_DIR_MASK;
	reg_val &= ~DNV_OUTPUT_DIS;

	if (val)
		writel(reg_val | DNV_LEVEL, reg);
	else
		writel(reg_val & ~DNV_LEVEL, reg);

	raw_spin_unlock_irqrestore(&vg->lock, flags);

	return 0;
}

static void dnv_gpio_dbg_show(struct seq_file *s, struct gpio_chip *gc)
{
	struct dnv_gpio *vg = to_dnv_gpio(gc);
	int gpi;
	u32 conf0, conf1;

	for (gpi = 0; gpi < vg->range.npins; gpi++) {
		const char *label = gpiochip_is_requested(gc, gpi);

		if (!dnv_gpio_reg(gc, gpi, DNV_PAD_CFG_DW0))
			continue;
		if (!label)
			label = "Unrequested";
		conf0 = readl(dnv_gpio_reg(gc, gpi, DNV_PAD_CFG_DW0));
		conf1 = readl(dnv_gpio_reg(gc, gpi, DNV_PAD_CFG_DW1));

		vg->chip_info->dbg_show(s, gpi, gc->base,
				vg->chip_info->reg_map_base[DNV_PAD_CFG_DW0],
				label, conf0, conf1);
	}
}

static int dnv_gpio_irq_chk(struct gpio_chip *gc, unsigned int gpi)
{
	void __iomem *reg = dnv_gpio_reg(gc, gpi, DNV_GPI_GPE_STS);
	u32 sts, bit_mask = dnv_gpio_mask(gc, gpi, DNV_GPI_GPE_STS);

	if (!reg || !bit_mask)
		return 0;
	sts = readl(reg);
	return !!(sts & bit_mask);
}

static irqreturn_t dnv_gpio_irq_handler(int irq, void *dev_id)
{
	struct dnv_gpio *vg = (struct dnv_gpio *)dev_id;
	int ret = 0, gpi;

	/* check from GPIO controller which pin triggered the interrupt */
	for (gpi = 0; gpi < vg->chip_info->npads; gpi++)
		if (vg->chip_priv[gpi].irq_type != IRQ_TYPE_NONE &&
				dnv_gpio_irq_chk(&vg->chip, gpi)) {
			#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0)
			unsigned int virq =
				irq_find_mapping(vg->chip.irq.domain, gpi);
			#else
			unsigned int virq =
				irq_find_mapping(vg->chip.irqdomain, gpi);
			#endif
			generic_handle_irq(virq);
			ret = 1;
		}

	return ret ? IRQ_HANDLED : IRQ_NONE;
}

static void dnv_irq_ack(struct irq_data *d)
{
	struct gpio_chip *gc = irq_data_get_irq_chip_data(d);
	struct dnv_gpio *vg = to_dnv_gpio(gc);
	unsigned int gpi = irqd_to_hwirq(d);
	void __iomem *stsreg = dnv_gpio_reg(gc, gpi, DNV_GPI_GPE_STS);
	u32 sts_mask = dnv_gpio_mask(gc, gpi, DNV_GPI_GPE_STS);

	if (!stsreg || !sts_mask)
		return;

	raw_spin_lock(&vg->lock);
	if (readl(stsreg) & sts_mask) {
		if (vg->chip_priv[gpi].irq_type == IRQ_TYPE_EDGE_BOTH) {
			void __iomem *cfgreg =
				dnv_gpio_reg(gc, gpi, DNV_PAD_CFG_DW0);
			u32 val = readl(cfgreg);

			writel(val ^ DNV_RXINV, cfgreg);
		}
		writel(sts_mask, stsreg);
	}
	raw_spin_unlock(&vg->lock);
}

static void dnv_irq_unmask(struct irq_data *d) { }
static void dnv_irq_mask(struct irq_data *d) { }

static struct irq_chip dnv_irqchip = {
	.name = "DNV-GPIO",
	.irq_ack = dnv_irq_ack,
	.irq_mask = dnv_irq_mask,
	.irq_unmask = dnv_irq_unmask,
	.irq_set_type = dnv_irq_set_type,
	.flags = IRQCHIP_SKIP_SET_WAKE,
};

static void dnv_gpio_irq_init_hw(struct dnv_gpio *vg)
{
	int gpi;

	/*
	 * Clear interrupt triggers for all pins that are GPIOs and
	 * do not use direct IRQ mode. This will prevent spurious
	 * interrupts from misconfigured pins.
	 */
	for (gpi = 0; gpi < vg->chip_info->npads; gpi++) {
		if (vg->range.pins[gpi] < 0)
			continue;
		dnv_gpio_clear_triggering(vg, gpi);
		dev_dbg(&vg->pdev->dev, "disabling GPIO %d\n", gpi);
	}
}

static int dnv_gpio_range_init(struct dnv_gpio *vg)
{
	int gpi;
	const struct chip_info *ci = vg->chip_info;
	unsigned int *pp = devm_kcalloc(&vg->pdev->dev,
					sizeof(vg->range.pins[0]),
					ci->ngpios, GFP_KERNEL);
	if (!pp)
		return -ENOMEM;

	for (gpi = 0; gpi < ci->ngpios; gpi++)
		pp[gpi] = ci->reg_map_offset(gpi, DNV_PAD_CFG_DW0);

	vg->range.name = "0";
	vg->range.npins = ci->ngpios;
	vg->range.pins = (const unsigned int *)pp;

	return 0;
}

static int dnv_gpio_probe(struct platform_device *pdev)
{
	struct dnv_gpio *vg;
	struct gpio_chip *gc;
	struct resource *mem_rc, *irq_rc;
	struct device *dev = &pdev->dev;
	//struct acpi_device *acpi_dev;
	//struct pinctrl_gpio_range *range;
	//acpi_handle handle = ACPI_HANDLE(dev);
	void __iomem	*reg_base;
	int i, ret;
	unsigned int padbar;
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	struct gpio_irq_chip *girq;
	#endif

	//if (acpi_bus_get_device(handle, &acpi_dev))
	//	return -ENODEV;

	mem_rc = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_base = devm_ioremap_resource(dev, mem_rc);
	if (IS_ERR(reg_base))
		return PTR_ERR(reg_base);

	vg = devm_kzalloc(dev, sizeof(struct dnv_gpio), GFP_KERNEL);
	if (!vg) {
		dev_err(&pdev->dev, "can't allocate dnv_gpio chip data\n");
		return -ENOMEM;
	}
	vg->pdev = pdev;
	padbar = readl(reg_base + DNV_PADBAR);
	for (i = 0; chip_info[i].name; i++)
		if (chip_info[i].padbar == padbar)
			break;
	if (!chip_info[i].name)
		return -ENODEV;
	vg->chip_info = &chip_info[i];

#if 0

	for (range = dnv_ranges; range->name; range++) {
		vg->chip.ngpio = range->npins;
		vg->range = range;
	}
	if (!vg->chip.ngpio || !vg->range)
		return -ENODEV;
#else
	if ((ret = dnv_gpio_range_init(vg)))
		return ret;

	vg->chip.ngpio = vg->range.npins;
#endif


	platform_set_drvdata(pdev, vg);

	if (!(vg->chip_priv = devm_kcalloc(&pdev->dev, vg->chip_info->npads,
					sizeof(vg->chip_priv[0]), GFP_KERNEL)))
		return -ENOMEM;

	vg->reg_base = reg_base;
	raw_spin_lock_init(&vg->lock);

	gc = &vg->chip;
	gc->label = dev_name(&pdev->dev);
	gc->owner = THIS_MODULE;
	gc->request = dnv_gpio_request;
	gc->free = dnv_gpio_free;
	gc->direction_input = dnv_gpio_direction_input;
	gc->direction_output = dnv_gpio_direction_output;
	gc->get = dnv_gpio_get;
	gc->set = dnv_gpio_set;
	gc->dbg_show = dnv_gpio_dbg_show;
	gc->base = -1;
	gc->can_sleep = false;
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 0)
	gc->parent = dev;
	#else
	gc->dev = dev;
	#endif

#ifdef CONFIG_PM_SLEEP
	vg->saved_context = devm_kcalloc(&pdev->dev, gc->ngpio,
				       sizeof(*vg->saved_context), GFP_KERNEL);
#endif

	ret = gpiochip_add(gc);
	if (ret) {
		dev_err(&pdev->dev, "failed to add dnv-gpio chip\n");
		goto err;
	}

	/* set up interrupts  */
	irq_rc = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (irq_rc && irq_rc->start) {
		dnv_gpio_irq_init_hw(vg);

		#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
		girq = &gc->irq;
		girq->chip = &dnv_irqchip;
		/* The event comes from the outside so no parent handler */
		girq->parent_handler = NULL;
		girq->num_parents = 0;
		girq->parents = NULL;
		girq->default_type = IRQ_TYPE_NONE;
		girq->handler = handle_simple_irq;

		gpio_irq_chip_set_chip(girq, &dnv_irqchip);
		#else
		ret = gpiochip_irqchip_add(gc, &dnv_irqchip, 0,
					   handle_simple_irq, IRQ_TYPE_NONE);
		#endif
		if (ret) {
			dev_err(dev, "failed to add irqchip\n");
			gpiochip_remove(gc);
			goto err;
		}

		if ((ret = request_irq((unsigned)irq_rc->start,
					dnv_gpio_irq_handler,
					IRQF_SHARED, gc->label, vg))) {
			dev_err(dev, "failed to request parent irq\n");
			gpiochip_remove(gc);
			goto err;
		}
	}

	pm_runtime_enable(dev);
	return 0;
err:
	if (vg->saved_context)
		devm_kfree(dev, vg->saved_context);
	devm_kfree(dev, vg);
	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int dnv_gpio_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct dnv_gpio *vg = platform_get_drvdata(pdev);
	int pin;

        for (pin = 0; pin < vg->chip.ngpio; pin++) {
		void __iomem *reg;
                u32 bit_mask, val;

		bit_mask = dnv_gpio_mask(&vg->chip, pin, DNV_PADCFGLOCK);

                /* LOCK */
                reg = dnv_gpio_reg(&vg->chip, pin, DNV_PADCFGLOCK);

		if (!reg || !bit_mask)
			continue;

                val = readl(reg);
                if (val & bit_mask)	/* Locked */
			continue;

		reg = dnv_gpio_reg(&vg->chip, pin, DNV_PAD_CFG_DW0);
		val = readl(reg) & DNV_CONF0_RESTORE_MASK;
		vg->saved_context[pin].conf0 = val;

		reg = dnv_gpio_reg(&vg->chip, pin, DNV_PAD_CFG_DW1);
		val = readl(reg) & DNV_CONF1_RESTORE_MASK;
		vg->saved_context[pin].conf1 = val;
        }

	return 0;
}

static int dnv_gpio_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct dnv_gpio *vg = platform_get_drvdata(pdev);
	int pin;

        for (pin = 0; pin < vg->chip.ngpio; pin++) {
		void __iomem *reg;
                u32 bit_mask, val;

		bit_mask = dnv_gpio_mask(&vg->chip, pin, DNV_PADCFGLOCK);

		if (!reg || !bit_mask)
			continue;

                /* LOCK */
                reg = dnv_gpio_reg(&vg->chip, pin, DNV_PADCFGLOCK);
                val = readl(reg);
                if (val & bit_mask)	/* Locked */
			continue;

		reg = dnv_gpio_reg(&vg->chip, pin, DNV_PAD_CFG_DW0);
		val = readl(reg);
		if ((val & DNV_CONF0_RESTORE_MASK) !=
		     vg->saved_context[pin].conf0) {
			val &= ~DNV_CONF0_RESTORE_MASK;
			val |= vg->saved_context[pin].conf0;
			writel(val, reg);
			dev_info(dev, "restored pin %d conf0 %#08x", pin, val);
		}
		reg = dnv_gpio_reg(&vg->chip, pin, DNV_PAD_CFG_DW1);
		val = readl(reg);
		if ((val & DNV_CONF1_RESTORE_MASK) !=
		     vg->saved_context[pin].conf1) {
			val &= ~DNV_CONF1_RESTORE_MASK;
			val |= vg->saved_context[pin].conf1;
			writel(val, reg);
			dev_info(dev, "restored pin %d conf1 %#08x", pin, val);
		}
        }
	return 0;
}
#endif

#ifdef CONFIG_PM
static int dnv_gpio_runtime_suspend(struct device *dev)
{
	return 0;
}

static int dnv_gpio_runtime_resume(struct device *dev)
{
	return 0;
}
#endif

static const struct dev_pm_ops dnv_gpio_pm_ops = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(dnv_gpio_suspend, dnv_gpio_resume)
	SET_RUNTIME_PM_OPS(dnv_gpio_runtime_suspend, dnv_gpio_runtime_resume,
			   NULL)
};

static const struct acpi_device_id dnv_gpio_acpi_match[] = {
	//{ "INT33B2", 0 },
	//{ "INT33FC", 0 },
	{ "INTC3000", 0},
	{ }
};
MODULE_DEVICE_TABLE(acpi, dnv_gpio_acpi_match);

static int dnv_gpio_remove(struct platform_device *pdev)
{
	struct dnv_gpio *vg = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);
	gpiochip_remove(&vg->chip);

	return 0;
}

static struct platform_driver dnv_gpio_driver = {
	.probe          = dnv_gpio_probe,
	.remove         = dnv_gpio_remove,
	.driver         = {
		.name   = "gpio_dnv",
		.pm	= &dnv_gpio_pm_ops,
		.owner	= THIS_MODULE,
		.acpi_match_table = dnv_gpio_acpi_match,
	},
};

static struct resource dnv_gpio_resources[] = {
	{
		.name   = "gpio_dnv",
		.start  = 0xfdc20000,
		.end    = 0xfdc20000 + SZ_256K - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.name   = "gpio_dnv_int",
		.start  = 9,
		.flags  = IORESOURCE_IRQ,
	},
};

static const struct platform_device_info pi __initconst = {
        .name           = "gpio_dnv",
        .id             = 0,
        .res            = dnv_gpio_resources,
        .num_res        = ARRAY_SIZE(dnv_gpio_resources),
};

struct platform_device *pdev;
static int __init dnv_gpio_init(void)
{

        pdev = platform_device_register_full(&pi);
        if (IS_ERR(pdev)) {
                return PTR_ERR(pdev);
        }

	return platform_driver_register(&dnv_gpio_driver);
}
subsys_initcall(dnv_gpio_init);

static void __exit dnv_gpio_exit(void)
{
	platform_device_unregister(pdev);
	platform_driver_unregister(&dnv_gpio_driver);
}
module_exit(dnv_gpio_exit);

MODULE_AUTHOR("LiBo <bo_li@usish.com>");
MODULE_DESCRIPTION("Intel Denverton pinctrl/GPIO core driver");
MODULE_LICENSE("GPL v2");
