/*
 * Copyright 2012 Ettus Research LLC
 * Copyright 2007,2008 Free Software Foundation, Inc.
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

#include "spi.h"
#include "memory_map.h"
#include "nonstdio.h"

void spi_init(void)
{
    spi_core->divider = 10;
}

void spi_wait(void)
{
    while ((readback_mux->irqs & SPI_READY_IRQ) == 0){
        //NOP
    }
}

uint32_t spi_transact(bool readback, int slave, uint32_t data, int length, uint32_t flags)
{
    uint32_t control_word = 0;
    control_word |= (slave << SPI_CORE_SLAVE_SELECT_SHIFT);
    control_word |= (length << SPI_CORE_NUM_BITS_SHIFT);
    if ((flags & SPI_PUSH_RISE)  != 0) control_word |= (1 << SPI_CORE_DATA_OUT_EDGE_SHIFT);
    if ((flags & SPI_PUSH_FALL)  != 0) control_word |= (0 << SPI_CORE_DATA_OUT_EDGE_SHIFT);
    if ((flags & SPI_LATCH_RISE) != 0) control_word |= (1 << SPI_CORE_DATA_IN_EDGE_SHIFT);
    if ((flags & SPI_LATCH_FALL) != 0) control_word |= (0 << SPI_CORE_DATA_IN_EDGE_SHIFT);

    const uint32_t data_out = data << (32 - length);

    spi_wait();
    spi_core->control = control_word;
    spi_core->data = data_out;

    if (!readback) return 0;

    spi_wait();
    return readback_mux->spi;
}
