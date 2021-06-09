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
    ip:         Generate the IP required for this simulation
    xsim:       Run the simulation using the Xilinx Vivado Simulator
    xclean:     Cleanup Xilinx Vivado Simulator intermediate files
    vsim:       Run the simulation using ModelSim simulator via Vivado 
    modelsim:   Run the simulation using ModelSim without Vivado
    vlint:      Compile the simulation using ModelSim
    vclean:     Cleanup ModelSim intermediate files


## Using Xilinx Vivado XSim

XSim is the built-in simulator in the Xilinx Vivado toolchain. If you already met the
prerequisites for building an FPGA image, then you don't need to install anything else.

Follow these steps to run a testbench:

 - Navigate to the directory that contains the top level testbench and Makefile
 - Run the setupenv.sh script for the USRP product that you are trying to simulate

   ``$ source <repo>/fpga/usrp3/top/<product>/setupenv.sh``

   This step is required even if the simulation is generic because the toolchain requires
   an FPGA part number to load simulation models.
 - Run the simulator specific target

   ``$ make xsim``


## Using Mentor Graphics ModelSim

### Setting Up ModelSim

ModelSim is a third-party simulation tool that is compatible with Vivado and the USRP
FPGA build infrastructure.

Use the following one-time setup to install and configure ModelSim on your system

 - Install ModelSim from the [Mentor Graphics](http://www.mentor.com/) website. 
   It is recommended that you install it to the default location (/opt/mentor/modelsim)
 - Run the setupenv.sh script for the USRP product that you are trying to simulate

   ``$ source <repo>/fpga/usrp3/top/<product>/setupenv.sh``

   This step is required even if the simulation is generic because the toolchain requires
   an FPGA part number to load simulation models.
 - Build the Xilinx simulation libraries

   ``$ build_simlibs``


To validate that everything was installed properly run ``setupenv.sh`` again. You should see the following

    Setting up X3x0 FPGA build environment (64-bit)...
    - Vivado: Found (/opt/Xilinx/Vivado/2019.1/bin)
    - Modelsim: Found (/opt/mentor/modelsim/modeltech/bin)
    - Modelsim Compiled Libs: Found (/opt/Xilinx/Vivado/2019.1/modelsim)
    
    Environment successfully initialized.

### Simulating with ModelSim through Vivado

Follow these steps to run a testbench using ModelSim with Vivado:

 - Navigate to the directory that contains the top level testbench and Makefile
 - Run the setupenv.sh script for the USRP product that you are trying to simulate

   ``$ source <repo>/fpga/usrp3/top/<product>/setupenv.sh``

   This step is required even if the simulation is generic because the toolchain requires
   an FPGA part number to load simulation models.
 - Run the simulator specific target

   ``$ make vsim``

Using this method launches Vivado and uses Vivado to launch ModelSim. This
ensures that Xilinx IP is properly included in the simulation.

To run the simulation in the Vivado GUI for debugging, add the GUI option:

``$ make vsim GUI=1``

### Simulating with ModelSim Natively

To run the simulation using ModelSim natively, the process is the same as
above, except use the `modelsim` make target.

``$ make modelsim``

This calls into Vivado only if Xilinx IP needs to be generated. Otherwise it
calls ModelSim directly without invoking Vivado. This leads to significantly
faster compile and load times for the simulation. However, this method does not
work with all testbenches because some Xilinx IP requires special handling by
Vivado.

To load the simulation into the ModelSim GUI for debugging, add the GUI option.

``$ make modelsim GUI=1``

You may also need to specify special arguments or libraries to use with
ModelSim. These can be added using the `MODELSIM_ARGS` and `MODELSIM_LIBS`
arguments. For example:

``$ make modelsim MODELSIM_ARGS="-t 1fs" MODELSIM_LIBS="secureip xpm"``

ARGS are simply appended to the ModelSim `vsim` command line invocation. LIBS
are added to the "-L" vsim command line argument. These can also be added to
the Makefile for the testbench.

### Compiling the Simulation with ModelSim

To compile your code in ModelSim, use the `vlint` target.

``$ make vlint``

This can be used to quickly check for syntax errors, or to recompile your code
prior to restarting a simulation from within ModelSim.

Using the `modelsim` and `vlint` targets allows for very rapid simulation
iterations.


## Troubleshooting

#### Vivado Not Found

If running the setupenv.sh script returns an error like the following:

    Vivado: Not found! (ERROR.. Builds and simulations will not work)

then it is possible that Vivado was not installed or it was not installed in the default
location. If Vivado is installed in a non-default location, just run the following:

``$ source <repo>/fpga/usrp3/top/<product>/setupenv.sh --vivado-path=<PATH>``

#### ModelSim Not Found

If running the setupenv.sh script returns an error like the following:

    Setting up a 64-bit FPGA build environment for the USRP-X3x0...
    - Vivado: Found (/opt/Xilinx/Vivado/2019.1/bin)
    - Modelsim: Not found! (WARNING.. Simulations with vsim will not work)
    
    Environment successfully initialized.

or something like this (even when Modelsim is installed)

    Setting up a 64-bit FPGA build environment for the USRP-X3x0...
    - Vivado: Found (/opt/Xilinx/Vivado/2019.1/bin)
    
    Environment successfully initialized.

then it is possible that Modelsim was not installed or it was not installed in the default
location. If Modelsim is installed in a non-default location, just run the following:

``$ source <repo>/fpga/usrp3/top/<product>/setupenv.sh --modelsim-path=<PATH>``

#### ModelSim Simulation Libraries Not Found

If running the setupenv.sh script returns an error like the following:

    Setting up a 64-bit FPGA build environment for the USRP-X3x0...
    - Vivado: Found (/opt/Xilinx/Vivado/2019.1/bin)
    - Modelsim: Found (/opt/mentor/modelsim/modeltech/bin)
    - Modelsim Compiled Libs: Not found! (Run build_simlibs to generate them.)
    
    Environment successfully initialized.

Just run the following:

`$ build_simlibs`
