# Running a Testbench

Each executable testbench has its own Makefile that automatically pulls in support
for all supported simulators. The build infrastructure supports the following simulators:

 - Xilinx Vivado (XSim)
 - Mentor Graphics ModelSim (may require an additional license)


In general running ``make <sim_target>`` will run the
simulation and report results in the console. Running ``make help`` will print out
all supported simulator targets. Currently, the following targets will work:

    Supported Targets:
    ipclean:    Cleanup all IP intermediate files
    clean:      Cleanup all simulator intermediate files
    cleanall:   Cleanup everything!
    xsim:       Run the simulation using the Xilinx Vivado Simulator
    xclean:     Cleanup Xilinx Vivado Simulator intermediate files
    vsim:       Run the simulation using Modelsim
    vclean:     Cleanup Modelsim intermediate files


## Using Xilinx Vivado XSim

XSim is the built-in simulator in the Xilinx Vivado toolchain. If you already met the
prerequisites for building an FPGA image, then you don't need to install anything else.

Follow these steps to run a testbench:

 - Navigate to the directory that contains the top level testbench and Makefile
 - Run the setenv.sh script for the USRP product that you are trying to simulate

   ``$ source <repo>/usrp3/top/<product>/setupenv.sh``

   This step is required even if the simulation is generic because the toolchain requires
   an FPGA part number to load simulation models.
 - Run the simulator specific target

   ``$ make xsim``


## Using Mentor Graphics ModelSim

ModelSim is a third-party simulation tool that is compatible with Vivado and the USRP
FPGA build infrastructure.

Use the following one-time setup to install and configure Modelsim on your system

 - Install Modelsim from the [Mentor Graphics](http://www.mentor.com/) website. 
   It is recommended that you install it to the default location (/opt/mentor/modelsim)
 - Run the setenv.sh script for the USRP product that you are trying to simulate

   ``$ source <repo>/usrp3/top/<product>/setupenv.sh``

   This step is required even if the simulation is generic because the toolchain requires
   an FPGA part number to load simulation models.
 - Build the Xilinx simulation libraries
   ``$ build_simlibs``


To validate that everything was install properly run ``setupenv.sh`` again. You should see the following

    Setting up X3x0 FPGA build environment (64-bit)...
    - Vivado: Found (/opt/Xilinx/Vivado/2014.4/bin)
    - Modelsim: Found (/opt/mentor/modelsim/modeltech/bin)
    - Modelsim Compiled Libs: Found (/opt/Xilinx/Vivado/2014.4/modelsim)
    
    Environment successfully initialized.

Follow these steps to run a testbench:

 - Navigate to the directory that contains the top level testbench and Makefile
 - Run the setenv.sh script for the USRP product that you are trying to simulate

   ``$ source <repo>/usrp3/top/<product>/setupenv.sh``

   This step is required even if the simulation is generic because the toolchain requires
   an FPGA part number to load simulation models.
 - Run the simulator specific target

   ``$ make vsim``


## Troubleshooting

#### Vivado Not Found

If running the setupenv.sh script return an error like the following:

    Vivado: Not found! (ERROR.. Builds and simulations will not work)

then it is possible that Vivado was not installed or it was not installed in the default
location. If Vivado is installed in a non-default location, just run the following:

  ``$ source <repo>/usrp3/top/<product>/setupenv.sh --vivado-path=<PATH>``

#### Modelsim Not Found

If running the setupenv.sh script return an error like the following:

    Setting up X3x0 FPGA build environment (64-bit)...
    - Vivado: Found (/opt/Xilinx/Vivado/2014.4/bin)
    - Modelsim: Not found! (WARNING.. Simulations with vsim will not work)
    
    Environment successfully initialized.

or something like this (even when Modelsim is installed)

    Setting up X3x0 FPGA build environment (64-bit)...
    - Vivado: Found (/opt/Xilinx/Vivado/2014.4/bin)
    
    Environment successfully initialized.

then it is possible that Modelsim was not installed or it was not installed in the default
location. If Modelsim is installed in a non-default location, just run the following:

  ``$ source <repo>/usrp3/top/<product>/setupenv.sh --modelsim-path=<PATH>``

#### Modelsim Simulation Libraries Not Found

If running the setupenv.sh script return an error like the following:

    Setting up X3x0 FPGA build environment (64-bit)...
    - Vivado: Found (/opt/Xilinx/Vivado/2014.4/bin)
    - Modelsim: Found (/opt/mentor/modelsim/modeltech/bin)
    - Modelsim Compiled Libs: Not found! (Run build_simlibs to generate them.)
    
    Environment successfully initialized.

just run the following

    $ build_simlibs