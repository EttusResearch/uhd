//
// Copyright 2010 Ettus Research LLC
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

#include <boost/tuple/tuple.hpp>
#include <usrp_uhd/time_spec.hpp>
#include <usrp_uhd/wax.hpp>
#include <complex>
#include <vector>

#ifndef INCLUDED_USRP_UHD_PROPS_HPP
#define INCLUDED_USRP_UHD_PROPS_HPP

namespace usrp_uhd{

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
     * Utility function to separate a named property into its components.
     * \param key a reference to the prop object
     * \param name a reference to the name object
     */
    inline named_prop_t extract_named_prop(const wax::type &key, const std::string &name = ""){
        if (key.type() == typeid(named_prop_t)){
            return wax::cast<named_prop_t>(key);
        }
        return named_prop_t(key, name);
    }

    /*!
    * Possible device properties:
    *   In general, a device will have a single mboard.
    *   In certain mimo applications, multiple boards
    *   will be present in the interface for configuration.
    */
    enum device_prop_t{
        DEVICE_PROP_NAME,              //ro, std::string
        DEVICE_PROP_MBOARD,            //ro, wax::obj
        DEVICE_PROP_MBOARD_NAMES       //ro, prop_names_t
    };

    /*!
    * Possible device mboard properties:
    *   The general mboard properties are listed below.
    *   Custom properties can be identified with a string
    *   and discovered though the others property.
    */
    enum mboard_prop_t{
        MBOARD_PROP_NAME,              //ro, std::string
        MBOARD_PROP_OTHERS,            //ro, prop_names_t
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
    * Possible device dsp properties:
    *   A dsp can have a wide range of possible properties.
    *   A ddc would have a properties "decim", "freq", "taps"...
    *   Other properties could be gains, complex scalars, enables...
    *   For this reason the only required properties of a dsp is a name
    *   and a property to get list of other possible properties.
    */
    enum dsp_prop_t{
        DSP_PROP_NAME,                 //ro, std::string
        DSP_PROP_OTHERS                //ro, prop_names_t
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
    * Possible device codec properties:
    *   A codec is expected to have a rate and gain elements.
    *   Other properties can be discovered through the others prop.
    */
    enum codec_prop_t{
        CODEC_PROP_NAME,               //ro, std::string
        CODEC_PROP_OTHERS,             //ro, prop_names_t
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
        SUBDEV_PROP_OTHERS,            //ro, prop_names_t
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
        SUBDEV_PROP_BANDWIDTH          //rw, freq_t
    };

} //namespace usrp_uhd

#endif /* INCLUDED_USRP_UHD_PROPS_HPP */
