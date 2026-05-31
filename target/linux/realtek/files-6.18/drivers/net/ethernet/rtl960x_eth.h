/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef RTETH_960X_H
#define RTETH_960X_H

#define RTETH_TX_RING_SIZE		256
#define RTETH_RX_RING_SIZE		512
#define RTETH_BUF_SIZE			2048
#define RTETH_MIN_PKT_LEN		60
#define RTETH_MAX_MTU			1500

#define RTETH_960X_CPU_PORT		9
#define RTETH_960X_WAN_PORT		5
#define RTETH_960X_WAN_PORT_MASK	BIT(5)
#define RTETH_960X_LAN_PORT_MASK	(BIT(0) | BIT(1) | BIT(2) | BIT(3))
#define RTETH_960X_TX_ALL_PORTMASK	(RTETH_960X_WAN_PORT_MASK | RTETH_960X_LAN_PORT_MASK)
#define RTETH_960X_WAN_VID	1
#define RTETH_960X_LAN_VID	2

/* Defaults as per OEM driver */
#define RTETH_INTERFRAMEGAP		0x03	/* 3 means InterFrameGap = the shortest one */
/* CPUTAG register */
#define RTETH_CT_TSIZE_8B		2	/* 2 means TX cputag size is 8 Bytes */
#define RTETH_CT_SWITCH_APRO		8	/* 8 means cputag format of switch is ApolloPro (RTL9607C) */
#define RTETH_CT_RSIZE_8B		2	/* 2 means RX cputag size is 8 Bytes */
#define RTETH_CT_PM_8370		0xff	/* 0xff is cputag protocol mask of 8370 */
#define RTETH_CT_PV_8370		0x04	/* 0x04 is cputag protocol value of 8370 */
/* CONFIG */
#define RTETH_RFIFO_SIZE_2K		2	/* 2 means GMAC RX FIFO size is 2KB */
/* IO_CMD register */
#define RTETH_DMA_BURST_32		1	/* 1 means DMA Burst size on bus is 32 Doublewords */
#define RTETH_RX_TIMER_60TU		0xf	/* 0xf means timer interval for RxOK interrupt is 60 TU (time units) */
#define RTETH_TX_THRESH_1024B		3	/* 3 means the threshold level for Tx FIFO is 1024B */
#define RTETH_TX_INT_4PKTS		1	/* 1 means the number of packets recieved before TxOK interrupt trigger is 4 */
#define RTETH_RX_THRESH_64B		2	/* 2 means the threshold level for Rx FIFO is 64 Bytes */
#define RTETH_RX_INT_4PKTS		1	/* 1 means the number of packets recieved before TxOK interrupt trigger is 4 */
/* IO_CMD1 register */
#define RTETH_APOLLO_DESC_FMT		3	/* 3 means the extra descriptor format is Apollo */
/* CPU_DESC_NUM1 */
#define THRESHOLD_ON_16			0x10	/* 0x10 means assert threshold: available desc <= 16*/
#define THRESHOLD_OFF_16		0x30	/* 0x10 means deassert threshold: available desc >= 16*/

/* FIXME this should belong with DSA driver */
#define SWCORE_PORT_ISO_CTRL(p)		(0x27000 + ((p) * 4))
#define SWCORE_FORCE_P_ABLTY(p)		(0x1CC + ((p) * 4))

/* FIXME this should belong with MDIO driver */
#define SWCORE_GPHY_IND_WD		0x0
#define SWCORE_GPHY_IND_CMD		0x4
#define SWCORE_GPHY_IND_RD		0x8
#define SWCORE_WRAP_GPHY_MISC		0x114

enum rteth_registers {
	RTETH_MAC0		= 0x0000,	/* Ehernet hardware address */
	RTETH_MAC4		= 0x0004,
	RTETH_MAR0		= 0x0008,	/* Multicast filter */
	RTETH_MAR4		= 0x000c,
	RTETH_MIB0		= 0x0010,	/* MIB counter 0 */
	RTETH_MIB1		= 0x0014,
	RTETH_MIB2		= 0x0018,
	RTETH_MIB3		= 0x001c,
	RTETH_MIB4		= 0x0020,
	RTETH_MIB5		= 0x0024,
	RTETH_MIB6		= 0x0028,
	RTETH_STS		= 0x0034,
	RTETH_CMD		= 0x0038,
#define RTETH_JUMBO		BIT(27)
#define RTETH_RX_VLAN		BIT(26)
#define RTETH_RX_CHKSUM		BIT(25)
#define RTETH_RESET		BIT(0)

