//
// Copyright 2011-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/device_addr.hpp>
#include <uhdlib/usrp/common/adf435x.hpp>
#include <uhdlib/usrp/common/max287x.hpp>

// LO Related
#define ADF435X_CE      (1 << 3)
#define ADF435X_PDBRF   (1 << 2)
#define ADF435X_MUXOUT  (1 << 1) // INPUT!!!
#define LOCKDET_MASK    (1 << 0) // INPUT!!!

// Common IO Pins
#define LO_LPF_EN       (1 << 15)

// TX IO Pins
#define TRSW            (1 << 14)               // 0 = TX, 1 = RX
#define TX_LED_TXRX     (1 << 7)                // LED for TX Antenna Selection TX/RX
#define TX_LED_LD       (1 << 6)                // LED for TX Lock Detect
#define DIS_POWER_TX    (1 << 5)                // on UNIT_TX, 0 powers up TX
#define TX_ENABLE       (1 << 4)                // on UNIT_TX, 0 disables TX Mixer

// RX IO Pins
#define LNASW           (1 << 14)               // 0 = TX/RX, 1 = RX2
#define RX_LED_RX1RX2   (1 << 7)                // LED for RX Antenna Selection RX1/RX2
#define RX_LED_LD       (1 << 6)                // LED for RX Lock Detect
#define DIS_POWER_RX    (1 << 5)                // on UNIT_RX, 0 powers up RX
#define RX_DISABLE      (1 << 4)                // on UNIT_RX, 1 disables RX Mixer and Baseband
#define RX_ATTN_SHIFT   8 //lsb of RX Attenuator Control
#define RX_ATTN_MASK    (63 << RX_ATTN_SHIFT) //valid bits of RX Attenuator Control

// TX Attenuator Pins
#define TX_ATTN_SHIFT   8                       // lsb of TX Attenuator Control
#define TX_ATTN_MASK    (63 << TX_ATTN_SHIFT)   // valid bits of TX Attenuator Control

// Mixer functions
#define TX_MIXER_ENB    (ADF435X_PDBRF|TX_ENABLE)
#define TX_MIXER_DIS    0

#define RX_MIXER_ENB    (ADF435X_PDBRF)
#define RX_MIXER_DIS    0

// Pin functions
#define TX_LED_IO       (TX_LED_TXRX|TX_LED_LD)     // LED gpio lines, pull down for LED
#define TXIO_MASK       (LO_LPF_EN|TRSW|ADF435X_CE|ADF435X_PDBRF|TX_ATTN_MASK|DIS_POWER_TX|TX_ENABLE)

#define RX_LED_IO       (RX_LED_RX1RX2|RX_LED_LD)   // LED gpio lines, pull down for LED
#define RXIO_MASK       (LO_LPF_EN|LNASW|ADF435X_CE|ADF435X_PDBRF|RX_ATTN_MASK|DIS_POWER_RX|RX_DISABLE)

// Power functions
#define TX_POWER_UP     (ADF435X_CE)
#define TX_POWER_DOWN   (DIS_POWER_TX)

#define RX_POWER_UP     (ADF435X_CE)
#define RX_POWER_DOWN   (DIS_POWER_RX)

// Antenna constants
#define ANT_TX          TRSW                    //the tx line is transmitting
#define ANT_RX          0                       //the tx line is receiving
#define ANT_TXRX        0                       //the rx line is on txrx
#define ANT_RX2         LNASW                   //the rx line in on rx2
#define ANT_XX          LNASW                   //dont care how the antenna is set


#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/algorithm.hpp>

#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/thread.hpp>


using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;


/***********************************************************************
 * The SBX dboard constants
 **********************************************************************/
static const freq_range_t sbx_freq_range(400e6, 4.4e9);
static const freq_range_t cbx_freq_range(1200e6, 6.0e9);

static const freq_range_t sbx_tx_lo_2dbm = list_of
    (range_t(0.35e9, 0.37e9))
;

static const freq_range_t sbx_enable_tx_lo_filter = list_of
    (range_t(0.4e9, 1.5e9))
;

static const freq_range_t sbx_enable_rx_lo_filter = list_of
    (range_t(0.4e9, 1.5e9))
;

static const freq_range_t cbx_enable_tx_lo_filter = list_of
    (range_t(1.2e9, 2e9))
;

static const freq_range_t cbx_enable_rx_lo_filter = list_of
    (range_t(1.2e9, 2e9))
;

static const std::vector<std::string> sbx_tx_antennas = list_of("TX/RX")("CAL");

static const std::vector<std::string> sbx_rx_antennas = list_of("TX/RX")("RX2")("CAL");

static const uhd::dict<std::string, gain_range_t> sbx_tx_gain_ranges = map_list_of
    ("PGA0", gain_range_t(0, 31.5, double(0.5)))
;

static const uhd::dict<std::string, gain_range_t> sbx_rx_gain_ranges = map_list_of
    ("PGA0", gain_range_t(0, 31.5, double(0.5)))
