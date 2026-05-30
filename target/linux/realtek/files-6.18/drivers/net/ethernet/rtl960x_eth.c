// SPDX-License-Identifier: GPL-2.0-only

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_net.h>
#include <linux/delay.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/crc32.h>
#include <linux/if_vlan.h>

#include "rtl960x_eth.h"

/*
 * Internal PHY power-on via GPHY indirect access
 * TODO Move all these to MDIO
 * The RTL9607C has internal GbE PHYs for ports 0-3 (and potentially 4-5).
 * These need to be powered on by clearing the power-down bit (bit 11)
 * in the BMCR register (MII register 0x00) via the GPHY indirect
 * access registers in the switchcore.
 *
 * Reference: _dal_rtl9607c_switch_phyPower_set()
 *   Read CMD:  0x20a400 | (port << 16) = read reg 0x00, page 0xa4
 *   Write CMD: 0x60a400 | (port << 16) = write reg 0x00, page 0xa4
 *   bit 11 of data = power down (clear to enable)
 */

static int rteth_gphy_wait_ready(struct rteth_priv *priv)
{
	u32 rd_val;
	int timeout = 1000;

	while (timeout--) {
		regmap_read(priv->swcore, SWCORE_GPHY_IND_RD, &rd_val);
		if (!(rd_val & BIT(16))) /* BUSY bit in RD register */
			return 0;
		udelay(10);
	}
	return -ETIMEDOUT;
}

static void rteth_phy_power_on(struct rteth_priv *priv)
{
	u32 rdata, wdata, cmd;
	int port;

	if (!priv->swcore)
		return;

	/*
	 * Set PATCH_PHY_DONE bit (bit 0 of WRAP_GPHY_MISC register at offset 0x114).
	 * This is required by the RTL9607C hardware to indicate that PHY patching
	 * is complete and the GPHY subsystem can operate normally.
	 * Without this, the MDIO controller cannot access internal PHYs.
	 * Reference: dal_rtl9607c_switch_init() sets this before PHY access.
	 */
	regmap_update_bits(priv->swcore, SWCORE_WRAP_GPHY_MISC, BIT(0), BIT(0));
	dev_info(&priv->pdev->dev, "WRAP_GPHY_MISC: PATCH_PHY_DONE set\n");

	/* Small delay after setting PATCH_PHY_DONE */
	msleep(10);

	for (port = 0; port <= 4; port++) {
		/* Issue read command for PHY register 0 (BMCR) on page 0xa4 */
		cmd = 0x20a400 | (port << 16);
		regmap_write(priv->swcore, SWCORE_GPHY_IND_CMD, cmd);

		if (rteth_gphy_wait_ready(priv)) {
			dev_warn(&priv->pdev->dev,
				 "GPHY indirect read timeout for port %d\n", port);
			continue;
		}

		/* Read the data (bits 15:0 = PHY register value) */
		regmap_read(priv->swcore, SWCORE_GPHY_IND_RD, &rdata);
		rdata &= 0xFFFF;  /* mask to 16 bits */

		dev_info(&priv->pdev->dev,
			 "Port %d BMCR read: 0x%04x (power-down=%d)\n",
			 port, rdata, !!(rdata & BIT(11)));

		/* Clear power-down bit (bit 11) to enable PHY */
		wdata = rdata & ~BIT(11);

		/* Write modified value back */
		regmap_write(priv->swcore, SWCORE_GPHY_IND_WD, wdata);

		/* Issue write command */
		cmd = 0x60a400 | (port << 16);
		regmap_write(priv->swcore, SWCORE_GPHY_IND_CMD, cmd);

		if (rteth_gphy_wait_ready(priv)) {
			dev_warn(&priv->pdev->dev,
				 "GPHY indirect write timeout for port %d\n", port);
			continue;
		}
	}

	/* Wait for PHYs to come out of power-down */
	msleep(100);

	dev_info(&priv->pdev->dev, "Internal PHY power-on complete (ports 0-4)\n");
}

/* Basic switch core initialization for CPU port and forwarding */

