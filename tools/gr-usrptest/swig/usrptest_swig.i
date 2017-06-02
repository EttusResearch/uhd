/* -*- c++ -*- */

#define USRPTEST_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "usrptest_swig_doc.i"

%{
#include "usrptest/measurement_sink_f.h"
%}


%include "usrptest/measurement_sink_f.h"
GR_SWIG_BLOCK_MAGIC2(usrptest, measurement_sink_f);