	RTETH_INTR		= 0x003c,	/* interrupt control mask + status */
#define RTETH_STS_RDU6		BIT(31)
#define RTETH_STS_RDU5		BIT(30)
#define RTETH_STS_RDU4		BIT(29)
#define RTETH_STS_RDU3		BIT(28)
#define RTETH_STS_RDU2		BIT(27)
#define RTETH_STS_SW_INT	BIT(26)
#define RTETH_STS_TDU		BIT(25)
#define RTETH_STS_LINK_CHG	BIT(24)
#define RTETH_STS_TER		BIT(23)
#define RTETH_STS_TOK_TI	BIT(22)
#define RTETH_STS_RDU		BIT(21)
#define RTETH_STS_RER_OVF	BIT(20)
#define RTETH_STS_RER_RUNT	BIT(18)
#define RTETH_STS_ROK		BIT(16)
#define RTETH_MSK_RDU6		BIT(15)
#define RTETH_MSK_RDU5		BIT(14)
#define RTETH_MSK_RDU4		BIT(13)
#define RTETH_MSK_RDU3		BIT(12)
#define RTETH_MSK_RDU2		BIT(11)
#define RTETH_MSK_SW_INT	BIT(10)
#define RTETH_MSK_TDU		BIT(9)
#define RTETH_MSK_LINK_CHG	BIT(8)
#define RTETH_MSK_TER		BIT(7)
#define RTETH_MSK_TOK_TI	BIT(6)
#define RTETH_MSK_RDU		BIT(5)
#define RTETH_MSK_RER_OVF	BIT(4)
#define RTETH_MSK_RER_RUNT	BIT(2)
#define RTETH_MSK_ROK		BIT(0)
#define RTETH_MSK_RXALL		(RTETH_MSK_ROK | RTETH_MSK_RER_RUNT | RTETH_MSK_RER_OVF | \
				 RTETH_MSK_RDU | RTETH_MSK_SW_INT)
#define RTETH_STS_RXALL		(RTETH_STS_ROK | RTETH_STS_RER_RUNT | RTETH_STS_RER_OVF | \
				 RTETH_STS_RDU | RTETH_STS_SW_INT)
#define RTETH_MSK_TXALL		(RTETH_MSK_TDU)
#define RTETH_STS_TXALL		(RTETH_STS_TDU)

	RTETH_TX_CONFIG			= 0x0040,
#define RTETH_INTER_FRAME_GAP_MASK	GENMASK(12,10)
#define RTETH_GMAC_PADDING		BIT(0)

	RTETH_RX_CONFIG		= 0x0044,
#define RTETH_ACPT_FLOW		BIT(6)
#define RTETH_ACCEPT_ERR	BIT(5)
#define RTETH_ACCEPT_RUNT	BIT(4)
#define RTETH_ACCEPT_BROADCAST	BIT(3)
#define RTETH_ACCEPT_MULTICAST	BIT(2)
#define RTETH_ACCEPT_MYPHYS	BIT(1)
#define RTETH_ACCEPT_ALLPHYS	BIT(0)
#define RTETH_ACCEPT_MASK	(RTETH_ACCEPT_ALLPHYS | RTETH_ACCEPT_ERR | \
				 RTETH_ACCEPT_RUNT | RTETH_ACCEPT_BROADCAST | \
				 RTETH_ACCEPT_MULTICAST | RTETH_ACCEPT_MYPHYS)

	RTETH_CPUTAG		= 0x0048,	/* CPU tag Configuration */
#define RTETH_CT_EN_RX		BIT(31)
#define RTETH_CT_TSIZE		GENMASK(30,27)
#define RTETH_CT_RSIZE_H	GENMASK(26,25)
#define RTETH_CT_DSLRN		BIT(24)
#define RTETH_CT_NORMK		BIT(23)
#define RTETH_CT_ASPRI		BIT(22)
#define RTETH_CT_SWITCH		GENMASK(21,18)
#define RTETH_CT_RSIZE_L	GENMASK(17,16)
#define RTETH_CT_PROTOCOL_MASK	GENMASK(15,8)
#define RTETH_CT_PROTOCOL_VAL	GENMASK(7,0)

	RTETH_CONFIG		= 0x004c,