static void rteth_switch_init(struct rteth_priv *priv)
{
	u32 fwd_mask;
	int port;

	if (!priv->swcore)
		return;

	/*
	 * Enable CPU port (port 9) force mode at 1Gbps full-duplex.
	 * Force ability register: bit1=speed1000, bit2=duplex,
	 * bit4=link, bit5=rx_pause, bit6=tx_pause, bit7=nway
	 */
	regmap_write(priv->swcore, SWCORE_FORCE_P_ABLTY(RTETH_960X_CPU_PORT),
		     BIT(1) | BIT(2) | BIT(4) | BIT(5) | BIT(6) | BIT(7));

	/*
	 * Set port isolation: allow LAN ports (0-3) and WAN port (5)
	 * to reach CPU port (9), and CPU port can reach all ports.
	 *
	 * CPU port (9) forwarding mask: ports 0-3 + port 5
	 * LAN/WAN port forwarding mask: CPU port 9 only
	 */

	/* CPU port 9 can forward to LAN ports 0-3 and WAN port 5 */
	fwd_mask = RTETH_960X_LAN_PORT_MASK | RTETH_960X_WAN_PORT_MASK;
	regmap_write(priv->swcore, SWCORE_PORT_ISO_CTRL(RTETH_960X_CPU_PORT), fwd_mask);

	/* LAN ports 0-3: can forward to CPU port 9 and other LAN ports */
	for (port = 0; port < 4; port++) {
		fwd_mask = BIT(RTETH_960X_CPU_PORT) | BIT(0) | BIT(1) | BIT(2) | BIT(3);
		fwd_mask &= ~BIT(port); /* don't forward to self */
		regmap_write(priv->swcore, SWCORE_PORT_ISO_CTRL(port), fwd_mask);
	}

	/* WAN port 5: can forward to CPU port 9 only (isolated from LAN) */
	regmap_write(priv->swcore, SWCORE_PORT_ISO_CTRL(5), BIT(RTETH_960X_CPU_PORT));

	/* Force LAN ports 0-3 link up at 1Gbps (they have internal PHYs) */
	for (port = 0; port < 4; port++) {
		regmap_write(priv->swcore, SWCORE_FORCE_P_ABLTY(port),
			     BIT(1) | BIT(2) | BIT(4) | BIT(5) | BIT(6) | BIT(7));
	}

	/* Force WAN port 5 link up at 1Gbps */
	regmap_write(priv->swcore, SWCORE_FORCE_P_ABLTY(RTETH_960X_WAN_PORT),
		     BIT(1) | BIT(2) | BIT(4) | BIT(5) | BIT(6) | BIT(7));

	dev_info(&priv->pdev->dev, "Switch core initialized: CPU=%d LAN=0-3 WAN=5\n",
		 RTETH_960X_CPU_PORT);
}

/* Descriptor ring allocation and initialization */

static int rteth_alloc_tx_ring(struct rteth_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	int i;

	priv->tx.desc = dma_alloc_coherent(dev,
					   RTETH_TX_RING_SIZE * sizeof(struct rteth_tx_desc),
					   &priv->tx.desc_dma, GFP_KERNEL);
	if (!priv->tx.desc)
		return -ENOMEM;

	priv->tx.skb = kcalloc(RTETH_TX_RING_SIZE, sizeof(struct sk_buff *), GFP_KERNEL);
	if (!priv->tx.skb) {
		dma_free_coherent(dev,
				  RTETH_TX_RING_SIZE * sizeof(struct rteth_tx_desc),
				  priv->tx.desc, priv->tx.desc_dma);
		return -ENOMEM;
	}

	/* Initialize TX descriptors - all owned by CPU */
	for (i = 0; i < RTETH_TX_RING_SIZE; i++) {
		priv->tx.desc[i].opts1 = 0;
		priv->tx.desc[i].addr = 0;
		priv->tx.desc[i].opts2 = 0;
		priv->tx.desc[i].opts3 = 0;
		priv->tx.desc[i].opts4 = 0;
	}
	/* Set End-of-Ring on last descriptor */
	priv->tx.desc[RTETH_TX_RING_SIZE - 1].opts1 = RTETH_RING_END;

	priv->tx.head = 0;
	priv->tx.tail = 0;

	return 0;
}

static int rteth_alloc_rx_ring(struct rteth_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	struct sk_buff *skb;
	dma_addr_t dma_addr;
	int i;

	priv->rx.desc = dma_alloc_coherent(dev,
					   RTETH_RX_RING_SIZE * sizeof(struct rteth_rx_desc),
					   &priv->rx.desc_dma, GFP_KERNEL);
	if (!priv->rx.desc)
		return -ENOMEM;

	priv->rx.skb = kcalloc(RTETH_RX_RING_SIZE, sizeof(struct sk_buff *), GFP_KERNEL);
	if (!priv->rx.skb) {
		dma_free_coherent(dev,
				  RTETH_RX_RING_SIZE * sizeof(struct rteth_rx_desc),
				  priv->rx.desc, priv->rx.desc_dma);
		return -ENOMEM;
	}

	/* Allocate RX buffers and fill descriptors */
	for (i = 0; i < RTETH_RX_RING_SIZE; i++) {
		skb = netdev_alloc_skb(priv->netdev, RTETH_BUF_SIZE);
		if (!skb) {
			dev_err(dev, "Failed to allocate RX skb %d\n", i);
			goto err_free;
		}
		priv->rx.skb[i] = skb;
		dma_addr = dma_map_single(dev, skb->data, RTETH_BUF_SIZE,
					  DMA_FROM_DEVICE);
		if (dma_mapping_error(dev, dma_addr)) {
			dev_kfree_skb(skb);
			priv->rx.skb[i] = NULL;
			goto err_free;
		}

		priv->rx.desc[i].addr = dma_addr;
		priv->rx.desc[i].opts2 = 0;
		priv->rx.desc[i].opts3 = 0;
		/* Set OWN bit (give to HW) and buffer size */
		if (i == RTETH_RX_RING_SIZE - 1)
			priv->rx.desc[i].opts1 = RTETH_DESC_OWN |
			RTETH_RING_END | (RTETH_BUF_SIZE & 0x1FFFF);
		else
			priv->rx.desc[i].opts1 = RTETH_DESC_OWN |
			(RTETH_BUF_SIZE & 0x1FFFF);
	}

	priv->rx.tail = 0;
	return 0;

err_free:
	for (i--; i >= 0; i--) {
		dma_unmap_single(dev, priv->rx.desc[i].addr,
				 RTETH_BUF_SIZE, DMA_FROM_DEVICE);
		dev_kfree_skb(priv->rx.skb[i]);
	}
	kfree(priv->rx.skb);
	priv->rx.skb = NULL;
	dma_free_coherent(dev,
			  RTETH_RX_RING_SIZE * sizeof(struct rteth_rx_desc),
			  priv->rx.desc, priv->rx.desc_dma);
	priv->rx.desc = NULL;
	priv->rx.desc_dma = 0;
	return -ENOMEM;
}

