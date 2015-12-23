
// Copyright 2012 Ettus Research LLC

#ifndef INCLUDED_WB_SPI_H
#define INCLUDED_WB_SPI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    WRITE, WRITE_READ
} wb_spi_rw_mode_t;

typedef enum {
    RISING, FALLING
} wb_spi_edge_t;

typedef struct {
    void*           base;
    uint32_t        slave_sel;
    uint32_t        clk_div;
    wb_spi_edge_t   mosi_edge;
    wb_spi_edge_t   miso_edge;
    bool            lsb_first;
} wb_spi_slave_t;

/*!
 * \brief Initialize SPI slave device
 */
void wb_spi_init(const wb_spi_slave_t* slave);

/*!
 * \brief Perform a SPI transaction in auto chip-select mode.
 */
inline void wb_spi_transact(const wb_spi_slave_t* slave,
    wb_spi_rw_mode_t rw_mode, const void* mosi_buf, void* miso_buf, uint32_t length);

/*!
 * \brief Perform a SPI transaction in manual chip-select mode.
 */
inline void wb_spi_transact_man_ss(const wb_spi_slave_t* slave,
    wb_spi_rw_mode_t rw_mode, const void* mosi_buf, void* miso_buf, uint32_t length);

/*!
 * \brief Select SPI slave
 */
void wb_spi_slave_select(const wb_spi_slave_t* slave);

/*!
 * \brief Deselect SPI slave
 */
void wb_spi_slave_deselect(const wb_spi_slave_t* slave);

#endif /* INCLUDED_WB_SPI_H */