#define RTETH_RFIFO_SIZE	GENMASK(29,28)
#define RTETH_EN_INTR_SPLIT	BIT(24)
#define RTETH_RX_SIDEBAND	GENMASK(23,22)

	RTETH_CPUTAG1		= 0x0050,
#define RTETH_CT1_SID		BIT(14)

	RTETH_MII_STS		= 0x0058,
#define RTETH_FORCE_TX		BIT(31)
#define RTETH_RX_FLOW		BIT(30)
#define RTETH_TX_FLOW		BIT(29)

	RTETH_MIIA		= 0x005c,
	RTETH_SW_INT		= 0x0060,
	RTETH_VLAN		= 0x0064,
	RTETH_VLAN1		= 0x0068,
	RTETH_LED_CTRL		= 0x0070,

	RTETH_IMR0		= 0x00d0,
#define RTETH_IMR0_ROK6		BIT(5)
#define RTETH_IMR0_ROK5		BIT(4)
#define RTETH_IMR0_ROK4		BIT(3)
#define RTETH_IMR0_ROK3		BIT(2)
#define RTETH_IMR0_ROK2		BIT(1)
#define RTETH_IMR0_ROK1		BIT(0)
#define RTETH_IMR0_ROK_MSK	(RTETH_IMR0_ROK6 | RTETH_IMR0_ROK5 | RTETH_IMR0_ROK4 | \
				 RTETH_IMR0_ROK3 | RTETH_IMR0_ROK2 | RTETH_IMR0_ROK1)
	RTETH_IMR1		= 0x00d4,
	RTETH_ISR1		= 0x00d8,

	RTETH_TX_DESC_ADDR	= 0x1300,
	RTETH_TX_CUR_DESC_OFFSET	= 0x1304,
#define RTETH_TX_CUR_DESC_MASK		GENMASK(11,0)

	RTETH_RX_RING_ROUTE	= 0x1370,
#define RTETH_PRI_7_ROUTE	GENMASK(30,28)
#define RTETH_PRI_6_ROUTE	GENMASK(26,24)
#define RTETH_PRI_5_ROUTE	GENMASK(22,20)
#define RTETH_PRI_4_ROUTE	GENMASK(18,16)
#define RTETH_PRI_3_ROUTE	GENMASK(14,12)
#define RTETH_PRI_2_ROUTE	GENMASK(10,8)
#define RTETH_PRI_1_ROUTE	GENMASK(6,4)
#define RTETH_PRI_0_ROUTE	GENMASK(2,0)

	RTETH_RX_DESC_ADDR	= 0x1390,
	RTETH_RX_DESC_OFFSET_AND_SIZE	= 0x1394,
#define RTETH_RX_RING_SIZE_MASK		GENMASK(27,16)
#define RTETH_RX_CUR_DESC_MASK		GENMASK(11,0)

	RTETH_RX_CPU_DESC_NUM		= 0x1398,
#define RTETH_RX_CPU_DESC_NUM_MASK	GENMASK(11,0)

	RTETH_RX_DESC_THRESHOLD		= 0x139c,
#define RTETH_RX_DESC_ON_THRESH_MASK	GENMASK(27,16)
#define RTETH_RX_DESC_OFF_THRESH_MASK	GENMASK(11,0)

	RTETH_RX_DESC_ADDR1	= 0x13f0,
	RTETH_RX_DESC_OFFSET_AND_SIZE1	= 0x13f4,

	RTETH_RX_DESC_THRESHOLD1	= 0x143c,
#define RTETH_DESC_OFF_THRESH_H		GENMASK(27,24)

	RTETH_RX_CPU_DESC_NUM1		= 0x1430,
#define RTETH_CPU_DESC_NUM_L		GENMASK(31,24)
#define RTETH_DESC_ON_THRESH_L		GENMASK(23,16)
#define RTETH_DESC_OFF_THRESH_L		GENMASK(15,8)
#define RTETH_CPU_DESC_NUM_H		GENMASK(7,4)
#define RTETH_DESC_ON_THRESH_H		GENMASK(3,0)

	RTETH_IO_CMD		= 0x1434,