;

/***********************************************************************
 * The SBX dboard
 **********************************************************************/
class sbx_xcvr : public xcvr_dboard_base{
public:
    sbx_xcvr(ctor_args_t args);
    virtual ~sbx_xcvr(void);

protected:

    uhd::dict<std::string, double> _tx_gains, _rx_gains;
    double       _rx_lo_freq, _tx_lo_freq;
    std::string  _tx_ant, _rx_ant;
    bool _rx_lo_lock_cache, _tx_lo_lock_cache;

    void set_rx_ant(const std::string &ant);
    void set_tx_ant(const std::string &ant);
    double set_rx_gain(double gain, const std::string &name);
    double set_tx_gain(double gain, const std::string &name);

    void update_atr(void);

    /*!
     * Set the LO frequency for the particular dboard unit.
     * \param unit which unit rx or tx
     * \param target_freq the desired frequency in Hz
     * \return the actual frequency in Hz
     */
    virtual double set_lo_freq(dboard_iface::unit_t unit, double target_freq);

    /*!
     * Get the lock detect status of the LO.
     * \param unit which unit rx or tx
     * \return true for locked
     */
    sensor_value_t get_locked(dboard_iface::unit_t unit);

    /*!
     * Flash the LEDs
     */
    void flash_leds(void);

    /*!
     * Version-agnostic ABC that wraps version-specific implementations of the
     * WBX base daughterboard.
     *
     * This class is an abstract base class, and thus is impossible to
     * instantiate.
     */
    class sbx_versionx {
    public:
        sbx_versionx() {}
        virtual ~sbx_versionx(void) {}

        virtual double set_lo_freq(dboard_iface::unit_t unit, double target_freq) = 0;
    };

    /*!
     * Version 3 of the SBX Daughterboard
     */
    class sbx_version3 : public sbx_versionx {
    public:
        sbx_version3(sbx_xcvr *_self_sbx_xcvr);
        virtual ~sbx_version3(void);

        double set_lo_freq(dboard_iface::unit_t unit, double target_freq);

        /*! This is the registered instance of the wrapper class, sbx_base. */
        sbx_xcvr *self_base;
    private:
        adf435x_iface::sptr _txlo;
        adf435x_iface::sptr _rxlo;
        void write_lo_regs(dboard_iface::unit_t unit, const std::vector<uint32_t> &regs);
    };

    /*!
     * Version 4 of the SBX Daughterboard
     *
     * The only difference in the fourth revision is the ADF4351 vs the ADF4350.
     */
    class sbx_version4 : public sbx_versionx {
    public:
        sbx_version4(sbx_xcvr *_self_sbx_xcvr);
        virtual ~sbx_version4(void);

        double set_lo_freq(dboard_iface::unit_t unit, double target_freq);

        /*! This is the registered instance of the wrapper class, sbx_base. */
        sbx_xcvr *self_base;
    private:
        adf435x_iface::sptr _txlo;
        adf435x_iface::sptr _rxlo;
        void write_lo_regs(dboard_iface::unit_t unit, const std::vector<uint32_t> &regs);
    };

    /*!
     * CBX daughterboard
     *
     * There are a few differences between SBX and CBX
     * - The CBX and SBX use the MAX2870 and ADF435x respectively for LOs
     * - There are different frequency ranges
     * - There are different LO LPF cutoff frequencies
     * There is also no LO filter switching required on CBX, but the GPIO is left
     * blank so we don't worry about it.
     */
    class cbx : public sbx_versionx {
    public:
        cbx(sbx_xcvr *_self_sbx_xcvr);
        virtual ~cbx(void);

        double set_lo_freq(dboard_iface::unit_t unit, double target_freq);

        /*! This is the registered instance of the wrapper class, sbx_base. */
        sbx_xcvr *self_base;
    private:
        void write_lo_regs(dboard_iface::unit_t unit, const std::vector<uint32_t> &regs);
        max287x_iface::sptr _txlo;
        max287x_iface::sptr _rxlo;
    };

    /*!
     * Frequency range of the daughterboard; this is set in the constructor
     * to correspond either to SBX or CBX.
     */
    freq_range_t freq_range;

    /*!
    * Frequency range to use the LO LPF in RX; this is set in the constructor
    * to correspond either to SBX or CBX.
    */
    freq_range_t enable_rx_lo_filter;

    /*!
    * Frequency range to use the LO LPF in TX; this is set in the constructor
    * to correspond either to SBX or CBX.
    */
    freq_range_t enable_tx_lo_filter;

    /*!
     * Handle to the version-specific implementation of the SBX.
     *
     * Since many of this class's functions are dependent on the version of the
     * SBX board, this class will instantiate an object of the appropriate
     * sbx_version* subclass, and invoke any relevant functions through that
     * object.  This pointer is set to the proper object at construction time.
     */
    typedef boost::shared_ptr<sbx_versionx> sbx_versionx_sptr;
    sbx_versionx_sptr db_actual;
};