static void rteth_free_tx_ring(struct rteth_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	int i;

	for (i = 0; i < RTETH_TX_RING_SIZE; i++) {
		if (priv->tx.skb[i]) {
			dma_unmap_single(dev, priv->tx.desc[i].addr,
					 priv->tx.skb[i]->len, DMA_TO_DEVICE);
			dev_kfree_skb(priv->tx.skb[i]);
			priv->tx.skb[i] = NULL;
		}
	}
	kfree(priv->tx.skb);
	dma_free_coherent(dev,
			  RTETH_TX_RING_SIZE * sizeof(struct rteth_tx_desc),
			  priv->tx.desc, priv->tx.desc_dma);
	priv->tx.desc = NULL;
	priv->tx.desc_dma = 0;
}

static void rteth_free_rx_ring(struct rteth_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	int i;

	for (i = 0; i < RTETH_RX_RING_SIZE; i++) {
		if (priv->rx.skb[i]) {
			dma_unmap_single(dev, priv->rx.desc[i].addr,
					 RTETH_BUF_SIZE, DMA_FROM_DEVICE);
			dev_kfree_skb(priv->rx.skb[i]);
			priv->rx.skb[i] = NULL;
		}
	}
	kfree(priv->rx.skb);
	priv->rx.skb = NULL;
	dma_free_coherent(dev,
			  RTETH_RX_RING_SIZE * sizeof(struct rteth_rx_desc),
			  priv->rx.desc, priv->rx.desc_dma);
	priv->rx.desc = NULL;
	priv->rx.desc_dma = 0;
}

/*
 * Hardware initialization
 * Reference: re8670_init_hw() / re8670_start_hw()
 */
static void rteth_hw_stop(struct rteth_priv *priv)
{
	/* Stop DMA engines */
	/* TODO Check if clearing entire io cmd is needed */
	regmap_write(priv->regmap, RTETH_IO_CMD, 0);
	regmap_write(priv->regmap, RTETH_IO_CMD1, 0);

	/* Mask all interrupts and clear pending interrupts */

	regmap_write(priv->regmap, RTETH_INTR, 0xffff0000);

	regmap_write(priv->regmap, RTETH_IMR0, 0);
	regmap_write(priv->regmap, RTETH_ISR1, 0xffffffff);

	udelay(10);
}

static void rteth_hw_setup_rings(struct rteth_priv *priv)
{
	u32 val, ring0_size_msk;
	u16 desc_l, desc_h;

	/* TX ring 1: set base address */
	regmap_write(priv->regmap, RTETH_TX_DESC_ADDR, (u32)priv->tx.desc_dma);
	regmap_clear_bits(priv->regmap, RTETH_TX_CUR_DESC_OFFSET,
			  RTETH_TX_CUR_DESC_MASK);

	/* RX ring 0: set base address and ring size */
	regmap_write(priv->regmap, RTETH_RX_DESC_ADDR1, (u32)priv->rx.desc_dma);
	ring0_size_msk = RTETH_RX_RING_SIZE - 1;
	desc_l = ring0_size_msk & 0xFF;
	desc_h = (ring0_size_msk >> 8) & 0xF;

	/*
	 * EthrntRxCPU_Des_Num register: ring size + flow control thresholds
	 * [31:24] = desc_l (ring size low 8 bits)
	 * [23:16] = TH_ON (flow ctrl assert threshold)
	 * [15:8]  = TH_OFF (flow ctrl de-assert threshold)
	 * [7:4]   = desc_h (ring size high 4 bits)
	 */
	val = FIELD_PREP(RTETH_CPU_DESC_NUM_L, desc_l) |
	      FIELD_PREP(RTETH_DESC_ON_THRESH_L, THRESHOLD_ON_16) |
	      FIELD_PREP(RTETH_DESC_OFF_THRESH_L, THRESHOLD_OFF_16) |
	      FIELD_PREP(RTETH_CPU_DESC_NUM_H, desc_h);

	regmap_write(priv->regmap, RTETH_RX_CPU_DESC_NUM1, val);

	/* RxCDO: current descriptor offset */
	/* This doesn't make any sense according to whats inside the register */
	/* Can we ignore it? */

	regmap_write(priv->regmap, RTETH_RX_DESC_OFFSET_AND_SIZE1, (desc_l << 8) | desc_h);
}

