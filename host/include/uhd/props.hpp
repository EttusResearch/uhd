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
#include <uhd/time_spec.hpp>
#include <uhd/wax.hpp>
#include <complex>
#include <vector>

#ifndef INCLUDED_UHD_PROPS_HPP
#define INCLUDED_UHD_PROPS_HPP

namespace uhd{

    //common typedefs for board properties
    typedef float gain_t;
    typedef double freq_t;

    //gain range tuple (min, max, step)
    typedef boost::tuple<gain_t, gain_t, gain_t> gain_range_t;

    //freq range tuple (min, max)
    typedef boost::tuple<freq_t, freq_t> freq_range_t;

    //scalar types (have not used yet, dont uncomment until needed)
    //typedef int int_scalar_t;
    //typedef float real_scalar_t;
    //typedef std::complex<real_scalar_t> complex_scalar_t;

    //vector types (have not used yet, dont uncomment until needed)
    //typedef std::vector<int_scalar_t> int_vec_t;
    //typedef std::vector<real_scalar_t> real_vec_t;
    //typedef std::vector<complex_scalar_t> complex_vec_t;

    //typedef for handling named properties
    typedef std::vector<std::string> prop_names_t;
    typedef boost::tuple<wax::obj, std::string> named_prop_t;

    /*!
     * Utility function to separate a named property into its components.
     * \param key a reference to the prop object
     * \param name a reference to the name object
     */
    inline named_prop_t extract_named_prop(const wax::obj &key, const std::string &name = ""){
        if (key.type() == typeid(named_prop_t)){
            return key.as<named_prop_t>();
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
        DEVICE_PROP_MBOARD_NAMES,      //ro, prop_names_t
        DEVICE_PROP_MAX_RX_SAMPLES,    //ro, size_t
        DEVICE_PROP_MAX_TX_SAMPLES     //ro, size_t
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
        MBOARD_PROP_PPS_POLARITY,      //rw, std::string (pos, neg)
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
        DBOARD_PROP_SUBDEV_NAMES       //ro, prop_names_t
        //DBOARD_PROP_CODEC              //ro, wax::obj //----> not sure, dont have to deal with yet
    }; 

    /*! ------ not dealing with yet, commented out ------------
    * Possible device codec properties:
    *   A codec is expected to have a rate and gain elements.
    *   Other properties can be discovered through the others prop.
    */
    /*enum codec_prop_t{
        CODEC_PROP_NAME,               //ro, std::string
        CODEC_PROP_OTHERS,             //ro, prop_names_t
        CODEC_PROP_GAIN,               //rw, gain_t
        CODEC_PROP_GAIN_MAX,           //ro, gain_t
        CODEC_PROP_GAIN_MIN,           //ro, gain_t
        CODEC_PROP_GAIN_STEP,          //ro, gain_t
        CODEC_PROP_GAIN_NAMES,         //ro, prop_names_t
        //CODEC_PROP_CLOCK_RATE          //ro, freq_t //----> not sure we care to know
    };*/

    /*!
    * Possible device subdev properties
    */
    enum subdev_prop_t{
        SUBDEV_PROP_NAME,              //ro, std::string
        SUBDEV_PROP_OTHERS,            //ro, prop_names_t
        SUBDEV_PROP_GAIN,              //rw, gain_t
        SUBDEV_PROP_GAIN_RANGE,        //ro, gain_range_t
        SUBDEV_PROP_GAIN_NAMES,        //ro, prop_names_t
        SUBDEV_PROP_FREQ,              //rw, freq_t
        SUBDEV_PROP_FREQ_RANGE,        //ro, freq_range_t
        SUBDEV_PROP_ANTENNA,           //rw, std::string
        SUBDEV_PROP_ANTENNA_NAMES,     //ro, prop_names_t
        SUBDEV_PROP_ENABLED,           //rw, bool
        SUBDEV_PROP_QUADRATURE,        //ro, bool
        SUBDEV_PROP_IQ_SWAPPED,        //ro, bool
        SUBDEV_PROP_SPECTRUM_INVERTED, //ro, bool
        SUBDEV_PROP_LO_INTERFERES      //ro, bool
        //SUBDEV_PROP_RSSI,              //ro, gain_t //----> not on all boards, use named prop
        //SUBDEV_PROP_BANDWIDTH          //rw, freq_t //----> not on all boards, use named prop
    };

} //namespace uhd

#endif /* INCLUDED_UHD_PROPS_HPP */
