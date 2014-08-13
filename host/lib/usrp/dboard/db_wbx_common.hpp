//
// Copyright 2011,2013-2014 Ettus Research LLC
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

#ifndef INCLUDED_LIBUHD_USRP_DBOARD_DB_WBX_COMMON_HPP
#define INCLUDED_LIBUHD_USRP_DBOARD_DB_WBX_COMMON_HPP

#include <uhd/types/device_addr.hpp>

#include "../common/adf435x_common.hpp"

// TX IO Pins
#define TX_PUP_5V       (1 << 7)                // enables 5.0V power supply
#define TX_PUP_3V       (1 << 6)                // enables 3.3V supply
#define TXMOD_EN        (1 << 4)                // on UNIT_TX, 1 enables TX Modulator

// RX IO Pins
#define RX_PUP_5V       (1 << 7)                // enables 5.0V power supply
#define RX_PUP_3V       (1 << 6)                // enables 3.3V supply
#define RXBB_PDB        (1 << 4)                // on UNIT_RX, 1 powers up RX baseband

// TX Attenuator Pins (v3 only)
#define TX_ATTN_16      (1 << 14)
#define TX_ATTN_8       (1 << 5)
#define TX_ATTN_4       (1 << 4)
#define TX_ATTN_2       (1 << 3)
#define TX_ATTN_1       (1 << 1)
#define TX_ATTN_MASK    (TX_ATTN_16|TX_ATTN_8|TX_ATTN_4|TX_ATTN_2|TX_ATTN_1)      // valid bits of TX Attenuator Control

// Mixer functions
#define TX_MIXER_ENB    (TXMOD_EN|ADF435X_PDBRF)    // for v3, TXMOD_EN tied to ADF435X_PDBRF rather than separate
#define TX_MIXER_DIS    0

#define RX_MIXER_ENB    (RXBB_PDB|ADF435X_PDBRF)
#define RX_MIXER_DIS    0

// Power functions
#define TX_POWER_UP     (TX_PUP_5V|TX_PUP_3V) // high enables power supply
#define TX_POWER_DOWN   0

#define RX_POWER_UP     (RX_PUP_5V|RX_PUP_3V|ADF435X_CE) // high enables power supply
#define RX_POWER_DOWN   0


#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/bind.hpp>

namespace uhd{ namespace usrp{


/***********************************************************************
 * The WBX Common dboard constants
 **********************************************************************/
static const uhd::dict<std::string, gain_range_t> wbx_rx_gain_ranges = boost::assign::map_list_of
    ("PGA0", gain_range_t(0, 31.5, 0.5));

static const freq_range_t wbx_tx_lo_5dbm = boost::assign::list_of
    (range_t(0.05e9, 1.7e9))
    (range_t(1.9e9, 2.2e9))
;

static const freq_range_t wbx_tx_lo_m1dbm = boost::assign::list_of
    (range_t(1.7e9, 1.9e9))
;

static const freq_range_t wbx_rx_lo_5dbm = boost::assign::list_of
    (range_t(0.05e9, 1.4e9))
;

static const freq_range_t wbx_rx_lo_2dbm = boost::assign::list_of
    (range_t(1.4e9, 2.2e9))
;


/***********************************************************************
 * The WBX dboard base class
 **********************************************************************/
class wbx_base : public xcvr_dboard_base{
public:
    wbx_base(ctor_args_t args);
    virtual ~wbx_base(void);

protected:
    virtual double set_rx_gain(double gain, const std::string &name);

    virtual void set_rx_enabled(bool enb);

    /*!
     * Get the lock detect status of the LO.
     *
     * This operation is identical for all versions of the WBX board.
     * \param unit which unit rx or tx
     * \return true for locked
     */
    virtual sensor_value_t get_locked(dboard_iface::unit_t unit);

    /*!
     * Version-agnostic ABC that wraps version-specific implementations of the
     * WBX base daughterboard.
     *
     * This class is an abstract base class, and thus is impossible to
     * instantiate.
     */
    class wbx_versionx {
    public:
        wbx_versionx():self_base(NULL) {}
        virtual ~wbx_versionx(void) {}

        virtual double set_tx_gain(double gain, const std::string &name) = 0;
        virtual void set_tx_enabled(bool enb) = 0;
        virtual double set_lo_freq(dboard_iface::unit_t unit, double target_freq) = 0;

        /*! This is the registered instance of the wrapper class, wbx_base. */
        wbx_base *self_base;

        property_tree::sptr get_rx_subtree(void){
            return self_base->get_rx_subtree();
        }

        property_tree::sptr get_tx_subtree(void){
            return self_base->get_tx_subtree();
        }
    };


    /*!
     * Version 2 of the WBX Daughterboard
     *
     * Basically the original release of the DB.
     */
    class wbx_version2 : public wbx_versionx {
    public:
        wbx_version2(wbx_base *_self_wbx_base);
        ~wbx_version2(void);

        double set_tx_gain(double gain, const std::string &name);
        void set_tx_enabled(bool enb);
        double set_lo_freq(dboard_iface::unit_t unit, double target_freq);
    };

    /*!
     * Version 3 of the WBX Daughterboard
     *
     * Fixed a problem with the AGC from Version 2.
     */
    class wbx_version3 : public wbx_versionx {
    public:
        wbx_version3(wbx_base *_self_wbx_base);
        ~wbx_version3(void);

        double set_tx_gain(double gain, const std::string &name);
        void set_tx_enabled(bool enb);
        double set_lo_freq(dboard_iface::unit_t unit, double target_freq);
    };

    /*!
     * Version 4 of the WBX Daughterboard
     *
     * Upgrades the Frequnecy Synthensizer from ADF4350 to ADF4351.
     */
    class wbx_version4 : public wbx_versionx {
    public:
        wbx_version4(wbx_base *_self_wbx_base);
        ~wbx_version4(void);

        double set_tx_gain(double gain, const std::string &name);
        void set_tx_enabled(bool enb);
        double set_lo_freq(dboard_iface::unit_t unit, double target_freq);
    };

    /*!
     * Handle to the version-specific implementation of the WBX.
     *
     * Since many of this class's functions are dependent on the version of the
     * WBX board, this class will instantiate an object of the appropriate
     * wbx_version_* subclass, and invoke any relevant functions through that
     * object.  This pointer is set to the proper object at construction time.
     */
    typedef boost::shared_ptr<wbx_versionx> wbx_versionx_sptr;
    wbx_versionx_sptr db_actual;

    uhd::dict<std::string, double> _tx_gains, _rx_gains;
    bool _rx_enabled, _tx_enabled;
};


}} //namespace uhd::usrp

#endif /* INCLUDED_LIBUHD_USRP_DBOARD_DB_WBX_COMMON_HPP */