static void rteth_hw_start(struct rteth_priv *priv)
{
	u32 val;

	val = RTETH_MAX_DMA_SEL_L | RTETH_SHORT_DES_FMT | RTETH_EN_EARLY_TX |
	      FIELD_PREP(RTETH_RX_PKT_TMR_H, (RTETH_RX_TIMER_60TU & 0x8) >> 3) |
	      FIELD_PREP(RTETH_TX_FIFO_THRESH, RTETH_TX_THRESH_1024B) |
	      FIELD_PREP(RTETH_TX_INT_TRIG_L, RTETH_TX_INT_4PKTS) |
	      FIELD_PREP(RTETH_RX_PKT_TMR_L, (RTETH_RX_TIMER_60TU & 0x7)) |
	      FIELD_PREP(RTETH_RX_FIFO_THRESH, RTETH_RX_THRESH_64B) |
	      FIELD_PREP(RTETH_RX_INT_TRIG_L, RTETH_RX_INT_4PKTS) |
	      RTETH_RX_ENABLE | RTETH_TX_ENABLE;

	/* Start the DMA engines - IO_CMD1 first, then IO_CMD */
	regmap_set_bits(priv->regmap, RTETH_IO_CMD1,
			FIELD_PREP(RTETH_DESC_FMT_EXTRA, RTETH_APOLLO_DESC_FMT));

	regmap_write(priv->regmap, RTETH_IO_CMD, val);

	dev_info(&priv->pdev->dev, "HW started\n");
}

static void rteth_hw_init(struct rteth_priv *priv)
{
	int i;
	u32 val;

	/* Reset GMAC IP */
	reset_control_assert(priv->gmac_rst);
	mdelay(10);
	reset_control_deassert(priv->gmac_rst);

	/* Enable RX checksum offload and jumbo frame support */
	regmap_set_bits(priv->regmap, RTETH_CMD, RTETH_RX_CHKSUM | RTETH_JUMBO);

	/* TCR: TX configuration - padding disabled, CRC append */
	val = FIELD_PREP(RTETH_INTER_FRAME_GAP_MASK,
				RTETH_INTERFRAMEGAP) | RTETH_GMAC_PADDING;
	regmap_update_bits(priv->regmap, RTETH_TX_CONFIG,
			   RTETH_INTER_FRAME_GAP_MASK | RTETH_GMAC_PADDING,
			   val);

	/* Do we need to clear the register? */
	regmap_write(priv->regmap, RTETH_CPUTAG, 0);

	val = RTETH_CT_EN_RX | FIELD_PREP(RTETH_CT_TSIZE, RTETH_CT_TSIZE_8B) |
	              FIELD_PREP(RTETH_CT_SWITCH, RTETH_CT_SWITCH_APRO) |
	              FIELD_PREP(RTETH_CT_RSIZE_L, RTETH_CT_RSIZE_8B) |
	              FIELD_PREP(RTETH_CT_PROTOCOL_MASK, RTETH_CT_PM_8370) |
	              FIELD_PREP(RTETH_CT_PROTOCOL_VAL, RTETH_CT_PV_8370);
	regmap_write(priv->regmap, RTETH_CPUTAG, val);

	regmap_set_bits(priv->regmap, RTETH_CPUTAG1, RTETH_CT1_SID);

	/* Setup TX/RX DMA rings */
	rteth_hw_setup_rings(priv);

	/* MSR: enable TX/RX flow control, force TX */
	regmap_set_bits(priv->regmap, RTETH_MII_STS,
			RTETH_FORCE_TX | RTETH_RX_FLOW | RTETH_TX_FLOW);

	regmap_set_bits(priv->regmap, RTETH_CONFIG,
			FIELD_PREP(RTETH_RFIFO_SIZE, RTETH_RFIFO_SIZE_2K) |
			RTETH_EN_INTR_SPLIT | RTETH_RX_SIDEBAND);
	/*
	 * Set RX ring routing (from set_rring_route).
	 * Value 0x65432111 routes different priority packets to ring 0.
	 * This is needed even with single ring to prevent RX stalls.
	 */
	val = FIELD_PREP(RTETH_PRI_7_ROUTE, 6) | FIELD_PREP(RTETH_PRI_6_ROUTE, 5) |
	      FIELD_PREP(RTETH_PRI_5_ROUTE, 4) | FIELD_PREP(RTETH_PRI_4_ROUTE, 3) |
	      FIELD_PREP(RTETH_PRI_3_ROUTE, 2) | FIELD_PREP(RTETH_PRI_2_ROUTE, 1) |
	      FIELD_PREP(RTETH_PRI_1_ROUTE, 1) | FIELD_PREP(RTETH_PRI_0_ROUTE, 1);

	for (i = 0; i < 7; i++)
		regmap_write(priv->regmap, RTETH_RX_RING_ROUTE + (i * 4), val);

	/* Start DMA engines */
	rteth_hw_start(priv);

	/* Some Multicast magic from OEM driver*/
	regmap_write(priv->regmap, RTETH_MAR0, 0xffffffff);
	regmap_write(priv->regmap, RTETH_MAR4, 0xffffffff);

	/* RCR: accept broadcast, multicast, and our MAC */
	regmap_write(priv->regmap, RTETH_RX_CONFIG, RTETH_ACCEPT_BROADCAST |
		     RTETH_ACCEPT_MULTICAST | RTETH_ACCEPT_MYPHYS);

	/* Enable RX interrupts: RX_OK, RDU, RER_RUNT, RER_OVF */
	regmap_set_bits(priv->regmap, RTETH_INTR, RTETH_MSK_RXALL);

	/* IMR0: unmask RX ring 0 interrupts */
	regmap_write(priv->regmap, RTETH_IMR0, RTETH_IMR0_ROK_MSK);
}

