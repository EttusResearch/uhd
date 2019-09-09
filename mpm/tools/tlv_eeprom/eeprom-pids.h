//
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <stdint.h>

struct pid_info {
  uint16_t pid;
  const char* name;
  const char* description;
  uint16_t rev_offset;
};

const struct pid_info* get_info_from_pid(uint16_t pid);
const char* get_name_from_pid(uint16_t pid);
const char* get_description_from_pid(uint16_t pid);
uint16_t get_rev_offset_from_pid(uint16_t pid);
