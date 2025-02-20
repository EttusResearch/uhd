# Running a Testbench

Each executable testbench has its own Makefile that automatically pulls in support
for all supported simulators. The build infrastructure supports the following simulators:

- Xilinx Vivado (XSim)
- Siemens ModelSim (Requires an additional license)
- Siemens Questa (Requires an additional license)

In general running ``make <sim_target>`` will run the
simulation and report results in the console. Running ``make help`` will print out
all supported simulator targets. Currently, the following targets will work:

    ipclean:    Cleanup all IP intermediate files
    clean:      Cleanup all simulator intermediate files
    cleanall:   Cleanup everything!
    ip:         Generate the IP required for this simulation
    xsim:       Run the simulation using the Xilinx Vivado Simulator
    xclean:     Cleanup Xilinx Vivado Simulator intermediate files
    vsim:       Run the simulation using ModelSim from inside Vivado
    qsim:       Run the simulation using Questa
    msim:       Run the simulation using ModelSim
    vcom:       Run ModelSim/Questa compiler to compile HDL files
    vclean:     Cleanup ModelSim/Questa intermediate files

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


## Using Siemens ModelSim or Questa

### Setting Up ModelSim or QuestaSim

ModelSim and Questa are third-party simulation tools that are compatible with
Vivado and the USRP FPGA build infrastructure.

Use the following one-time setup to install and configure ModelSim on your system

- Install the simulator of your choice from the
  [Siemens](http://www.siemens.com/) website. It is recommended that you
  install it to the default location (``/opt/mentor/modelsim``,
  ``/opt/mentor/questasim``)
- Run the setupenv.sh script for the USRP product that you want to simulate:

  ``$ source <repo>/fpga/usrp3/top/<product>/setupenv.sh``

  This step is required even if the simulation is not for a specific device
  because the toolchain requires an FPGA part number to load simulation
  models.

  If setup was successful, then you should see something like the following:

      Setting up X3x0 FPGA build environment (64-bit)...
      - Vivado: Found (/opt/Xilinx/Vivado/2021.1/bin)
      - ModelSim/Questa: Found (/opt/mentor/modelsim/modeltech/bin)
      - ModelSim/Questa Compiled Libs: Found (/opt/Xilinx/Vivado/2021.1/modelsim)

      Environment successfully initialized.

- If setupenv.sh did not report finding your simulator, then you may need to
  indicate where it is installed using the ``--modelsim-path`` argument:

  ``$ source <repo>/fpga/usrp3/top/<product>/setupenv.sh --modelsim-path=/path/to/modelsim``

- If setupenv.sh could not find your simulation libraries, then you will need
  to either build them or set an environment variable to help the tool locate
  them. To build build the Xilinx simulation libraries, run the following:

  ``$ build_simlibs``

  This will build the simulation libraries and put them, by default, in the
  Vivado installation directory. For other options, run:

  ``$ build_simlibs --help``

  If they have already been built, you can set the ``SIM_COMPLIBDIR``
  environment variable to point to their location and rerun setupenv.sh.

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

### Simulating with ModelSim

To run the simulation using ModelSim natively, the process is the same as
above, except use the `modelsim` make target.

``$ make msim``

This calls into Vivado only if Xilinx IP needs to be generated. Otherwise it
calls ModelSim directly without invoking Vivado. This leads to significantly
faster compile and load times for the simulation. However, this method does not
work with all testbenches because some Xilinx IP requires special handling by
Vivado.

To load the simulation into the ModelSim GUI for debugging, add the GUI option.

``$ make msim GUI=1``

You may also need to specify special arguments or libraries to use with
ModelSim. These can be added using the `MODELSIM_ARGS` and `MODELSIM_LIBS`
arguments. For example:

``$ make msim MODELSIM_ARGS="-t 1fs" MODELSIM_LIBS="secureip xpm"``

ARGS are simply appended to the ModelSim `vsim` command line invocation. LIBS
are added to the "-L" vsim command line argument. These can also be added to
the Makefile for the testbench.

### Simulating with QuestaSim

For Questa, use the ``qsim`` target instead of ``msim``. For example:

``$ make qsim GUI=1``

Because the commands for Questa are very similar to ModelSim, you can also use
the ``msim`` target with Questa, but this will use the classic GUI and the
legacy +acc mode with vopt. +acc mode is no longer recommended with Questa.

### Compiling the Simulation with ModelSim or Questa

To compile your code in ModelSim or Questa, without running the simulation,
use the `vcom` target.

``$ make vcom``

This can be used to quickly check for syntax errors, or to recompile your code
prior to restarting a simulation from within the GUI.

Using the `msim`/`qsim` and `vcom` targets allows for more rapid simulation
iterations than when using Vivado.


## Troubleshooting

#### Vivado Not Found

If running the setupenv.sh script returns an error like the following:

    Vivado: Not found! (ERROR.. Builds and simulations will not work)

Then it is possible that Vivado was not installed or it was not installed in
the default location. If Vivado is installed in a non-default location, just
run the following:

``$ source <repo>/fpga/usrp3/top/<product>/setupenv.sh --vivado-path=<PATH>``

#### ModelSim/Questa Not Found

If running the setupenv.sh script returns an error like the following:

    Setting up a 64-bit FPGA build environment for the USRP-X3x0...
    - Vivado: Found (/opt/Xilinx/Vivado/2019.1/bin)
    - ModelSim/Questa: Not found! (WARNING: Simulations with ModelSim/Questa might not work!)

    Environment successfully initialized.

Or something like this, even when ModelSim or Questa is installed:

    Setting up a 64-bit FPGA build environment for the USRP-X3x0...
    - Vivado: Found (/opt/Xilinx/Vivado/2019.1/bin)

    Environment successfully initialized.

Then is possible that ModelSim or Questa was not properly installed, or that it
was not installed in the default location. If ModelSim or Questa is installed
in a non-default location, run the following:

    $ source <repo>/fpga/usrp3/top/<product>/setupenv.sh --modelsim-path=<PATH>

#### ModelSim Simulation Libraries Not Found

If running the setupenv.sh script returns an error like the following:

    Setting up a 64-bit FPGA build environment for the USRP-X3x0...
    - Vivado: Found (/opt/Xilinx/Vivado/2019.1/bin)
    - ModelSim/Questa: Found (/opt/mentor/modelsim/modeltech/bin)
    - ModelSim/Questa Compiled Libs: Not found! (Run build_simlibs to generate them.)

    Environment successfully initialized.

Then you need to build the simulation libraries. To do this, run:

    $ build_simlibs

This will build the simulation libraries and put them, by default, in the
Vivado installation directory. For other options, run:

    $ build_simlibs --help

Note that it is okay if some of the Xilinx libraries fail to compile without
errors. This is only a problem if you intend to simulate one of those
libraries. If not, then those errors can be ignored.