/*
 * TX path
 */
static void rteth_tx_reclaim(struct rteth_priv *priv)
{
	struct device *dev = &priv->pdev->dev;
	struct rteth_tx_desc *txd;

	while (priv->tx.tail != priv->tx.head) {
		txd = &priv->tx.desc[priv->tx.tail];

		/* If HW still owns this descriptor, stop */
		if (txd->opts1 & RTETH_DESC_OWN)
			break;

		if (priv->tx.skb[priv->tx.tail]) {
			dma_unmap_single(dev, txd->addr,
					 priv->tx.skb[priv->tx.tail]->len,
		    DMA_TO_DEVICE);
			dev_kfree_skb_any(priv->tx.skb[priv->tx.tail]);
			priv->tx.skb[priv->tx.tail] = NULL;
		}

		priv->tx.tail = (priv->tx.tail + 1) % RTETH_TX_RING_SIZE;
	}
}

static netdev_tx_t rteth_start_xmit(struct sk_buff *skb,
				       struct net_device *dev)
{
	struct rteth_priv *priv = netdev_priv(dev);
	struct rteth_tx_desc *txd;
	dma_addr_t dma_addr;
	u32 opts1;
	int entry;

	// TMP until IRQ fix
	napi_schedule(&priv->napi);

	spin_lock_bh(&priv->tx_lock);

	/* Reclaim completed TX buffers */
	rteth_tx_reclaim(priv);

	/* Check if ring is full */
	entry = priv->tx.head;
	if (((entry + 1) % RTETH_TX_RING_SIZE) == priv->tx.tail) {
		netif_stop_queue(dev);
		spin_unlock_bh(&priv->tx_lock);
		return NETDEV_TX_BUSY;
	}

	/* Pad short frames */
	if (skb->len < RTETH_MIN_PKT_LEN) {
		if (skb_padto(skb, RTETH_MIN_PKT_LEN)) {
			dev->stats.tx_dropped++;
			spin_unlock_bh(&priv->tx_lock);
			return NETDEV_TX_OK;
		}
		skb->len = RTETH_MIN_PKT_LEN;
	}

	/* Map the buffer for DMA */
	dma_addr = dma_map_single(&priv->pdev->dev, skb->data, skb->len,
				  DMA_TO_DEVICE);
	if (dma_mapping_error(&priv->pdev->dev, dma_addr)) {
		dev_kfree_skb(skb);
		dev->stats.tx_dropped++;
		spin_unlock_bh(&priv->tx_lock);
		return NETDEV_TX_OK;
	}

	txd = &priv->tx.desc[entry];
	priv->tx.skb[entry] = skb;

	/* Set buffer address */
	txd->addr = dma_addr;

	txd->opts2 = RTETH_TX_CPUTAG_GEN;
	txd->opts3 = 0;

	/* Set opts1 last (contains OWN bit) - memory barrier before */
	wmb();
	opts1 = RTETH_DESC_OWN | RTETH_FIRST_FRAG | RTETH_LAST_FRAG |
		RTETH_TX_DESC_IPCS | RTETH_TX_DESC_CRC | (skb->len & 0x1FFFF);
	if (entry == RTETH_TX_RING_SIZE - 1)
		opts1 |= RTETH_RING_END;
	txd->opts1 = opts1;

	/* Advance head */
	priv->tx.head = (entry + 1) % RTETH_TX_RING_SIZE;

	/* Kick TX DMA - set TX_POLL bit in IO_CMD */
	wmb();
	regmap_set_bits(priv->regmap, RTETH_IO_CMD, RTETH_TX_FN1);

	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	spin_unlock_bh(&priv->tx_lock);

	return NETDEV_TX_OK;
}

