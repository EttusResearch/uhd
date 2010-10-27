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
    dummy_subdev(double resolution):
        _resolution(resolution)
    {
        /* NOP */
    }
private:
    void get(const wax::obj &key, wax::obj &val){
        switch(key.as<subdev_prop_t>()){

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

    double _freq, _resolution;
};

class dummy_subdev_basic : public wax::obj{
private:
    void get(const wax::obj &key, wax::obj &val){
        switch(key.as<subdev_prop_t>()){

        case SUBDEV_PROP_FREQ:
            val = double(0.0); //always zero
            return;

        case SUBDEV_PROP_USE_LO_OFFSET:
            val = false;
            return;

        default: UHD_THROW_PROP_GET_ERROR();
        }
    }

    void set(const wax::obj &key, const wax::obj &){
        switch(key.as<subdev_prop_t>()){
        case SUBDEV_PROP_FREQ:
            // do nothing
            return;

        default: UHD_THROW_PROP_SET_ERROR();
        }
    }
};

class dummy_subdev_bw : public wax::obj{
private:
    void get(const wax::obj &key, wax::obj &val){
        switch(key.as<subdev_prop_t>()){

        case SUBDEV_PROP_FREQ:
            val = _freq;
            return;

        case SUBDEV_PROP_USE_LO_OFFSET:
            val = true;
            return;

        case SUBDEV_PROP_BANDWIDTH:
            val = _bandwidth;
            return;

        default: UHD_THROW_PROP_GET_ERROR();
        }
    }

    void set(const wax::obj &key, const wax::obj &val){
        switch(key.as<subdev_prop_t>()){
        case SUBDEV_PROP_FREQ:
            _freq = val.as<double>();
            return;

        case SUBDEV_PROP_BANDWIDTH:
            _bandwidth = val.as<double>();
            return;

        default: UHD_THROW_PROP_SET_ERROR();
        }
    }

    double _freq, _bandwidth;
};

class dummy_dsp : public wax::obj{
public:
    dummy_dsp(double codec_rate):
        _codec_rate(codec_rate)
    {
        /* NOP */
    }
private:
    void get(const wax::obj &key_, wax::obj &val){
        named_prop_t key = named_prop_t::extract(key_);
        switch(key.as<dsp_prop_t>()){
        case DSP_PROP_CODEC_RATE:
            val = _codec_rate;
            return;

        case DSP_PROP_HOST_RATE:
            val = _host_rate;
            return;

        case DSP_PROP_FREQ_SHIFT:
            val = _freq_shift;
            return;

        case DSP_PROP_FREQ_SHIFT_NAMES:
            val = prop_names_t(1, "");
            return;

        default: UHD_THROW_PROP_GET_ERROR();
        }
    }

    void set(const wax::obj &key_, const wax::obj &val){
        named_prop_t key = named_prop_t::extract(key_);
        switch(key.as<dsp_prop_t>()){
        case DSP_PROP_FREQ_SHIFT:
            _freq_shift = val.as<double>();
            return;

        case DSP_PROP_HOST_RATE:
            _host_rate = val.as<double>();
            return;

        default: UHD_THROW_PROP_SET_ERROR();
        }
    }

    double _codec_rate, _freq_shift, _host_rate;
};

/***********************************************************************
 * Test cases
 **********************************************************************/
static const double tolerance = 0.001;

BOOST_AUTO_TEST_CASE(test_tune_helper_rx){
    dummy_subdev subdev(1e6);
    dummy_dsp dsp(100e6);

    std::cout << "Testing tune helper RX automatic IF offset" << std::endl;
    tune_result_t tr = tune_rx_subdev_and_dsp(subdev.get_link(), dsp.get_link(), 0, 2.3451e9);
    std::cout << tr.to_pp_string() << std::endl;
    BOOST_CHECK_CLOSE(tr.actual_inter_freq, 2.345e9, tolerance);
    BOOST_CHECK_CLOSE(tr.actual_dsp_freq, -100e3, tolerance);

    double freq_derived = derive_freq_from_rx_subdev_and_dsp(subdev.get_link(), dsp.get_link(), 0);
    BOOST_CHECK_CLOSE(freq_derived, 2.3451e9, tolerance);
}

BOOST_AUTO_TEST_CASE(test_tune_helper_tx){
    dummy_subdev subdev(1e6);
    dummy_dsp dsp(100e6);

    std::cout << "Testing tune helper TX automatic IF offset" << std::endl;
    tune_result_t tr = tune_tx_subdev_and_dsp(subdev.get_link(), dsp.get_link(), 0, 2.3451e9);
    std::cout << tr.to_pp_string() << std::endl;
    BOOST_CHECK_CLOSE(tr.actual_inter_freq, 2.345e9, tolerance);
    BOOST_CHECK_CLOSE(tr.actual_dsp_freq, 100e3, tolerance);

    double freq_derived = derive_freq_from_tx_subdev_and_dsp(subdev.get_link(), dsp.get_link(), 0);
    BOOST_CHECK_CLOSE(freq_derived, 2.3451e9, tolerance);
}

BOOST_AUTO_TEST_CASE(test_tune_helper_rx_nyquist){
    dummy_subdev_basic subdev;
    dummy_dsp dsp(100e6);

    std::cout << "Testing tune helper RX dummy basic board" << std::endl;
    tune_result_t tr = tune_rx_subdev_and_dsp(subdev.get_link(), dsp.get_link(), 0, 55e6);
    std::cout << tr.to_pp_string() << std::endl;
    BOOST_CHECK_CLOSE(tr.actual_inter_freq, 0.0, tolerance);
    BOOST_CHECK_CLOSE(tr.actual_dsp_freq, 45e6, tolerance);

    double freq_derived = derive_freq_from_rx_subdev_and_dsp(subdev.get_link(), dsp.get_link(), 0);
    BOOST_CHECK_CLOSE(freq_derived, -45e6, tolerance);
}

BOOST_AUTO_TEST_CASE(test_tune_helper_rx_lo_off){
    dummy_subdev_bw subdev;
    dummy_dsp dsp(100e6);
    tune_result_t tr;

    std::cout << "Testing tune helper RX automatic LO offset B >> fs" << std::endl;
    subdev[SUBDEV_PROP_BANDWIDTH] = double(40e6);
    dsp[DSP_PROP_HOST_RATE] = double(4e6);
    tr = tune_rx_subdev_and_dsp(subdev.get_link(), dsp.get_link(), 0, 2.45e9);
    std::cout << tr.to_pp_string() << std::endl;
    BOOST_CHECK_CLOSE(tr.actual_inter_freq, 2.45e9+4e6/2, tolerance);

    std::cout << "Testing tune helper RX automatic LO offset B > fs" << std::endl;
    subdev[SUBDEV_PROP_BANDWIDTH] = double(40e6);
    dsp[DSP_PROP_HOST_RATE] = double(25e6);
    tr = tune_rx_subdev_and_dsp(subdev.get_link(), dsp.get_link(), 0, 2.45e9);
    std::cout << tr.to_pp_string() << std::endl;
    BOOST_CHECK_CLOSE(tr.actual_inter_freq, 2.45e9+(40e6-25e6)/2, tolerance);

    std::cout << "Testing tune helper RX automatic LO offset B < fs" << std::endl;
    subdev[SUBDEV_PROP_BANDWIDTH] = double(20e6);
    dsp[DSP_PROP_HOST_RATE] = double(25e6);
    tr = tune_rx_subdev_and_dsp(subdev.get_link(), dsp.get_link(), 0, 2.45e9);
    std::cout << tr.to_pp_string() << std::endl;
    BOOST_CHECK_CLOSE(tr.actual_inter_freq, 2.45e9, tolerance);
}
