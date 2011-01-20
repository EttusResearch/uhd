/* -*- c++ -*- */
/*
 * Copyright 2010 Ettus Research LLC
 *
 */

#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint8_t type;
	size_t length;
	uint32_t addr;
	uint8_t *data;
} ihex_record_t;


int ihex_parse(char input[], ihex_record_t *record);
