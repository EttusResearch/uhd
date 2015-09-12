/*
 * Copyright 2015 Ettus Research LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <octoclock.h>

#include <net/enc28j60.h>

#include <util/delay.h>

static uint8_t  current_bank;
static uint16_t next_pkt_ptr;

#define SET_CS_ACTIVE()  SPI_PORT &= ~(1<<SPI_CS);
#define SET_CS_PASSIVE() SPI_PORT |= (1<<SPI_CS);
#define SPI_WAIT()       while(!(SPSR & (1<<SPIF)));

static uint8_t enc28j60_read_op(uint8_t op, uint8_t addr){
    SET_CS_ACTIVE();
    SPDR = (op | (addr & ADDR_MASK));
    SPI_WAIT();
    SPDR = 0x00;
    SPI_WAIT();

    if(addr & 0x80){
        SPDR = 0x00;
        SPI_WAIT();
    }

    SET_CS_PASSIVE();
    return SPDR;
}

static void enc28j60_write_op(uint8_t op, uint8_t addr, uint8_t value){
    SET_CS_ACTIVE();

    SPDR = (op | (addr & ADDR_MASK));
    SPI_WAIT();
    SPDR = value;
    SPI_WAIT();

    SET_CS_PASSIVE();
}

static void enc28j60_read_buffer(uint8_t* buf, uint16_t len){
    SET_CS_ACTIVE();

    SPDR = RBM;
    SPI_WAIT();
    while(len){
        len--;
        SPDR = 0x00;
        SPI_WAIT();
        *buf = SPDR;
        buf++;
    }
    *buf = '\0';

    SET_CS_PASSIVE();
}

static void enc28j60_write_buffer(uint8_t* buf, uint16_t len){
    SET_CS_ACTIVE();

    SPDR = WBM;
    SPI_WAIT();
    while(len){
        len--;
        SPDR = *buf;
        buf++;
        SPI_WAIT();
    }

    SET_CS_PASSIVE();
}

static void enc28j60_set_bank(uint8_t addr){
    if((addr & BANK_MASK) != current_bank){
        enc28j60_write_op(BFC, ECON1, (BSEL1|BSEL0));
        enc28j60_write_op(BFS, ECON1, ((addr & BANK_MASK) >> 5));
        current_bank = (addr & BANK_MASK);
    }
}

static uint8_t enc28j60_read(uint8_t addr){
    enc28j60_set_bank(addr);
    return enc28j60_read_op(RCR, addr);
}

static void enc28j60_write(uint8_t addr, uint16_t value){
    enc28j60_set_bank(addr);
    enc28j60_write_op(WCR, addr, value);
}

void enc28j60_init(uint8_t* mac_addr){
    SPI_DDR |= (1 << SPI_CS);
    SET_CS_PASSIVE();

    SPI_DDR  |= ((1 << SPI_MOSI) | (1 << SPI_SCK));
    SPI_DDR  &= ~(1 << SPI_MISO);
    SPI_PORT &= ~(1 << SPI_MOSI);
    SPI_PORT &= ~(1 << SPI_SCK);
    SPCR      = ((1 << SPE) | (1 << MSTR));
    SPSR     |= (1 << SPI2X);
    enc28j60_write_op(SC, 0, SC);
    next_pkt_ptr = RXSTART_INIT;

    // Designate RX addresses
    enc28j60_write(ERXSTL,   (RXSTART_INIT & 0xFF));
    enc28j60_write(ERXSTH,   (RXSTART_INIT >> 8));
    enc28j60_write(ERXNDL,   (RXSTOP_INIT & 0xFF));
    enc28j60_write(ERXNDH,   (RXSTOP_INIT >> 8));

    // Designate TX addresses
    enc28j60_write(ETXSTL,   (TXSTART_INIT & 0xFF));
    enc28j60_write(ETXSTH,   (TXSTART_INIT >> 8));
    enc28j60_write(ETXNDL,   (TXSTOP_INIT & 0xFF));
    enc28j60_write(ETXNDH,   (TXSTOP_INIT >> 8));

    // Configure filters
    enc28j60_write(ERXFCON,  (UCEN|CRCEN|PMEN|BCEN));
    enc28j60_write(EPMM0,    0x3F);
    enc28j60_write(EPMM1,    0x30);
    enc28j60_write(EPMCSL,   0xF9);
    enc28j60_write(EPMCSH,   0xF7);

    // MAC initialization
    enc28j60_write(MACON1,   (MARXEN|TXPAUS|RXPAUS));
    enc28j60_write(MACON2,   0x00);
    enc28j60_write_op(BFS,   MACON3, (PADCFG0|TXCRCEN|FRMLNEN));
    enc28j60_write(MAIPGL,   0x12);
    enc28j60_write(MAIPGH,   0x0C);
    enc28j60_write(MABBIPG,  0x12);
    enc28j60_write(MAMXFLL,  (MAX_FRAMELEN & 0xFF)); 
    enc28j60_write(MAMXFLH,  (MAX_FRAMELEN >> 8));
    enc28j60_write(MAADR5,   mac_addr[0]);
    enc28j60_write(MAADR4,   mac_addr[1]);
    enc28j60_write(MAADR3,   mac_addr[2]);
    enc28j60_write(MAADR2,   mac_addr[3]);
    enc28j60_write(MAADR1,   mac_addr[4]);
    enc28j60_write(MAADR0,   mac_addr[5]);

    enc28j60_set_bank(ECON1);
    enc28j60_write_op(BFS, ECON1, ENCRXEN);
}

uint16_t enc28j60_recv(uint8_t* buf, uint16_t max_len){
    uint16_t rxstat, len;

    // Return if no data is available
    if(enc28j60_read(EPKTCNT) == 0) return 0;

    enc28j60_write(ERDPTL, (next_pkt_ptr & 0xFF));
    enc28j60_write(ERDPTH, (next_pkt_ptr >> 8));
    next_pkt_ptr  = enc28j60_read_op(RBM, 0) | ((uint16_t)enc28j60_read_op(RBM, 0) << 8);
    len           = enc28j60_read_op(RBM, 0) | ((uint16_t)enc28j60_read_op(RBM, 0) << 8);
    len -= 4;
    rxstat        = enc28j60_read_op(RBM, 0) | ((uint16_t)enc28j60_read_op(RBM, 0) << 8);

    // Length sanity check and actual enc28j60_read call
    if(len > (max_len - 1))  len = max_len - 1;
    if((rxstat & 0x80) == 0) len = 0;
    else enc28j60_read_buffer(buf, len);

    // Update next packet pointer
    enc28j60_write(ERXRDPTL, (next_pkt_ptr & 0xFF));
    enc28j60_write(ERXRDPTH, (next_pkt_ptr >> 8));
    if(((next_pkt_ptr - 1) < RXSTART_INIT) || ((next_pkt_ptr - 1) > RXSTOP_INIT)){
        enc28j60_write(ERXRDPTL, (RXSTOP_INIT & 0xFF));
        enc28j60_write(ERXRDPTH, (RXSTOP_INIT >> 8));
    }
    else{
        enc28j60_write(ERXRDPTL, ((next_pkt_ptr - 1) & 0xFF));
        enc28j60_write(ERXRDPTH, ((next_pkt_ptr - 1) >> 8));
    }
    enc28j60_write_op(BFS, ECON2, PKTDEC);

    return len;
}

void enc28j60_send(uint8_t* buf, uint16_t len){

    // Wait for any current transmission to finish
    while(enc28j60_read_op(RCR, ECON1) & TXRTS){
        if(enc28j60_read(EIR) & TXERIF){
            enc28j60_write_op(BFS, ECON1, TXRST);
            enc28j60_write_op(BFC, ECON1, TXRST);
        }
    }

    enc28j60_write(EWRPTL, (TXSTART_INIT & 0xFF));
    enc28j60_write(EWRPTH, (TXSTART_INIT >> 8));
    enc28j60_write(ETXNDL, ((TXSTART_INIT + len) & 0xFF));
    enc28j60_write(ETXNDH, ((TXSTART_INIT + len) >> 8));
    enc28j60_write_op(WBM, 0, 0x00);
    enc28j60_write_buffer(buf, len);
    enc28j60_write_op(BFS, ECON1, TXRTS);
}