#define RTETH_MAX_DMA_SEL_L	BIT(31)
#define RTETH_SHORT_DES_FMT	BIT(30)
#define RTETH_MAX_DMA_SEL_H	BIT(29)
#define RTETH_EN_EARLY_TX	BIT(28)
#define RTETH_TX_PKT_TMR	GENMASK(27,24)
#define RTETH_TX_INT_TRIG_H	BIT(23)
#define RTETH_RX_PKT_TMR_H	BIT(22)
#define RTETH_TX_FIFO_THRESH	GENMASK(20,19)
#define RTETH_TX_INT_TRIG_L	GENMASK(18,16)
#define RTETH_RX_PKT_TMR_L	GENMASK(15,13)
#define RTETH_RX_FIFO_THRESH	GENMASK(12,11)
#define RTETH_RX_INT_TRIG_L	GENMASK(10,8)
#define RTETH_REG_INI_TMR_SEL	GENMASK(7,6)
#define RTETH_RX_ENABLE		BIT(5)
#define RTETH_TX_ENABLE		BIT(4)
#define RTETH_TX_FN4		BIT(3)
#define RTETH_TX_FN3		BIT(2)
#define RTETH_TX_FN2		BIT(1)
#define RTETH_TX_FN1		BIT(0)

	RTETH_IO_CMD1		= 0x1438,
#define RTETH_DESC_FMT_EXTRA	GENMASK(30,28)
#define RTETH_TX_EN_PRECISE_DMA	BIT(27)
#define RTETH_EN_RX_MULT_RING	BIT(25)
#define RTETH_EN_1GB_ADDR	BIT(24)
#define RTETH_RX_RING6		BIT(21)
#define RTETH_RX_RING5		BIT(20)
#define RTETH_RX_RING4		BIT(19)
#define RTETH_RX_RING3		BIT(18)
#define RTETH_RX_RING2		BIT(17)
#define RTETH_RX_RING1		BIT(16)
#define RTETH_TX_EN_RR_SCHEDULE	BIT(14)
#define RTETH_TX_FN5		BIT(8)
#define RTETH_TXQ5_H		BIT(4)
#define RTETH_TXQ4_H		BIT(3)
#define RTETH_TXQ3_H		BIT(2)
#define RTETH_TXQ2_H		BIT(1)
#define RTETH_TXQ1_H		BIT(0)
};

enum rteth_desc_status_bit {
	/* First doubleword. */
	RTETH_DESC_OWN		= BIT(31),	/* Descriptor is owned by NIC */
	RTETH_RING_END		= BIT(30),	/* End of descriptor ring */
	RTETH_FIRST_FRAG	= BIT(29),	/* First segment of a packet */
	RTETH_LAST_FRAG		= BIT(28),	/* Final segment of a packet */
};

enum rteth_rx_desc_bit {
	RTETH_RX_DESC_CRC_ERR	= BIT(27),
};

enum rteth_tx_desc_bit {
	/* First doubleword. */
	RTETH_TX_DESC_IPCS	= BIT(27),
	RTETH_TX_DESC_L4CS	= BIT(26),
	RTETH_TX_DESC_CRC	= BIT(23),

	/* Second doubleword. */
	RTETH_TX_CPUTAG_GEN	= BIT(31),
};

/* opts2 TX CPU tag */
#define RTETH_TX_PORTMASK		GENMASK(26,16)

/* opts3 RX CPU tag */
#define RTETH_RX_SRC_PORT		GENMASK(19,16)

struct rteth_tx_desc {
	__be32 opts1;
	__be32 addr;
	__be32 opts2;
	__be32 opts3;
	__be32 opts4;
} __packed;

struct rteth_rx_desc {
	__be32 opts1;
	__be32 addr;
	__be32 opts2;
	__be32 opts3;
} __packed;

/* Per-ring state */
struct rteth_tx_ring {
	struct rteth_tx_desc *desc;
	dma_addr_t desc_dma;
	struct sk_buff **skb;
	int head;
	int tail;
};

struct rteth_rx_ring {
	struct rteth_rx_desc *desc;
	dma_addr_t desc_dma;
	struct sk_buff **skb;
	int tail;
};

/* Main driver state */
struct rteth_priv {
	struct net_device *netdev;
	struct platform_device *pdev;
	struct napi_struct napi;
	struct regmap *regmap;
	struct regmap *swcore;
	struct reset_control *gmac_rst;
	int irq;
	spinlock_t tx_lock;

	struct rteth_tx_ring tx;
	struct rteth_rx_ring rx;

	/* IO_CMD register cached value */
	u32 iocmd_reg;
	u32 iocmd1_reg;
};

#endif /* RTETH_960X_H */
