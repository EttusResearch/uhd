//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: test_exec (Header)
//
// Description: Useful macros and definitions for PkgTestExec. This file should
// be included by modules that use PkgTestExec.
//

`ifndef TEST_EXEC_H
`define TEST_EXEC_H


//-----------------------------------------------------------------------------
// Simulation Timing
//-----------------------------------------------------------------------------
//
// In order for simulations to work correctly, it's important that that all
// modules use the same time unit and a compatible precision. Otherwise the
// times passed between the module and PkgTestExec may not be consistent.
//
//-----------------------------------------------------------------------------

timeunit      1ns;
timeprecision 1ps;


//-----------------------------------------------------------------------------
// Assertion Macros
//-----------------------------------------------------------------------------
//
// These are mirrors of the equivalently named class methods. These are
// re-implemented as macros here so that the correct line and file information
// gets reported by the simulator.
//
// NOTE: This assumes that there is a PkgTestExec object within scope of where
//       the macro is used.
//
//-----------------------------------------------------------------------------

// To change the name of the TestExec object being used by the assertion
// macros, `define TEST_EXEC_OBJ before including this file and `undef it at
// the end of your testbench. Otherwise, it defaults to the shared object
// "PkgTestExec::test".
`ifndef TEST_EXEC_OBJ
`define TEST_EXEC_OBJ PkgTestExec::test
`endif


// Assert the given expression and call $fatal() if it fails.
//
//   EXPR:     The expression value to be asserted
//   MESSAGE:  String to report if the assertion fails
//  
`define ASSERT_FATAL(EXPR, MESSAGE)                                  \
  begin                                                              \
    `TEST_EXEC_OBJ.num_assertions++;                                 \
    assert (EXPR) else begin                                         \
      `TEST_EXEC_OBJ.test_status[`TEST_EXEC_OBJ.num_started] = 0;    \
      $fatal(1, MESSAGE);                                            \
    end                                                              \
  end

// Assert the given expression and call $error() if it fails. Simulation
// will also be stopped (using $stop) if stop_on_error is true.
//
//   EXPR:     The expression value to be asserted
//   MESSAGE:  String to report if the assertion fails
//  
`define ASSERT_ERROR(EXPR, MESSAGE)                                  \
  begin                                                              \
    `TEST_EXEC_OBJ.num_assertions++;                                 \
    assert (EXPR) else begin                                         \
      `TEST_EXEC_OBJ.test_status[`TEST_EXEC_OBJ.num_started] = 0;    \
      $error(MESSAGE);                                               \
      if (`TEST_EXEC_OBJ.stop_on_error) $stop(1);                    \
    end                                                              \
  end

// Assert the given expression and call $warning() if it fails.
//
//   EXPR:     The expression value to be asserted
//   MESSAGE:  String to report if the assertion fails
//  
`define ASSERT_WARNING(EXPR, MESSAGE)                                \
  begin                                                              \
    `TEST_EXEC_OBJ.num_assertions++;                                 \
    assert (EXPR) else begin                                         \
      $warning(MESSAGE);                                             \
    end                                                              \
  end

// Assert the given expression and call $info() if it fails.
//
//   EXPR:     The expression value to be asserted
//   MESSAGE:  String to report if the assertion fails
//  
`define ASSERT_INFO(EXPR, MESSAGE)                                   \
  begin                                                              \
    assert (EXPR) else begin                                         \
      $info(MESSAGE)                                                 \
    end                                                              \
  end


`endif
