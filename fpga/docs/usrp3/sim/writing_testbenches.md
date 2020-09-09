# Writing a Testbench

Writing a unit test or system level test is easy with the Vivado makefile infrastructure! 
Most of the overhead of building and running a testbench is handled by the build tools. 
Even recurring tasks like reporting and monitoring are implemented by framework libraries.

Each executable FPGA unit test must have the following components:

1. \ref md_usrp3_sim_writing_sim_makefile "A simulation Makefile"
2. \ref md_usrp3_sim_writing_sim_top "A top-level simulation module"

If creating a testbench for an RFNoC block, it is recommended to use the RFNoC
ModTool (rfnoc_create_verilog.py) to create a template Makefile and testbench
for your RFNoC block.

It is encouraged to use (and create) reusable libraries in product specific 
test benches. Libraries can provide macros, modules, tasks and functions for
ease-of-use with particular protocols and subsystems.

See the following manual pages for information about how to create the Makefile
and top-level simulation file.

- \subpage md_usrp3_sim_writing_sim_makefile "Writing a Simulation Makefile"
- \subpage md_usrp3_sim_writing_sim_top "Writing a Top-level Simulation Module"