/* RX path */
static int rteth_rx_poll(struct napi_struct *napi, int budget)
{
	struct rteth_priv *priv = container_of(napi, struct rteth_priv, napi);
	struct net_device *dev = priv->netdev;
	struct device *dma_dev = &priv->pdev->dev;
	int work_done = 0;

	while (work_done < budget) {
		struct rteth_rx_desc *rxd;
		struct sk_buff *skb, *new_skb;
		dma_addr_t dma_addr;
		u32 opts1;
		int pkt_len;
		int entry;

		entry = priv->rx.tail;
		rxd = &priv->rx.desc[entry];

		opts1 = rxd->opts1;
		if (opts1 & RTETH_DESC_OWN)
			break;

		/* Validate: first and last segment must be set */
		if (!(opts1 & RTETH_FIRST_FRAG) || !(opts1 & RTETH_LAST_FRAG)) {
			dev->stats.rx_errors++;
			goto refill;
		}

		pkt_len = opts1 & 0x1FFFF;
		if (pkt_len == 0 || pkt_len > RTETH_BUF_SIZE) {
			dev->stats.rx_length_errors++;
			goto refill;
		}

		/* Check for CRC error */
		if (opts1 & RTETH_RX_DESC_CRC_ERR) { /* CRCErr bit */
			dev->stats.rx_crc_errors++;
			goto refill;
		}

		/* Try to allocate a new buffer before consuming this one */
		new_skb = netdev_alloc_skb(dev, RTETH_BUF_SIZE);
		if (unlikely(!new_skb)) {
			dev->stats.rx_dropped++;
			goto refill;
		}

		/* Unmap the current buffer */
		skb = priv->rx.skb[entry];
		dma_unmap_single(dma_dev, rxd->addr,
				 RTETH_BUF_SIZE, DMA_FROM_DEVICE);

		if (opts1 & RTETH_FIRST_FRAG) {
			skb_reserve(skb, 2); // HW DMA start at 4N+2 only in FS
		}
		skb_put(skb, pkt_len);
		skb->protocol = eth_type_trans(skb, dev);
		skb->ip_summed = CHECKSUM_NONE;

		napi_gro_receive(napi, skb);

		dev->stats.rx_packets++;
		dev->stats.rx_bytes += pkt_len;

		/* Install the new buffer */
		priv->rx.skb[entry] = new_skb;
		dma_addr = dma_map_single(dma_dev, new_skb->data,
					  RTETH_BUF_SIZE, DMA_FROM_DEVICE);
		if (dma_mapping_error(dma_dev, dma_addr)) {
			dev_kfree_skb(new_skb);
			priv->rx.skb[entry] = NULL;
			break;
		}
		rxd->addr = dma_addr;

refill:
		/* Give descriptor back to HW */
		wmb();
		if (entry == RTETH_RX_RING_SIZE - 1)
			rxd->opts1 = RTETH_DESC_OWN | RTETH_RING_END |
						  (RTETH_BUF_SIZE & 0x1FFFF);
		else
			rxd->opts1 = RTETH_DESC_OWN |
						  (RTETH_BUF_SIZE & 0x1FFFF);

		priv->rx.tail = (entry + 1) % RTETH_RX_RING_SIZE;
		work_done++;
	}

	if (work_done < budget) {
		napi_complete_done(napi, work_done);
		/* Re-enable RX interrupts */
		regmap_set_bits(priv->regmap, RTETH_INTR, RTETH_MSK_RXALL);
		regmap_set_bits(priv->regmap, RTETH_IMR0, RTETH_IMR0_ROK_MSK);
	}

	return work_done;
}

/* Interrupt handler */
static irqreturn_t rteth_interrupt(int irq, void *dev_id)
{
	struct rteth_priv *priv = dev_id;
	u32 intr_sts, isr1, agg_isr;

	regmap_read(priv->regmap, RTETH_INTR, &intr_sts);
	intr_sts &= 0xffff0000;
	regmap_read(priv->regmap, RTETH_ISR1, &isr1);
	agg_isr = intr_sts | FIELD_PREP(RTETH_STS_ROK, !!(isr1 & RTETH_IMR0_ROK_MSK));
	if (!agg_isr)
		return IRQ_NONE;

	if (agg_isr & RTETH_STS_RXALL) {
		/* Disable RX interrupts and schedule NAPI */
		regmap_clear_bits(priv->regmap, RTETH_INTR, RTETH_MSK_RXALL);
		regmap_clear_bits(priv->regmap, RTETH_IMR0, RTETH_IMR0_ROK_MSK);
		napi_schedule(&priv->napi);
	}

	/* Lets not handle TX interrupts? */
// 	if (intr_sts & RTETH_STS_TOK_TI) {
// 		/* TX done - reclaim buffers */
// 		spin_lock(&priv->tx_lock);
// 		rteth_tx_reclaim(priv);
// 		spin_unlock(&priv->tx_lock);
//
// 		if (netif_queue_stopped(priv->netdev))
// 			netif_wake_queue(priv->netdev);
// 	}

	/* Clear handled interrupts */
	regmap_set_bits(priv->regmap, RTETH_INTR, intr_sts);
	regmap_set_bits(priv->regmap, RTETH_ISR1, isr1);

	return IRQ_HANDLED;
}

/* Net device operations */
static int rteth_open(struct net_device *dev)
{
	struct rteth_priv *priv = netdev_priv(dev);
	int err;

	/* Allocate descriptor rings */
	err = rteth_alloc_tx_ring(priv);
	if (err)
		return err;

	err = rteth_alloc_rx_ring(priv);
	if (err) {
		rteth_free_tx_ring(priv);
		return err;
	}

	/* Power on internal PHYs before switch init */
	rteth_phy_power_on(priv);

	/* Initialize switch core */
	rteth_switch_init(priv);

	/* Request interrupt */
	err = request_irq(priv->irq, rteth_interrupt, IRQF_SHARED,
			  dev->name, priv);
	if (err) {
		dev_err(&priv->pdev->dev, "Failed to request IRQ %d: %d\n",
			priv->irq, err);
		rteth_free_rx_ring(priv);
		rteth_free_tx_ring(priv);
		return err;
	}

	/* Start NAPI */
	napi_enable(&priv->napi);

	/* Initialize and start hardware */
	rteth_hw_init(priv);

	netif_start_queue(dev);

	dev_info(&priv->pdev->dev, "Interface %s opened, IRQ %d\n",
		 dev->name, priv->irq);

	return 0;
}

