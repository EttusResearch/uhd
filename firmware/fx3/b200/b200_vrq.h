//
// Copyright 2013-2014 Ettus Research LLC
//

/* This file defines b200 vendor requests handlers, version 1
 */
#ifndef B200_VRQ_H
#define B200_VRQ_H

uint32_t ad9361_transact_spi(const uint32_t bits);

// note: for a write instruction bit 7 from byte 0 is set to 1
#define MAKE_AD9361_WRITE(dest, reg, val) {dest[0] = 0x80 | ((reg >> 8) & 0x3F); \
                                           dest[1] = reg & 0xFF; \
                                           dest[2] = val;}
#define MAKE_AD9361_READ(dest, reg) {dest[0] = (reg >> 8) & 0x3F; \
                                                dest[1] = reg & 0xFF;}

#endif //B200_VRQ_H


