//
// Copyright 2010 Ettus Research LLC
//

#include <boost/tuple/tuple.hpp>
#include <usrp_uhd/wax.hpp>
#include <complex>
#include <vector>
#include <stdint.h>

#ifndef INCLUDED_USRP_UHD_PROPS_HPP
#define INCLUDED_USRP_UHD_PROPS_HPP

namespace usrp_uhd{

    /*!
     * A time_spec_t holds a seconds and ticks time value.
     * The temporal width of a tick depends on the device's clock rate.
     * The time_spec_t can be used when setting the time on devices
     * and for controlling the start of streaming for applicable dsps.
     */
    struct time_spec_t{
        uint32_t secs;
        uint32_t ticks;

        /*!
         * Create a time_spec_t that holds a wildcard time.
         * This will have implementation-specific meaning.
         */
        time_spec_t(void){
            secs = ~0;
            ticks = ~0;
        }

        /*!
         * Create a time_spec_t from seconds and ticks.
         * \param new_secs the new seconds
         * \param new_ticks the new ticks (default = 0)
         */
        time_spec_t(uint32_t new_secs, uint32_t new_ticks = 0){
            secs = new_secs;
            ticks = new_ticks;
        }
    };

    //common typedefs for board properties
    typedef float gain_t;
    typedef double freq_t;

    //scalar types
    typedef int int_scalar_t;
    typedef float real_scalar_t;
    typedef std::complex<real_scalar_t> complex_scalar_t;

    //vector types
    typedef std::vector<int_scalar_t> int_vec_t;
    typedef std::vector<real_scalar_t> real_vec_t;
    typedef std::vector<complex_scalar_t> complex_vec_t;

    //typedef for handling named properties
    typedef std::vector<std::string> prop_names_t;
    typedef boost::tuple<wax::type, std::string> named_prop_t;

    /*!
    * Possible device properties.
    */
    enum device_prop_t{
        DEVICE_PROP_NAME,              //ro, std::string
        DEVICE_PROP_MBOARD,            //ro, wax::obj
        DEVICE_PROP_MBOARD_NAMES       //ro, prop_names_t
    };

    /*!
    * Possible device mboard properties
    */
    enum mboard_prop_t{
        MBOARD_PROP_NAME,              //ro, std::string
        MBOARD_PROP_MTU,               //ro, size_t
        MBOARD_PROP_CLOCK_RATE,        //ro, freq_t
        MBOARD_PROP_RX_DSP,            //ro, wax::obj
        MBOARD_PROP_RX_DSP_NAMES,      //ro, prop_names_t
        MBOARD_PROP_TX_DSP,            //ro, wax::obj
        MBOARD_PROP_TX_DSP_NAMES,      //ro, prop_names_t
        MBOARD_PROP_RX_DBOARD,         //ro, wax::obj
        MBOARD_PROP_RX_DBOARD_NAMES,   //ro, prop_names_t
        MBOARD_PROP_TX_DBOARD,         //ro, wax::obj
        MBOARD_PROP_TX_DBOARD_NAMES,   //ro, prop_names_t
        MBOARD_PROP_PPS_SOURCE,        //rw, std::string (sma, mimo)
        MBOARD_PROP_PPS_SOURCE_NAMES,  //ro, prop_names_t
        MBOARD_PROP_PPS_POLARITY,      //rw, int, +/- 1
        MBOARD_PROP_REF_SOURCE,        //rw, std::string (int, sma, mimo)
        MBOARD_PROP_REF_SOURCE_NAMES,  //ro, prop_names_t
        MBOARD_PROP_TIME_NOW,          //wo, time_spec_t
        MBOARD_PROP_TIME_NEXT_PPS      //wo, time_spec_t
    };

    /*!
    * Possible device dsp properties
    */
    enum dsp_prop_t{
        DSP_PROP_NAME,                 //ro, std::string
        DSP_PROP_FREQ,                 //rw, freq_t
        DSP_PROP_TAPS,                 //rw, *_vec_t
        DSP_PROP_RATE,                 //rw, *_scalar_t, decim/interp
        DSP_PROP_SCALAR,               //rw, *_scalar_t
        DSP_PROP_ENABLED               //rw, bool or time_spec_t
    };

    /*!
    * Possible device dboard properties
    */
    enum dboard_prop_t{
        DBOARD_PROP_NAME,              //ro, std::string
        DBOARD_PROP_SUBDEV,            //ro, wax::obj
        DBOARD_PROP_SUBDEV_NAMES,      //ro, prop_names_t
        DBOARD_PROP_CODEC              //ro, wax::obj
    };

    /*!
    * Possible device codec properties
    */
    enum codec_prop_t{
        CODEC_PROP_NAME,               //ro, std::string
        CODEC_PROP_GAIN,               //rw, gain_t
        CODEC_PROP_GAIN_MAX,           //ro, gain_t
        CODEC_PROP_GAIN_MIN,           //ro, gain_t
        CODEC_PROP_GAIN_STEP,          //ro, gain_t
        CODEC_PROP_GAIN_NAMES,         //ro, prop_names_t
        CODEC_PROP_CLOCK_RATE          //ro, freq_t
    };

    /*!
    * Possible device subdev properties
    */
    enum subdev_prop_t{
        SUBDEV_PROP_NAME,              //ro, std::string
        SUBDEV_PROP_GAIN,              //rw, gain_t
        SUBDEV_PROP_GAIN_MAX,          //ro, gain_t
        SUBDEV_PROP_GAIN_MIN,          //ro, gain_t
        SUBDEV_PROP_GAIN_STEP,         //ro, gain_t
        SUBDEV_PROP_GAIN_NAMES,        //ro, prop_names_t
        SUBDEV_PROP_FREQ,              //rw, freq_t
        SUBDEV_PROP_FREQ_MAX,          //ro, freq_t
        SUBDEV_PROP_FREQ_MIN,          //ro, freq_t
        SUBDEV_PROP_ANTENNA,           //rw, std::string
        SUBDEV_PROP_ANTENNA_NAMES,     //ro, prop_names_t
        SUBDEV_PROP_ENABLED,           //rw, bool
        SUBDEV_PROP_QUADRATURE,        //ro, bool
        SUBDEV_PROP_IQ_SWAPPED,        //ro, bool
        SUBDEV_PROP_SPECTRUM_INVERTED, //ro, bool
        SUBDEV_PROP_IS_TX,             //ro, bool
        SUBDEV_PROP_RSSI,              //ro, gain_t
        SUBDEV_PROP_BANDWIDTH,         //rw, freq_t
        SUBDEV_PROP_CLOCK_RATE         //ro, freq_t
    };

} //namespace usrp_uhd

#endif /* INCLUDED_USRP_UHD_PROPS_HPP */
