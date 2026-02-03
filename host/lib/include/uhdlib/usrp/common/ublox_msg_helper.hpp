//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/byte_vector.hpp>
#include <uhd/types/dict.hpp>

// UBLOX HEADER Byte Pair for every message
const uhd::byte_vector_t UBLOX_HDR = {0xB5, 0x62};

// UBLOX Message Class, ID Byte Pairs
const uhd::byte_vector_t UBLOX_CFG_MSG_ID    = {0x06, 0x01};
const uhd::byte_vector_t UBLOX_CFG_VALSET_ID = {0x06, 0x8A};
const uhd::byte_vector_t UBLOX_CFG_VALGET_ID = {0x06, 0x8B};

// UBLOX Configuration Key IDs to be used with VALSET and VALGET messages
constexpr uint32_t CFG_TP_TP1_ENA               = 0x10050007;
constexpr uint32_t CFG_TP_USE_LOCKED_TP1        = 0x10050009;
constexpr uint32_t CFG_TP_ALIGN_TO_TOW_TP1      = 0x1005000a;
constexpr uint32_t CFG_TP_POL_TP1               = 0x1005000b;
constexpr uint32_t CFG_SIGNAL_GPS_L1CA_ENA      = 0x10310001;
constexpr uint32_t CFG_SIGNAL_GAL_E1_ENA        = 0x10310007;
constexpr uint32_t CFG_SIGNAL_BDS_B1C_ENA       = 0x1031000f;
constexpr uint32_t CFG_SIGNAL_GPS_ENA           = 0x1031001f;
constexpr uint32_t CFG_SIGNAL_GAL_ENA           = 0x10310021;
constexpr uint32_t CFG_SIGNAL_BDS_ENA           = 0x10310022;
constexpr uint32_t CFG_SIGNAL_QZSS_ENA          = 0x10310024;
constexpr uint32_t CFG_TP_TIMEGRID_TP1          = 0x2005000c;
constexpr uint32_t CFG_TP_PULSE_DEF             = 0x20050023;
constexpr uint32_t CFG_TP_PULSE_LENGTH_DEF      = 0x20050030;
constexpr uint32_t CFG_TP_DRSTR_TP1             = 0x20050035;
constexpr uint32_t CFG_MSGOUT_NMEA_ID_VTG_UART1 = 0x209100b1;
constexpr uint32_t CFG_MSGOUT_NMEA_ID_GSA_UART1 = 0x209100c0;
constexpr uint32_t CFG_MSGOUT_NMEA_ID_GSV_UART1 = 0x209100c5;
constexpr uint32_t CFG_MSGOUT_NMEA_ID_GLL_UART1 = 0x209100ca;
constexpr uint32_t CFG_MSGOUT_NMEA_ID_ZDA_UART1 = 0x209100d9;
constexpr uint32_t CFG_MSGOUT_RXM_MEASX_UART1   = 0x20910205;
constexpr uint32_t CFG_TP_PERIOD_TP1            = 0x40050002;
constexpr uint32_t CFG_TP_PERIOD_LOCK_TP1       = 0x40050003;
constexpr uint32_t CFG_TP_LEN_TP1               = 0x40050004;
constexpr uint32_t CFG_TP_LEN_LOCK_TP1          = 0x40050005;
constexpr uint32_t CFG_UART1_BAUDRATE           = 0x40520001;

/*! Constructs a UBLOX VALSET message according to the UBLOX protocol. Note that this
 * function assumes that the users are passing in values in big-endian format, even though
 * the UBLOX expects little-endian. This function does the conversion, so that the callers
 * can use a more human-readable format.
 * \param cfg_layer Which configuration layer to set the values in.
 * \param settings A dictionary of key IDs to value byte vectors to set.
 * \return A byte vector containing the complete UBLOX VALSET message.
 */
uhd::byte_vector_t construct_valset_message(
    uint8_t cfg_layer, uhd::dict<uint32_t, uhd::byte_vector_t> settings);

/*! Constructs a UBLOX VALGET message according to the UBLOX protocol. Note that this
 * function assumes that the users are passing in values in big-endian format, even though
 * the UBLOX expects little-endian. This function does the conversion, so that the callers
 * can use a more human-readable format.
 * \param cfg_layer Which configuration layer to get the values from.
 * \param settings A dictionary of key IDs to value byte vectors to get.
 * \return A byte vector containing the complete UBLOX VALGET message.
 */
uhd::byte_vector_t construct_valget_message(
    uint8_t cfg_layer, uhd::dict<uint32_t, uhd::byte_vector_t> settings);
