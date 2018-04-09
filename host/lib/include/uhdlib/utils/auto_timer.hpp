//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
//
// NOTE: This is an experimental utility class and Ettus Research
//       reserves the right to make behavioral and interface changes at
//       any time, including removing this file without notice.
// It should not be used in production code.
//

#ifndef INCLUDED_UHD_UTILS_AUTO_TIMER_HPP
#define INCLUDED_UHD_UTILS_AUTO_TIMER_HPP

// for now, only implemented for windows
#ifdef UHD_PLATFORM_WIN32

// Defines struct tm
#include "time.h"

#include <windows.h>

#include <uhd/utils/msg.hpp>

/*!
 * Inserts a timer that logs the duration of its existence (construction to destruction) and the context string to UHD_MSG
 * \param context The context string to log in addition to the duration. String buffer MUST be maintained by caling code throughout lifetime of timer object.
 */
#define PROFILE_TIMING(context) \
   uhd::_auto_timer::auto_timer ___at(context);

/*!
 * Inserts a timer that logs the duration (if exceeds threshold) of its existence (construction to destruction) and the context string to UHD_MSG
 * \param context The context string to log in addition to the duration. String buffer MUST be maintained by caling code throughout lifetime of timer object.
 * \param threshold Only if the lifetime of the timer exceeds this value will it be logged
 */
#define PROFILE_TIMING_WITH_THRESHOLD(context,threshold) \
   uhd::_auto_timer::auto_timer ___at(context,threshold);

 /*!
 * Inserts a timer that logs the duration of its existence (construction to destruction) and the context string to UHD_MSG
 * \param context The context string to log in addition to the duration. String buffer MUST be maintained by caling code throughout lifetime of timer object.
 * \param unitScale Report duration in ms or us (kUnitScaleMS or kUnitScaleUS)
 */
#define PROFILE_TIMING_WITH_SCALE(context,unitScale) \
   uhd::_auto_timer::auto_timer ___at(context,0,unitScale);

 /*!
 * Inserts a timer that logs the duration (if exceeds threshold) of its existence (construction to destruction) and the context string to UHD_MSG
 * \param context The context string to log in addition to the duration. String buffer MUST be maintained by caling code throughout lifetime of timer object.
 * \param threshold Only if the lifetime of the timer exceeds this value will it be logged
 * \param unitScale Report duration in ms or us (kUnitScaleMS or kUnitScaleUS)
 */
#define PROFILE_TIMING_WITH_THRESHOLD_AND_SCALE(context,threshold,unitScale) \
   uhd::_auto_timer::auto_timer ___at(context,threshold,unitScale);

namespace uhd {
   namespace _auto_timer {

static const uint64_t kUnitScaleMS = 1000;
static const uint64_t kUnitScaleUS = 1000000;


class auto_timer
{
public:

   auto_timer(
      const char* context,
      uint64_t reporting_threshold = 0,
      uint64_t unit_scale = kUnitScaleUS) :
      _context(context),
      _reporting_threshold(reporting_threshold),
      _unit_scale(unit_scale)
   {
      ::QueryPerformanceCounter(&_start_time);
      switch (unit_scale)
      {
      case kUnitScaleMS:
         _unit_scale_str = "ms";
         break;
      case kUnitScaleUS:
      default:
         _unit_scale_str = "us";
         break;
      }
   }

   ~auto_timer()
   {
      LARGE_INTEGER freq;
      uint64_t diff_time = 0;

      ::QueryPerformanceCounter(&_end_time);
      QueryPerformanceFrequency(&freq);
      diff_time =
         (uint64_t)(_end_time.QuadPart - _start_time.QuadPart)*
         _unit_scale /
         freq.QuadPart;

      if (diff_time >= _reporting_threshold)
      {
         UHD_MSG(status) << "^ " << _context << "\t" << std::dec << diff_time << _unit_scale_str << std::endl;
      }

   }

private:
   // Usage
   auto_timer();
   auto_timer(const auto_timer&);

   LARGE_INTEGER _start_time;
   LARGE_INTEGER _end_time;
   uint64_t _unit_scale;
   uint64_t _reporting_threshold;
   const char* _context;
   char* _unit_scale_str;

}; // class auto_timer

}} //namespace uhd::_auto_timer

#else //non-windows platforms

#define PROFILE_TIMING(context) 

#define PROFILE_TIMING_WITH_THRESHOLD(context,threshold) 

#define PROFILE_TIMING_WITH_SCALE(context,unitScale) 

#define PROFILE_TIMING_WITH_THRESHOLD_AND_SCALE(context,threshold,unitScale) 

#endif

#endif /* INCLUDED_UHD_UTILS_AUTO_TIMER_HPP */
