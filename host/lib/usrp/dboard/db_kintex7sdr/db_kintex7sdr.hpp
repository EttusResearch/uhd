#pragma once

#include <uhd/usrp/dboard_base.hpp>

using namespace uhd;
using namespace uhd::usrp;

namespace uhd { namespace usrp { namespace dboard { namespace db_kintex7sdr {

static const dboard_id_t DB_KINTEX7SDR_PROTO_V3_TX_ID(0x73);
static const dboard_id_t DB_KINTEX7SDR_PROTO_V3_RX_ID(0x74);
static const dboard_id_t DB_KINTEX7SDR_PROTO_V4_TX_ID(0x75);
static const dboard_id_t DB_KINTEX7SDR_PROTO_V4_RX_ID(0x76);
static const dboard_id_t DB_KINTEX7SDR_V1_40MHZ_TX_ID(0x77);
static const dboard_id_t DB_KINTEX7SDR_V1_40MHZ_RX_ID(0x78);
static const dboard_id_t DB_KINTEX7SDR_V1_160MHZ_TX_ID(0x79);
static const dboard_id_t DB_KINTEX7SDR_V1_160MHZ_RX_ID(0x7A);
static const dboard_id_t DB_KINTEX7SDR_V2_40MHZ_TX_ID(0x7B);
static const dboard_id_t DB_KINTEX7SDR_V2_40MHZ_RX_ID(0x7C);
static const dboard_id_t DB_KINTEX7SDR_V2_160MHZ_TX_ID(0x7D);
static const dboard_id_t DB_KINTEX7SDR_V2_160MHZ_RX_ID(0x7E);
static const dboard_id_t DB_KINTEX7SDR_LP_160MHZ_TX_ID(0x0200);
static const dboard_id_t DB_KINTEX7SDR_LP_160MHZ_RX_ID(0x0201);
static const dboard_id_t DB_KINTEX7SDR_TDD_160MHZ_TX_ID(0x0202);
static const dboard_id_t DB_KINTEX7SDR_TDD_160MHZ_RX_ID(0x0203);
static const std::vector<dboard_id_t> db_kintex7sdr_ids{DB_KINTEX7SDR_PROTO_V3_TX_ID,
    DB_KINTEX7SDR_PROTO_V4_TX_ID,
    DB_KINTEX7SDR_V1_40MHZ_TX_ID,
    DB_KINTEX7SDR_V1_160MHZ_TX_ID,
    DB_KINTEX7SDR_V2_40MHZ_TX_ID,
    DB_KINTEX7SDR_V2_160MHZ_TX_ID,
    DB_KINTEX7SDR_LP_160MHZ_TX_ID,
    DB_KINTEX7SDR_TDD_160MHZ_TX_ID,
    DB_KINTEX7SDR_PROTO_V3_RX_ID,
    DB_KINTEX7SDR_PROTO_V4_RX_ID,
    DB_KINTEX7SDR_V1_40MHZ_RX_ID,
    DB_KINTEX7SDR_V1_160MHZ_RX_ID,
    DB_KINTEX7SDR_V2_40MHZ_RX_ID,
    DB_KINTEX7SDR_V2_160MHZ_RX_ID,
    DB_KINTEX7SDR_LP_160MHZ_RX_ID,
    DB_KINTEX7SDR_TDD_160MHZ_RX_ID};

static UHD_INLINE double get_max_pfd_freq(dboard_id_t dboard_id)
{
    if ((dboard_id == DB_KINTEX7SDR_PROTO_V3_TX_ID) || (dboard_id == DB_KINTEX7SDR_PROTO_V3_RX_ID)) {
        return 25e6;
    }
    return 50e6;
}

}}}}; // namespace uhd::usrp::dboard::db_kintex7sdr
