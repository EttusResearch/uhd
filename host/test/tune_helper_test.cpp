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

#include <boost/test/unit_test.hpp>
#include <uhd/usrp/tune_helper.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Dummy properties objects
 **********************************************************************/
class dummy_subdev : public wax::obj{
public:
    dummy_subdev(bool is_quadrature, bool is_spectrum_inverted, double resolution):
        _is_quadrature(is_quadrature),
        _is_spectrum_inverted(is_spectrum_inverted),
        _resolution(resolution)
    {
        /* NOP */
    }
private:
    void get(const wax::obj &key, wax::obj &val){
        switch(key.as<subdev_prop_t>()){
        case SUBDEV_PROP_QUADRATURE:
            val = _is_quadrature;
            return;

        case SUBDEV_PROP_SPECTRUM_INVERTED:
            val = _is_spectrum_inverted;
            return;

        case SUBDEV_PROP_FREQ:
            val = _freq;
            return;

        case SUBDEV_PROP_USE_LO_OFFSET:
            val = false;
            return;

        default: UHD_THROW_PROP_GET_ERROR();
        }
    }

    void set(const wax::obj &key, const wax::obj &val){
        switch(key.as<subdev_prop_t>()){
        case SUBDEV_PROP_FREQ:
            _freq = _resolution*int(val.as<double>()/_resolution);
            return;

        default: UHD_THROW_PROP_SET_ERROR();
        }
    }

    bool _is_quadrature, _is_spectrum_inverted;
    double _freq, _resolution;
};

class dummy_dsp : public wax::obj{
public:
    dummy_dsp(double codec_rate):
        _codec_rate(codec_rate)
    {
        /* NOP */
    }
private:
    void get(const wax::obj &key, wax::obj &val){
        switch(key.as<dsp_prop_t>()){
        case DSP_PROP_CODEC_RATE:
            val = _codec_rate;
            return;

        case DSP_PROP_FREQ_SHIFT:
            val = _freq_shift;
            return;

        default: UHD_THROW_PROP_GET_ERROR();
        }
    }

    void set(const wax::obj &key, const wax::obj &val){
        switch(key.as<dsp_prop_t>()){
        case DSP_PROP_FREQ_SHIFT:
            _freq_shift = val.as<double>();
            return;

        default: UHD_THROW_PROP_SET_ERROR();
        }
    }

    double _codec_rate, _freq_shift;
};

/***********************************************************************
 * Tests
 **********************************************************************/
static const double tolerance = 0.001;

BOOST_AUTO_TEST_CASE(test_tune_helper_rx){
    dummy_subdev subdev(true, false, 1e6);
    dummy_dsp dsp(100e6);

    std::cout << "Testing tune helper RX automatic LO offset" << std::endl;
    tune_result_t tr = tune_rx_subdev_and_dsp(subdev.get_link(), dsp.get_link(), 2.3451e9);
    std::cout << tr.to_pp_string() << std::endl;
    BOOST_CHECK_CLOSE(tr.actual_inter_freq, 2.345e9, tolerance);
    BOOST_CHECK_CLOSE(tr.actual_dsp_freq, -100e3, tolerance);
}

BOOST_AUTO_TEST_CASE(test_tune_helper_tx){
    dummy_subdev subdev(true, false, 1e6);
    dummy_dsp dsp(100e6);

    std::cout << "Testing tune helper TX automatic LO offset" << std::endl;
    tune_result_t tr = tune_tx_subdev_and_dsp(subdev.get_link(), dsp.get_link(), 2.3451e9);
    std::cout << tr.to_pp_string() << std::endl;
    BOOST_CHECK_CLOSE(tr.actual_inter_freq, 2.345e9, tolerance);
    BOOST_CHECK_CLOSE(tr.actual_dsp_freq, 100e3, tolerance);
}
