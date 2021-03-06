/*
 * board/renesas/silk/silk.c
 *
 * Copyright (C) 2015 Renesas Electronics Corporation
 * Copyright (C) 2015 Cogent Embedded, Inc.
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include <common.h>
#include <malloc.h>
#include <asm/processor.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <asm/arch/rmobile.h>
#include <asm/arch/rcar-mstp.h>
#include <asm/arch/mmc.h>
#include <netdev.h>
#include <miiphy.h>
#include <i2c.h>
#include <div64.h>
#include "qos.h"

DECLARE_GLOBAL_DATA_PTR;

#define CLK2MHZ(clk)	(clk / 1000 / 1000)
void s_init(void)
{
	struct rcar_rwdt *rwdt = (struct rcar_rwdt *)RWDT_BASE;
	struct rcar_swdt *swdt = (struct rcar_swdt *)SWDT_BASE;

	/* Watchdog init */
	writel(0xA5A5A500, &rwdt->rwtcsra);
	writel(0xA5A5A500, &swdt->swtcsra);

	/* QoS */
	qos_init();
}

#define TMU0_MSTP125	(1 << 25)
#define SCIF2_MSTP719	(1 << 19)
#define ETHER_MSTP813	(1 << 13)
#define IIC1_MSTP323	(1 << 23)
#define MMC0_MSTP315	(1 << 15)

int board_early_init_f(void)
{
	/* TMU */
	mstp_clrbits_le32(MSTPSR1, SMSTPCR1, TMU0_MSTP125);

	/* SCIF2 */
	mstp_clrbits_le32(MSTPSR7, SMSTPCR7, SCIF2_MSTP719);

	/* ETHER */
	mstp_clrbits_le32(MSTPSR8, SMSTPCR8, ETHER_MSTP813);

	/* IIC1 / sh-i2c ch1 */
	mstp_clrbits_le32(MSTPSR3, SMSTPCR3, IIC1_MSTP323);

#ifdef CONFIG_SH_MMCIF
	/* MMC */
	mstp_clrbits_le32(MSTPSR3, SMSTPCR3, MMC0_MSTP315);
#endif
	return 0;
}

int board_init(void)
{
	/* adress of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

	/* Init PFC controller */
	r8a7794_pinmux_init();

	/* Ether Enable */
	gpio_request(GPIO_FN_ETH_CRS_DV, NULL);
	gpio_request(GPIO_FN_ETH_RX_ER, NULL);
	gpio_request(GPIO_FN_ETH_RXD0, NULL);
	gpio_request(GPIO_FN_ETH_RXD1, NULL);
	gpio_request(GPIO_FN_ETH_LINK, NULL);
	gpio_request(GPIO_FN_ETH_REFCLK, NULL);
	gpio_request(GPIO_FN_ETH_MDIO, NULL);
	gpio_request(GPIO_FN_ETH_TXD1, NULL);
	gpio_request(GPIO_FN_ETH_TX_EN, NULL);
	gpio_request(GPIO_FN_ETH_MAGIC, NULL);
	gpio_request(GPIO_FN_ETH_TXD0, NULL);
	gpio_request(GPIO_FN_ETH_MDC, NULL);
	gpio_request(GPIO_FN_IRQ8, NULL);

	/* PHY reset */
	gpio_request(GPIO_GP_1_24, NULL);
	gpio_direction_output(GPIO_GP_1_24, 0);
	mdelay(20);
	gpio_set_value(GPIO_GP_1_24, 1);
	udelay(1);

	return 0;
}

#define CXR24 0xEE7003C0 /* MAC address high register */
#define CXR25 0xEE7003C8 /* MAC address low register */
int board_eth_init(bd_t *bis)
{
#ifdef CONFIG_SH_ETHER
	int ret = -ENODEV;
	u32 val;
	unsigned char enetaddr[6];

	ret = sh_eth_initialize(bis);
	if (!eth_getenv_enetaddr("ethaddr", enetaddr))
		return ret;

	/* Set Mac address */
	val = enetaddr[0] << 24 | enetaddr[1] << 16 |
		enetaddr[2] << 8 | enetaddr[3];
	writel(val, CXR24);

	val = enetaddr[4] << 8 | enetaddr[5];
	writel(val, CXR25);

	return ret;
#else
	return 0;
#endif
}

int board_mmc_init(bd_t *bis)
{
	int ret = 0;

#ifdef CONFIG_SH_MMCIF
	/* MMC0 */
	gpio_request(GPIO_GP_4_31, NULL);
	gpio_set_value(GPIO_GP_4_31, 1);

	ret = mmcif_mmc_init();
#endif
	return ret;
}

int dram_init(void)
{
	gd->ram_size = CONFIG_SYS_SDRAM_SIZE;

	return 0;
}

const struct rmobile_sysinfo sysinfo = {
	CONFIG_RMOBILE_BOARD_STRING
};

void reset_cpu(ulong addr)
{
	u8 val;

	i2c_set_bus_num(1); /* PowerIC connected to ch1 */
	i2c_read(CONFIG_SYS_I2C_POWERIC_ADDR, 0x13, 1, &val, 1);
	val |= 0x02;
	i2c_write(CONFIG_SYS_I2C_POWERIC_ADDR, 0x13, 1, &val, 1);
}
