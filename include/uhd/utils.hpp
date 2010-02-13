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

#include <uhd/wax.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <map>

#ifndef INCLUDED_UHD_UTILS_HPP
#define INCLUDED_UHD_UTILS_HPP

/*!
 * Useful templated functions and classes that I like to pretend are part of stl
 */
namespace std{

    class assert_error : public std::logic_error{
    public:
        explicit assert_error(const string& what_arg) : logic_error(what_arg){
            /* NOP */
        }
    };

    #define ASSERT_THROW(_x) if (not (_x)) { \
        throw std::assert_error("Assertion Failed: " + std::string(#_x)); \
    }

    template<class T, class InputIterator, class Function>
    T reduce(InputIterator first, InputIterator last, Function fcn, T init = 0){
        T tmp = init;
        for ( ; first != last; ++first ){
            tmp = fcn(tmp, *first);
        }
        return tmp;
    }

    template<class T, class InputIterator>
    bool has(InputIterator first, InputIterator last, const T &elem){
        return last != std::find(first, last, elem);
    }

    template<class T>
    T sum(const T &a, const T &b){
        return a + b;
    }

    template<typename T> T signum(T n){
        if (n < 0) return -1;
        if (n > 0) return 1;
        return 0;
    }

}//namespace std

namespace uhd{

inline void tune(
    freq_t target_freq,
    freq_t lo_offset,
    wax::obj subdev_freq_proxy,
    bool subdev_quadrature,
    bool subdev_spectrum_inverted,
    bool subdev_is_tx,
    wax::obj dsp_freq_proxy,
    freq_t dsp_sample_rate
){
    // Ask the d'board to tune as closely as it can to target_freq+lo_offset
    subdev_freq_proxy = target_freq + lo_offset;
    freq_t inter_freq = wax::cast<freq_t>(subdev_freq_proxy);

    // Calculate the DDC setting that will downconvert the baseband from the
    // daughterboard to our target frequency.
    freq_t delta_freq = target_freq - inter_freq;
    int delta_sign = std::signum(delta_freq);
    delta_freq *= delta_sign;
    delta_freq = fmod(delta_freq, dsp_sample_rate);
    bool inverted = delta_freq > dsp_sample_rate/2.0;
    freq_t dxc_freq = inverted? (delta_freq - dsp_sample_rate) : (-delta_freq);
    dxc_freq *= delta_sign;

    // If the spectrum is inverted, and the daughterboard doesn't do
    // quadrature downconversion, we can fix the inversion by flipping the
    // sign of the dxc_freq...  (This only happens using the basic_rx board)
    if (subdev_spectrum_inverted){
        inverted = not inverted;
    }
    if (inverted and not subdev_quadrature){
        dxc_freq = -dxc_freq;
        inverted = not inverted;
    }
    if (subdev_is_tx){
        dxc_freq = -dxc_freq;	// down conversion versus up conversion
    }

    dsp_freq_proxy = dxc_freq;
    //freq_t actual_dxc_freq = wax::cast<freq_t>(dsp_freq_proxy);

    //return some kind of tune result tuple/struct
}

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_HPP */
