
#ifndef INCLUDED_XGE_MAC_H
#define INCLUDED_XGE_MAC_H

#define HAVE_OPENCORES_XGEMAC 1

#ifdef HAVE_OPENCORES_XGEMAC
#define XGE_TX_ENABLE    (1 << 0)         // Set to enable transmission.
// Interupt register bits.
#define XGE_RX_FRAG_ERR  (1 << 8)
#define XGE_RX_CRC_ERR   (1 << 7)
#define XGE_RX_PAUSE     (1 << 6)
#define XGE_REMOTE_FAULT (1 << 5)
#define XGE_LOCAL_FAULT  (1 << 4)
#define XGE_RX_UNDERFLOW (1 << 3)
#define XGE_RX_OVERFLOW  (1 << 2)
#define XGE_TX_UNDERFLOW (1 << 1)
#define XGE_TX_OVERFLOW  (1 << 0)
#endif

// MDIO OP
#define XGE_MDIO_CLAUSE(n) ((n & 0x1) << 12)
#define CLAUSE45 1
#define CLAUSE22 0
#define XGE_MDIO_OP(n)     ((n & 0x3) << 10)
#define MDIO_ADDRESS 0
#define MDIO_WRITE 1
#define MDIO_READ 3
#define MDIO_C22_WRITE 2
#define MDIO_C22_READ 1
#define MDIO_READ_POST 2
#define XGE_MDIO_ADDR(n)   ((n & 0x1f) << 5)
#define XGE_MDIO_MMD(n)    ((n & 0x1f) << 0)

#endif


