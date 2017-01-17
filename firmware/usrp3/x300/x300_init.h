
// Copyright 2012 Ettus Research LLC

#ifndef INCLUDED_B250_INIT_H
#define INCLUDED_B250_INIT_H

#include "x300_defs.h"
#include "x300_fw_common.h"

void x300_init(x300_eeprom_map_t *eeprom_map);

void x300_serial_loader_run1(void);

typedef enum
  {
    DCO_156p25,
    DCO_125,
    DCO_10
  } dco_freq_t;

#endif /* INCLUDED_B250_INIT_H */
