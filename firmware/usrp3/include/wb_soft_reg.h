//
// Copyright 2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_SOFT_REG_H
#define INCLUDED_SOFT_REG_H

#include <stdint.h>
#include <stdbool.h>
#include <wb_utils.h>

/* Keeps track of all metadata associated with a soft register.
 * Use this struct when you need to manage a hardware register that needs
 * to be accessed from different sections of software. If a register contains
 * several unrelated bitfields, this object can be used to ensure coherency.
 * It is recommended that the client hold this as a global/static object.
 */
typedef struct
{
    uint32_t wr_addr;
    uint32_t rd_addr;
    uint32_t soft_copy;
} soft_reg_t;

/* A register field is defined as a tuple of the mask and the shift.
 * It can be used to make read-modify-write operations more convenient
 * For efficiency reasons, it is recommended to always use a constant
 * of this type because it will get optimized out by the compiler and
 * will result in zero memory overhead
 */
typedef struct
{
    uint8_t num_bits;
    uint8_t shift;
} soft_reg_field_t;


/*!
 * Initialize the soft_reg_t struct as a read-write register.
 * Params:
 * - reg: Pointer to the soft_reg struct
 * - wr_addr: The address used to flush the register to HW
 * - rd_addr: The address used to read the register from HW
 */
static inline void initialize_readwrite_soft_reg(soft_reg_t* reg, uint32_t wr_addr, uint32_t rd_addr)
{
    reg->wr_addr = wr_addr;
    reg->rd_addr = rd_addr;
    reg->soft_copy = 0;
}

/*!
 * Initialize the soft_reg_t struct as a write-only register.
 * Params:
 * - reg: Pointer to the soft_reg struct
 * - addr: The address used to flush the register to HW
 */
static inline void initialize_writeonly_soft_reg(soft_reg_t* reg, uint32_t addr)
{
    reg->wr_addr = addr;
    reg->rd_addr = 0;
    reg->soft_copy = 0;
}

/*!
 * Update specified field in the soft-copy with the arg value.
 * Performs a read-modify-write operation so all other field are preserved.
 * NOTE: This does not write the value to hardware.
 */
static inline void soft_reg_set(soft_reg_t* reg, const soft_reg_field_t field, const uint32_t field_value)
{
    const uint32_t mask = ((1<<field.num_bits)-1)<<field.shift;
    reg->soft_copy = (reg->soft_copy & ~mask) | ((field_value << field.shift) & mask);
}

/*!
 * Write the contents of the soft-copy to hardware.
 */
static inline void soft_reg_flush(const soft_reg_t* reg)
{
    wb_poke32(reg->wr_addr, reg->soft_copy);
}

/*!
 * Shortcut for a set and a flush.
 */
static inline void soft_reg_write(soft_reg_t* reg, const soft_reg_field_t field, const uint32_t field_value)
{
    soft_reg_set(reg, field, field_value);
    soft_reg_flush(reg);
}

/*!
 * Get the value of the specified field from the soft-copy.
 * NOTE: This does not read anything from hardware.
 */
static inline uint32_t soft_reg_get(const soft_reg_t* reg, const soft_reg_field_t field)
{
    const uint32_t mask = ((1<<field.num_bits)-1)<<field.shift;
    return (reg->soft_copy & mask) >> field.shift;
}

/*!
 * Read the contents of the register from hardware and update the soft copy.
 */
static inline void soft_reg_refresh(soft_reg_t* reg)
{
    if (reg->rd_addr) {
        reg->soft_copy = wb_peek32(reg->rd_addr);
    }
}

/*!
 * Shortcut for refresh and get
 */
static inline uint32_t soft_reg_read(soft_reg_t* reg, const soft_reg_field_t field)
{
    soft_reg_refresh(reg);
    return soft_reg_get(reg, field);
}

#endif /* INCLUDED_SOFT_REG_H */
