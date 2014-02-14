
The design files are located at
/home/ianb/fpgapriv_usrp3/fpgapriv/usrp3/top/b250/coregen:

   - ddr3_interface_fast.veo:
        veo template file containing code that can be used as a model
        for instantiating a CORE Generator module in a HDL design.

   - ddr3_interface_fast.xco:
       CORE Generator input file containing the parameters used to
       regenerate a core.

   - ddr3_interface_fast_flist.txt:
        Text file listing all of the output files produced when a customized
        core was generated in the CORE Generator.

   - ddr3_interface_fast_readme.txt:
        Text file indicating the files generated and how they are used.

   - ddr3_interface_fast_xmdf.tcl:
        ISE Project Navigator interface file. ISE uses this file to determine
        how the files output by CORE Generator for the core can be integrated
        into your ISE project.

   - ddr3_interface_fast.csv:
        Includes the pin out information which is used as support file
        for PlanAhead.

   - ddr3_interface_fast.gise and ddr3_interface_fast.xise:
        ISE Project Navigator support files. These are generated files and
        should not be edited directly.

   - ddr3_interface_fast directory.

In the ddr3_interface_fast directory, three folders are created:
   - docs:
        This folder contains MIG user guide.

   - example_design:
        This folder includes script files to implement and simulate the design.
        This includes the traffic generator RTL modules and example_top module.

   - user_design:
        This folder includes the all RTL modules of controller, phy and
        user interface RTL modules. UCF file is provided.

The example_design and user_design folders contain several other folders
and files. All these output folders are discussed in more detail in
MIG user guide (ug586_7Series_MIS.pdf) located in docs folder.
    