static int rteth_close(struct net_device *dev)
{
	struct rteth_priv *priv = netdev_priv(dev);

	netif_stop_queue(dev);

	rteth_hw_stop(priv);
	free_irq(priv->irq, priv);
	napi_disable(&priv->napi);

	rteth_free_rx_ring(priv);
	rteth_free_tx_ring(priv);

	dev_info(&priv->pdev->dev, "Interface %s closed\n", dev->name);
	return 0;
}

static void rteth_set_rx_mode(struct net_device *dev)
{
	struct rteth_priv *priv = netdev_priv(dev);
	u32 rcr = RTETH_ACCEPT_MYPHYS;

	if (dev->flags & IFF_PROMISC)
		rcr = RTETH_ACCEPT_MASK;
	else if (dev->flags & IFF_ALLMULTI)
		rcr |= RTETH_ACCEPT_MULTICAST | RTETH_ACCEPT_BROADCAST;
	else
		rcr |= RTETH_ACCEPT_MULTICAST | RTETH_ACCEPT_BROADCAST;

	regmap_write(priv->regmap, RTETH_RX_CONFIG, rcr);
}

static void rteth_set_mac_hw(struct net_device *dev, u8 *mac)
{
	u32 mac_lo = (mac[3] << 24) | (mac[2] << 16) | (mac[1] << 8) | mac[0];
	u32 mac_hi = (mac[5] << 8) | mac[4];
	struct rteth_priv *priv;

	priv = netdev_priv(dev);

	regmap_write(priv->regmap, RTETH_MAC0, mac_lo);
	regmap_write(priv->regmap, RTETH_MAC4, mac_hi);
}

static int rteth_set_mac_address(struct net_device *dev, void *p)
{
	struct sockaddr *addr = p;
	u8 *mac = (u8 *)(addr->sa_data);

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	eth_hw_addr_set(dev, addr->sa_data);
	rteth_set_mac_hw(dev, mac);

	pr_info("Using MAC %pM\n", dev->dev_addr);
	return 0;
}

static void rteth_tx_timeout(struct net_device *dev, unsigned int txqueue)
{
	struct rteth_priv *priv = netdev_priv(dev);
	u32 val;

	dev_err(&priv->pdev->dev, "TX timeout, resetting...\n");

	spin_lock_irq(&priv->tx_lock);
	/* MII Rx and Tx Disable */
	regmap_clear_bits(priv->regmap, RTETH_IO_CMD,
			  RTETH_RX_ENABLE | RTETH_TX_ENABLE);
	rteth_hw_stop(priv);

	/* Reset GMAC IP */
	reset_control_assert(priv->gmac_rst);
	mdelay(10);
	reset_control_deassert(priv->gmac_rst);

	/* force the Ethernet module to a software reset state */
	regmap_set_bits(priv->regmap, RTETH_CMD, RTETH_RESET);

	regmap_read_poll_timeout(priv->regmap, RTETH_CMD, val,
				 !(val & RTETH_RESET), 10, 1000);

	regmap_set_bits(priv->regmap, RTETH_IO_CMD,
			  RTETH_RX_ENABLE | RTETH_TX_ENABLE);
	rteth_hw_init(priv);
	spin_unlock_irq(&priv->tx_lock);

	mdelay(500);

	netif_wake_queue(dev);
}

static const struct net_device_ops rteth_netdev_ops = {
	.ndo_open		= rteth_open,
	.ndo_stop		= rteth_close,
	.ndo_start_xmit		= rteth_start_xmit,
	.ndo_set_rx_mode	= rteth_set_rx_mode,
	.ndo_set_mac_address	= rteth_set_mac_address,
	.ndo_tx_timeout		= rteth_tx_timeout,
	.ndo_validate_addr	= eth_validate_addr,
};

static const struct regmap_config regmap_config = {
	.reg_bits		= 32,
	.val_bits		= 32,
	.reg_stride		= 4,
};

