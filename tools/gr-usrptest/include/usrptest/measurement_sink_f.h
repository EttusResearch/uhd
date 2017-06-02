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


#ifndef INCLUDED_USRPTEST_MEASUREMENT_SINK_F_H
#define INCLUDED_USRPTEST_MEASUREMENT_SINK_F_H

#include <usrptest/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace usrptest {

    /*!
     * \brief <+description of block+>
     * \ingroup usrptest
     *
     */
    class USRPTEST_API measurement_sink_f : virtual public gr::sync_block
    {
     public:
      typedef boost::shared_ptr<measurement_sink_f> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of usrptest::measurement_sink_f.
       *
       * To avoid accidental use of raw pointers, usrptest::measurement_sink_f's
       * constructor is in a private implementation
       * class. usrptest::measurement_sink_f::make is the public interface for
       * creating new instances.
       */
      static sptr make(int num_samples,int runs);
      virtual std::vector<float> get_avg() const = 0;
      virtual std::vector<float> get_stddev() const = 0;
      virtual int get_run() const = 0;
      virtual void start_run() = 0;

    };

  } // namespace usrptest
} // namespace gr

#endif /* INCLUDED_USRPTEST_MEASUREMENT_SINK_F_H */

