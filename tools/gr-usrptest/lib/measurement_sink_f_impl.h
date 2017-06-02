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

#ifndef INCLUDED_USRPTEST_MEASUREMENT_SINK_F_IMPL_H
#define INCLUDED_USRPTEST_MEASUREMENT_SINK_F_IMPL_H

#include <usrptest/measurement_sink_f.h>

namespace gr {
  namespace usrptest {

    class measurement_sink_f_impl : public measurement_sink_f
    {
     private:
       std::vector< float > d_avg;
       std::vector< float > d_stddev;
       bool d_run;
       int d_runs;
       int d_nsamples;
       int d_curr_sample;
       int d_curr_run;
       float d_curr_avg;
       float d_curr_M2;
       void inc_both(const float new_val);


     public:
      measurement_sink_f_impl(int num_samples, int runs);
      ~measurement_sink_f_impl();
      std::vector<float> get_avg() const;
      std::vector<float> get_stddev() const;
      int get_run() const;
      void start_run();

      // Where all the action really happens
      int work(int noutput_items,
         gr_vector_const_void_star &input_items,
         gr_vector_void_star &output_items);
    };

  } // namespace usrptest
} // namespace gr

#endif /* INCLUDED_USRPTEST_MEASUREMENT_SINK_F_IMPL_H */