static int rteth_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct net_device *netdev;
	struct rteth_priv *priv;
	void __iomem *base;
	u8 mac_addr[ETH_ALEN];
	int err;

	/* TODO Change to devm_alloc_etherdev_mqs */
	netdev = devm_alloc_etherdev(dev, sizeof(*priv));
	if (!netdev)
		return -ENOMEM;

	SET_NETDEV_DEV(netdev, dev);
	priv = netdev_priv(netdev);
	priv->netdev = netdev;
	priv->pdev = pdev;
	spin_lock_init(&priv->tx_lock);

	base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(base))
		return PTR_ERR(base);

	priv->regmap = devm_regmap_init_mmio(dev, base, &regmap_config);
	if (IS_ERR(priv->regmap))
		return PTR_ERR(priv->regmap);

	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq < 0)
	return priv->irq;

	priv->gmac_rst = devm_reset_control_array_get_optional_exclusive(dev);
	if (IS_ERR(priv->gmac_rst))
		return dev_err_probe(dev, PTR_ERR(priv->gmac_rst),
				     "Failed to get gmac reset\n");

	priv->swcore = syscon_regmap_lookup_by_phandle(dev->of_node,
						       "realtek,switchcore");
	if (IS_ERR(priv->swcore))
		return PTR_ERR(priv->swcore);

	err = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
	if (err) {
		dev_err(dev, "Failed to set DMA mask: %d\n", err);
		return err;
	}

	/* Get MAC address from device tree or generate random */
	err = of_get_mac_address(dev->of_node, mac_addr);
	if (err == -EPROBE_DEFER)
		return err;

	if (is_valid_ether_addr(mac_addr)) {
		rteth_set_mac_hw(netdev, mac_addr);
	} else {
		/* Try reading from hardware registers (set by bootloader) */
		u32 low, high;

		regmap_read(priv->regmap, RTETH_MAC0, &low);
		regmap_read(priv->regmap, RTETH_MAC4, &high);

		mac_addr[0] = low & 0xFF;
		mac_addr[1] = (low >> 8) & 0xFF;
		mac_addr[2] = (low >> 16) & 0xFF;
		mac_addr[3] = (low >> 24) & 0xFF;
		mac_addr[4] = high & 0xFF;
		mac_addr[5] = (high >> 8) & 0xFF;
	}
	dev_addr_set(netdev, mac_addr);
	/* if the address is invalid, use a random value */
	if (!is_valid_ether_addr(netdev->dev_addr)) {
		struct sockaddr sa = { AF_UNSPEC };

		netdev_warn(netdev, "Invalid MAC address, using random\n");
		eth_hw_addr_random(netdev);
		memcpy(sa.sa_data, netdev->dev_addr, ETH_ALEN);
		if (rteth_set_mac_address(netdev, &sa))
			netdev_warn(netdev, "Failed to set MAC address.\n");
	}
	pr_info("Using MAC %pM\n", netdev->dev_addr);

	/* Configure netdev */
	netdev->netdev_ops = &rteth_netdev_ops;
	netdev->watchdog_timeo = 5 * HZ;
	netdev->mtu = RTETH_MAX_MTU;
	netdev->min_mtu = ETH_MIN_MTU;
	netdev->max_mtu = RTETH_MAX_MTU;
	strscpy(netdev->name, "eth%d", sizeof(netdev->name));

	/*
	 * Enable hardware VLAN acceleration. The driver uses __vlan_hwaccel_put_tag()
	 * on RX to tag packets with the source switch port VLAN, and reads
	 * skb_vlan_tag_get_id() on TX to select the correct switch port mask.
	 */
	/* TODO Add more features for Jumbo, RX checksum */
	/* Unsupported for now
	netdev->features |= NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_HW_VLAN_CTAG_RX |
	NETIF_F_HW_VLAN_CTAG_FILTER;
	netdev->hw_features |= NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_HW_VLAN_CTAG_RX |
	NETIF_F_HW_VLAN_CTAG_FILTER;
	*/

	/* Initialize NAPI */
	netif_napi_add(netdev, &priv->napi, rteth_rx_poll);

	platform_set_drvdata(pdev, netdev);

	/* Register network device */
	err = devm_register_netdev(dev, netdev);
	if (err) {
		dev_err(dev, "Failed to register netdev: %d\n", err);
		netif_napi_del(&priv->napi);
		return err;
	}

	/* All of rtk_gmac_hw_init */
	reset_control_deassert(priv->gmac_rst);

	rteth_hw_stop(priv);

	/* Reset GMAC IP */
	reset_control_assert(priv->gmac_rst);
	mdelay(10);
	reset_control_deassert(priv->gmac_rst);

	/* config_tx_jumbo with enable */
	regmap_set_bits(priv->regmap, RTETH_TX_CONFIG, BIT(15));
	regmap_set_bits(priv->regmap, RTETH_IO_CMD, RTETH_EN_EARLY_TX);
	regmap_set_bits(priv->regmap, RTETH_CONFIG, BIT(16));

	/* Disable GMAC padding */
	regmap_set_bits(priv->regmap, RTETH_TX_CONFIG, BIT(0));
	dev_info(dev, "RTL9607C GMAC Ethernet driver loaded, MAC %pM, IRQ %d\n",
		 netdev->dev_addr, priv->irq);

	return 0;
}

static void rteth_remove(struct platform_device *pdev)
{
	struct net_device *netdev = platform_get_drvdata(pdev);
	struct rteth_priv *priv = netdev_priv(netdev);

	unregister_netdev(netdev);
	netif_napi_del(&priv->napi);
	rteth_hw_stop(priv);
}

static const struct of_device_id rteth_of_ids[] = {
	{ .compatible = "realtek,rtl9607-eth" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, rteth_of_ids);

static struct platform_driver rteth_driver = {
	.probe  = rteth_probe,
	.remove = rteth_remove,
	.driver = {
	.name = "rtl960x-eth",
		.of_match_table = rteth_of_ids,
	},
};

module_platform_driver(rteth_driver);

MODULE_DESCRIPTION("RTL960X SoC GMAC Ethernet Driver");
MODULE_LICENSE("GPL");
