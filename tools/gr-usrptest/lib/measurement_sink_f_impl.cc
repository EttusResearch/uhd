/* -*- c++ -*- */
/* 
 * Copyright 2016 Ettus Research LLC.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "measurement_sink_f_impl.h"

namespace gr {
  namespace usrptest {

    measurement_sink_f::sptr
    measurement_sink_f::make(int num_samples, int runs)
    {
      return gnuradio::get_initial_sptr
        (new measurement_sink_f_impl(num_samples, runs));

    }

    /*
     * The private constructor
     */
    measurement_sink_f_impl::measurement_sink_f_impl(int num_samples, int runs)
      : gr::sync_block("measurement_sink_f",
              gr::io_signature::make(1, 1, sizeof(float)),
              gr::io_signature::make(0, 0, 0)),
        d_runs(runs),
        d_nsamples(num_samples)
    {
      d_curr_run = 0; // number of completed measurement runs
      d_curr_avg = 0.0f; // accumulated average
      d_curr_M2 = 0.0f; // accumulated M2
      d_run = false; // true if a measurement is currently recorded
      d_curr_sample = 0; // current sample count
    }

    /*
     * Our virtual destructor.
     */
    measurement_sink_f_impl::~measurement_sink_f_impl()
    {
    }

    int
    measurement_sink_f_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const float *in = (const float *) input_items[0];
      if ((d_curr_run < d_runs)&&d_run){ //check if we need to record data
        const int max_items = std::min(noutput_items, d_nsamples-d_curr_sample); // calculate number of samples we have to take into account
        for (int item=0; item < max_items;++item){
          ++d_curr_sample;
          inc_both(in[item]);
        }
        if (d_curr_sample == d_nsamples) {
          d_avg.push_back(d_curr_avg);
          d_stddev.push_back(std::sqrt(d_curr_M2/(float)(d_curr_sample - 1)));
          ++d_curr_run;
          d_run = false;
          d_curr_sample = 0;
          d_curr_avg = 0.0f;
          d_curr_M2 = 0.0f;
        }
      }
      return noutput_items;
    }


    void
    measurement_sink_f_impl::inc_both(const float new_val)
    {
      float delta = new_val - d_curr_avg;
      d_curr_avg = d_curr_avg + delta/(float)(d_curr_sample);
      d_curr_M2 = d_curr_M2 + delta*(new_val - d_curr_avg);
    }

    
    void
    measurement_sink_f_impl::start_run()
    {
      d_run = true;
    }

    std::vector<float>
    measurement_sink_f_impl::get_avg() const
    {
      return d_avg;
    }

    std::vector<float>
    measurement_sink_f_impl::get_stddev() const
    {
      return d_stddev;
    }

    int
    measurement_sink_f_impl::get_run() const
    {
      return d_curr_run;
    }

  } /* namespace usrptest */
} /* namespace gr */

