//
// Copyright 2021 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "eeprom-pids.h"
#include <stddef.h>

static struct pid_info pid_list[] = {
  { 0x0410, "x410", "X410 Motherboard", 0 },
  { 0x4000, NULL, "Power Aux Board", 0 },
  { 0x4001, NULL, "Debug RF DB", 0 },
  { 0x4002, "zbx", "ZBX RF DB", 0},
  { 0x4003, NULL, "HDMI SE DIO Aux Board", 0},
  { 0x4004, NULL, "Clocking Aux Board with GPSDO", 0},
  { 0x4005, NULL, "Clocking Aux Board (no GPSDO)", 0},
  { 0x4006, NULL, "IF Test Manufacturing CCA", 0},
};

#define ARRAY_SIZE(x) (sizeof((x)) / sizeof((*x)))

const struct pid_info* get_info_from_pid(uint16_t pid) {
  for (size_t i = 0; i < ARRAY_SIZE(pid_list); i++)
    if (pid_list[i].pid == pid)
      return &pid_list[i];

  return NULL;
}

const char* get_name_from_pid(uint16_t pid) {
  const struct pid_info *info = get_info_from_pid(pid);
  if (!info)
    return NULL;
  return info->name;
}

const char* get_description_from_pid(uint16_t pid) {
  const struct pid_info *info = get_info_from_pid(pid);
  if (!info)
    return NULL;
  return info->description;
}

uint16_t get_rev_offset_from_pid(uint16_t pid) {
  const struct pid_info *info = get_info_from_pid(pid);
  if (!info)
    return -1;
  return info->rev_offset;
}
