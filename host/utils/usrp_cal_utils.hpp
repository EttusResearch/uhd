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

#include <vector>
#include <complex>
#include <cmath>
#include <fstream>

struct result_t{double freq, real_corr, imag_corr, sup;};

/***********************************************************************
 * Constants
 **********************************************************************/
static const double tau = 6.28318531;
static const double alpha = 0.0001; //very tight iir filter
static const size_t wave_table_len = 8192;
static const size_t num_search_steps = 5;
static const size_t num_search_iters = 7;
static const size_t skip_initial_samps = 20;

/***********************************************************************
 * Sinusoid wave table
 **********************************************************************/
static inline std::vector<std::complex<float> > gen_table(void){
    std::vector<std::complex<float> > wave_table(wave_table_len);
    for (size_t i = 0; i < wave_table_len; i++){
        wave_table[i] = std::polar<float>(1.0, (tau*i)/wave_table_len);
    }
    return wave_table;
}

static inline std::complex<float> wave_table_lookup(const size_t index){
    static const std::vector<std::complex<float> > wave_table = gen_table();
    return wave_table[index % wave_table_len];
}

/***********************************************************************
 * Compute power of a tone
 **********************************************************************/
static inline double compute_tone_dbrms(
    const std::vector<std::complex<float> > &samples,
    const double freq //freq is fractional
){
    //shift the samples so the tone at freq is down at DC
    std::vector<std::complex<double> > shifted(samples.size() - skip_initial_samps);
    for (size_t i = 0; i < shifted.size(); i++){
        shifted[i] = std::complex<double>(samples[i+skip_initial_samps]) * std::polar<double>(1.0, -freq*tau*i);
    }

    //filter the samples with a narrow low pass
    std::complex<double> iir_output = 0, iir_last = 0;
    double output = 0;
    for (size_t i = 0; i < shifted.size(); i++){
        iir_output = alpha * shifted[i] + (1-alpha)*iir_last;
        iir_last = iir_output;
        output += std::abs(iir_output);
    }

    return 20*std::log10(output/shifted.size());
}

/***********************************************************************
 * Write a dat file
 **********************************************************************/
static inline void write_samples_to_file(
    const std::vector<std::complex<float> > &samples, const std::string &file
){
    std::ofstream outfile(file.c_str(), std::ofstream::binary);
    outfile.write((const char*)&samples.front(), samples.size()*sizeof(std::complex<float>));
    outfile.close();
}
