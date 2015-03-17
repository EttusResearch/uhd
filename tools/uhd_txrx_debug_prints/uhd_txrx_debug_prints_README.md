UHD TX/RX DEBUG PRINTS
======================

A tool for extensive debugging with UHD.

Install
-------
Activate it by ticking `UHD_TXRX_DEBUG_PRINTS` in cmake-gui for your UHD installation. Then recompile and reinstall UHD.

Use
---
Run your application and pipe stderr to a file.
this is mostly done by <br>
`app_call 2> dbg_print_file.txt`<br>
After finishing the application offline processing of the gathered data is done with a python script called<br>
`uhd_txrx_debug_prints_graph.py`<br>
There are a lot of functions that help to preprocess your data and that describe the actual meaning of all the data points. in the end though, it comes down to the users needs what he wants to plot and see.