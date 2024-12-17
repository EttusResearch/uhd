"""Image builder: Build dispatch.

This is the entrypoint into the module from rfnoc_image_builder.py.

Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

RFNoC image builder: All the algorithms required to turn either a YAML
description or a GRC file into an rfnoc_image_core.v file.
"""

import glob
import logging
import os
import pathlib
import re
import shutil
import subprocess
import sys

import mako.lookup
from mako import exceptions

from . import grc, yaml_utils
from .builder_config import ImageBuilderConfig
from .template import Template

### DATA ######################################################################
# Directory under the FPGA repo where the device directories are

USRP3_LIB_RFNOC_DIR = os.path.join("usrp3", "lib", "rfnoc")

# Map device names to the corresponding directory under usrp3/top
DEVICE_DIR_MAP = {
    "x300": "x300",
    "x310": "x300",
    "e300": "e300",
    "e310": "e31x",
    "e320": "e320",
    "n300": "n3xx",
    "n310": "n3xx",
    "n320": "n3xx",
    "x400": "x400",
    "x410": "x400",
    "x440": "x400",
}

# Picks the default make target per device
DEVICE_DEFAULTTARGET_MAP = {
    "x300": "X300_HG",
    "x310": "X310_HG",
    "x410": "X410",
    "x440": "X440",
    "e310": "E310_SG3",
    "e320": "E320_1G",
    "n300": "N300_HG",
    "n310": "N310_HG",
    "n320": "N320_XG",
}

# Secure core RTL hierarchical path
SECURE_CORE_INST_PATH = {
    "x300": "x300_core/bus_int/rfnoc_sandbox_i/secure_image_core_i",
    "x310": "x300_core/bus_int/rfnoc_sandbox_i/secure_image_core_i",
    "e310": "e31x_core_inst/rfnoc_image_core_i/secure_image_core_i",
    "e320": "e320_core_i/rfnoc_sandbox_i/secure_image_core_i",
    "n300": "n3xx_core/rfnoc_sandbox_i/secure_image_core_i",
    "n310": "n3xx_core/rfnoc_sandbox_i/secure_image_core_i",
    "n320": "n3xx_core/rfnoc_sandbox_i/secure_image_core_i",
    "x400": "x4xx_core_i/rfnoc_image_core_i/secure_image_core_i",
    "x410": "x4xx_core_i/rfnoc_image_core_i/secure_image_core_i",
    "x440": "x4xx_core_i/rfnoc_image_core_i/secure_image_core_i",
}


def get_vivado_path(fpga_top_dir, args):
    """Return path to Vivado installation directory."""
    viv_path = None
    if args.get("vivado_path"):
        viv_path = args["vivado_path"]
    else:
        get_viv_path_cmd = '. ./setupenv.sh && echo "VIVADO_PATH=\$VIVADO_PATH"'
        try:
            output = subprocess.check_output(
                get_viv_path_cmd,
                shell=True,
                cwd=fpga_top_dir,
                text=True,
            )
            viv_path = re.search(r"VIVADO_PATH=(.*)", output).group(1)
        except subprocess.CalledProcessError:
            logging.error("Failed to get Vivado path from setupenv.sh")
            sys.exit(1)
    logging.debug("Using Vivado path: %s", viv_path)
    # check if viv_path is a directory
    if not os.path.isdir(viv_path):
        logging.error("Vivado path not found: %s", viv_path)
        sys.exit(1)
    return viv_path


def write_verilog(config, destination, args, template):
    """Generate Verilog output from a template.

    Mako templates from local template folder are used to generate the image
    core file. The template engine does not do any computation on the script
    parameter. Instead all necessary dependencies are resolved in this script
    to enforce early failure which is easier to track than errors in the
    template engine.
    :param config: ImageBuilderConfig derived from script parameter
    :param destination: Filepath to write to
    :param args: Dictionary of arguments for the code generation
    :param template: Mako template to use
    :return: None
    """
    template_dir = os.path.join(os.path.dirname(__file__), "templates")
    lookup = mako.lookup.TemplateLookup(directories=[template_dir])
    tpl_filename = os.path.join(template_dir, template)
    tpl = Template(filename=tpl_filename, lookup=lookup)
    try:
        block = tpl.render(**{"config": config, "args": args})
    except:
        print(exceptions.text_error_template().render())
        sys.exit(1)

    logging.debug("Writing output to %s", destination)
    with open(destination, "w", encoding="utf-8") as image_core_file:
        image_core_file.write(block)


def patch_netlist_constraints(device, build_dir):
    """Update constraints for the secure_image_core netlist.

    Updates the constraints for the secure_image_core netlist so that they will
    work when applied to the top-level design. The constraints output by Vivado
    assume the netlist is the top level. We need to adjust the constraints so
    that they apply to the correct level of the hierarchy when applied to the
    actual top level.

    :param device: The device to build for (x310, x410, e320, etc).
    :param build_dir: The build directory where all the build artifacts are stored.
    :return: None
    """
    if device not in SECURE_CORE_INST_PATH:
        logging.warning(f"Device {device} not found. Skipping secure core netlist patch.")
        return
    netlist_root = SECURE_CORE_INST_PATH[device]
    # Make a backup of "secure_image_core.xdc" in the artifact directory
    secure_xdc = os.path.join(build_dir, "secure_image_core.xdc")
    secure_xdc_bak = os.path.join(build_dir, "secure_image_core.xdc.bak")
    shutil.copy(secure_xdc, secure_xdc_bak)
    # Update all calls to "current_instance" to have the correct relative path.
    # Some examples:
    # current_instance                  -> current_instance {new_path}
    # current_instance -quiet           -> current_instance -quiet {new_path}
    # current_instance {path/in/design} -> current_instance {new_path/path/in/design}
    with open(secure_xdc, "r") as file:
        constraints = file.read()
    # Update path in calls that already include a path
    constraints = re.sub(
        r"^(\s*current_instance\s*(?:-quiet)?)\s*{(.*)}\s*$",
        rf"\1 {{{netlist_root}/\2}}",
        constraints,
        flags=re.MULTILINE,
    )
    # Substitute calls that don't include a path
    constraints = re.sub(
        r"^(\s*current_instance\s*(?:-quiet)?)\s*$",
        rf"\1 {{{netlist_root}}}",
        constraints,
        flags=re.MULTILINE,
    )
    # Set the current instance at the start and reset it at the end
    constraints = (
        f"current_instance {{{netlist_root}}}\n" + constraints + "\ncurrent_instance -quiet\n"
    )
    # Replace the constraints file with the new constraints
    with open(secure_xdc, "w") as file:
        file.write(constraints)


def gen_make_command(args, build_dir, device, use_secure_netlist, makefile_src_paths):
    """Generate the 'make' command that will build the desired bitfiles.

    This generates a full make command, including all the necessary options.
    """
    target = args["target"] if "target" in args else ""
    return (
        f"make {default_target(device, target)} "
        f"BUILD_DIR={build_dir} IMAGE_CORE_NAME={args['image_core_name']}"
        + (
            f" SECURE_CORE=1 SECURE_KEY={args.get('secure_key_file')}"
            if args.get("secure_core")
            else ""
        )
        + (" SECURE_NETLIST=1" if use_secure_netlist else "")
        + (" GUI=1 " if args.get("GUI") else "")
        + (" CHECK=1" if args.get("check_hdl") else "")
        + (" SYNTH=1" if args.get("synthesize_only") else "")
        + (
            " RFNOC_OOT_MAKEFILE_SRCS=" + "\\ ".join(makefile_src_paths)
            if makefile_src_paths
            else ""
        )
        + (" --jobs " + args["num_jobs"] if args.get("num_jobs") else "")
        + (" PROJECT=1" if args.get("save_project") else "")
        + (" IP_ONLY=1" if args.get("ip_only") else "")
        + (
            " BUILD_OUTPUT_DIR=" + os.path.abspath(args["build_output_dir"])
            if args.get("build_output_dir")
            else ""
        )
        + (
            " BUILD_IP_DIR=" + os.path.abspath(args["build_ip_dir"])
            if args.get("build_ip_dir")
            else ""
        )
    )


def build(fpga_top_dir, device, build_dir, use_secure_netlist, base_dir, **args):
    """Call FPGA toolchain to actually build the image.

    :param fpga_top_dir: A path to the FPGA device source directory
                         (/path/to/uhd/fpga/usrp3/top/<device>)
    :param device: The device to build for (x310, x410, e320, etc). The name must
                   match the top-level directory under usrp3/top.
    :param build_make_cmd: The make command to actually build the bitfile. Targets
                           like 'cleanall' and so on are all appended by this
                           function.
    :param build_dir: The build directory where all the build artifacts are stored.
                         (e.g., .../usrp3/x400/build-x410_XG_200_rfnoc_image_core/)
    :param use_secure_netlist: Boolean for whether the build with secure image
                               core netlist
    :param base_dir: The base directory
    :param **args: Additional options
                   target: The target to build (leave empty for default).
                   clean_all: passed to Makefile
                   GUI: passed to Makefile
                   save_project: passed to Makefile
                   ip_only: passed to Makefile
                   num_jobs: Number of make jobs to use
                   build_output_dir: Custom bitstream output directory
                   source: The source of the build (YAML or GRC file path)
                   include_paths: List of paths to OOT modules
                   that don't follow the OOT module layout. These paths must
                   point directly to a Makefile.srcs file.
    :return: exit value of build process
    """
    ret_val = 0
    build_dir = os.path.abspath(build_dir)
    cwd = os.getcwd()
    if not os.path.isdir(fpga_top_dir):
        logging.error("Not a valid directory: %s", fpga_top_dir)
        return 1
    makefile_src_paths = [
        os.path.join(os.path.abspath(os.path.normpath(x)), os.path.join("fpga", "Makefile.srcs"))
        for x in args.get("include_paths", [])
    ] + args.get("extra_makefile_srcs", [])
    setup_cmd = ". ./setupenv.sh "
    if "vivado_path" in args and args["vivado_path"]:
        setup_cmd += "--vivado-path=" + args["vivado_path"] + " "
    make_cmd = ""
    if "clean_all" in args and args["clean_all"]:
        make_cmd += "make cleanall && "
    make_cmd += gen_make_command(args, build_dir, device, use_secure_netlist, makefile_src_paths)

    if args.get("generate_only"):
        logging.info("Skipping build (--generate-only option given)!")
        logging.info("Use the following command to manually build the image: %s", make_cmd)
        return 0

    make_cmd = setup_cmd + "&& " + make_cmd
    build_output_dir = args.get("build_output_dir")
    if build_output_dir is None:
        build_output_dir = os.path.join(base_dir, "build")
    else:
        build_output_dir = os.path.abspath(build_output_dir)
    build_ip_dir = args.get("build_ip_dir")
    if build_ip_dir is None:
        build_ip_dir = os.path.join(base_dir, "build-ip")
    else:
        build_ip_dir = os.path.abspath(build_ip_dir)
    logging.info("Launching build with the following settings:")
    logging.info(" * FPGA Directory: %s", fpga_top_dir)
    logging.info(" * Build Artifacts Directory: %s", build_dir)
    logging.info(" * Build Output Directory: %s", build_output_dir)
    logging.info(" * Build IP Directory: %s", build_ip_dir)
    logging.info("Executing the following command: %s", make_cmd)
    my_env = os.environ.copy()
    ret_val = subprocess.call(make_cmd, shell=True, env=my_env, cwd=fpga_top_dir)
    if ret_val == 0 and args.get("secure_core"):
        patch_netlist_constraints(device, build_dir)
    logging.info("Build finished with return code %d.", ret_val)
    logging.info("It was launched with the following settings:")
    logging.info(" * FPGA Directory: %s", fpga_top_dir)
    logging.info(" * Build Artifacts Directory: %s", build_dir)
    logging.info(" * Build Output Directory: %s", build_output_dir)
    logging.info(" * Build IP Directory: %s", build_ip_dir)
    return ret_val


def target_dir(device):
    """Return target directory derived from chosen device.

    :param device: device to build for
    :return: target directory (relative path)
    """
    if not device.lower() in DEVICE_DIR_MAP:
        logging.error(
            "Unsupported device %s. Supported devices are %s", device, DEVICE_DIR_MAP.keys()
        )
        sys.exit(1)
    return DEVICE_DIR_MAP[device.lower()]


def default_target(device, target):
    """Select a valid target.

    If no target specified, selects the default building target based on the
    targeted device
    """
    if target is None:
        return DEVICE_DEFAULTTARGET_MAP.get(device.lower())
    return target


def load_module_yamls(include_paths):
    """Load all known descriptor YAMLs."""

    def load_module_descs(module_type):
        """Load separate block/module defs."""
        paths = [
            os.path.abspath(os.path.normpath(os.path.join(x, "rfnoc", module_type)))
            for x in include_paths
        ]
        logging.debug("Looking for %s descriptors in:", module_type[:-1])
        for path in paths:
            logging.debug("    %s", os.path.normpath(path))
        return yaml_utils.read_yaml_definitions(*paths)

    known_modules = {
        module_type: load_module_descs(module_type)
        for module_type in ("blocks", "modules", "transport_adapters", "includes")
    }
    # In the path structure, blocks are in blocks/, but the image builder wants
    # this to be called noc_blocks to match the YAML.
    known_modules["noc_blocks"] = known_modules.pop("blocks")
    return known_modules


def load_image_core_source(
    source,
    source_type,
    config_path,
    core_config_path,
    device,
    target,
    image_core_name,
    known_modules,
):
    """Load image configuration.

    The configuration can be either passed as RFNoC image configuration or as
    GNU Radio Companion grc. In latter case the grc files is converted into a
    RFNoC image configuration on the fly.
    :param source: Path to the source f ile
    :param source_type: Type of the source file (yaml or grc)
    :return: image configuration as dictionary
    """

    def get_image_core_name(config):
        icore_name = image_core_name or config.get("image_core_name")
        if not icore_name:
            icore_name = os.path.splitext(os.path.basename(source))[0]
        return icore_name

    if source_type == "yaml":
        config = yaml_utils.load_config_validate(source, config_path, True)
    # Otherwise, it's a GRC file
    elif source_type == "grc":
        config = yaml_utils.read_yaml(source)
        logging.info("Converting GNU Radio Companion file to image builder format")
        config = grc.convert_to_image_config(config, core_config_path, known_modules)
        # Run it through ruamel.yaml backwards and forwards to guarantee the
        # same object types as if we had loaded it directly from a YAML file.
        config = yaml_utils.reload_dict(config)
    else:
        logging.error("No configuration file provided")
        sys.exit(1)
    device = device or config.get("device")
    target = target or config.get("default_target")
    return config, device, get_image_core_name(config), target


def build_image(repo_fpga_path, config_path, device, **args):
    """Generate image dependent Verilog code and run FPGA toolchain, if requested.

    :param config: A dictionary containing the image configuration options.
                   This must obey the rfnoc_image_builder_args schema.
    :param repo_fpga_path: A path that holds the FPGA sources (/path/to/uhd/fpga).
                           Under this path, there should be a usrp3/top/
                           directory.
    :param device: The device to build for.
    :param **args: Additional options including
                   target: The target to build (leave empty for default).
                   generate_only: Do not build the bitfile after generating the code.
                   clean_all: passed to Makefile
                   GUI: passed to Makefile
                   save_project: passed to Makefile
                   ip_only: passed to Makefile
                   build_output_dir: passed to Makefile
                   include_paths: Paths to additional blocks
                   image_core_name: The name of the image core to be generated
                                    (e.g., usrp_x410_fpga_CG_400)
    :return: Exit result of build process or 0 if generate-only is given.
    """
    # We start by loading some core/standard files that are always needed.
    core_config_path = yaml_utils.get_core_config_path(config_path)

    # TODO does this work for both block yamls and HDL sources?
    include_paths = args.get("include_paths", []) + [config_path]
    logging.debug("Include paths: %s", ";".join(include_paths))
    # A list of all known module descriptors
    known_modules = load_module_yamls(include_paths)
    # resolve signature after modules have been loaded (the module YAML files
    # may contain signatures themselves)
    signatures_conf = yaml_utils.io_signatures(core_config_path, *list(known_modules.values()))

    # Next, load the image core configuration
    assert "source" in args
    config, device, image_core_name, target = load_image_core_source(
        args["source"],
        args["source_type"],
        config_path,
        core_config_path,
        device,
        args.get("target"),
        args.get("image_core_name"),
        known_modules,
    )
    logging.info("Selected device: %s", device)
    assert image_core_name
    args["image_core_name"] = image_core_name
    logging.debug("Image core name: %s", image_core_name)
    if "base_dir" in args and args["base_dir"]:
        base_dir = args["base_dir"]
    else:
        base_dir = os.getcwd()
    args.pop("base_dir", None)
    logging.debug("Using base directory: %s", base_dir)
    if "build_dir" in args and args["build_dir"]:
        build_dir = args["build_dir"]
    else:
        build_dir = os.path.join(base_dir, "build-" + image_core_name)
    args.pop("build_dir", None)
    logging.debug("Using build artifacts directory: %s", build_dir)
    if pathlib.Path(build_dir).is_file():
        logging.error("Build artifacts directory is pointing to a file: %s", build_dir)
        return 1
    elif pathlib.Path(build_dir).is_dir():
        logging.info("Build artifacts directory already exists (contents will be overwritten).")
    else:
        logging.debug("Creating build artifacts directory...")
        pathlib.Path(build_dir).mkdir(parents=True, exist_ok=True)
    image_core_path = os.path.join(build_dir, "rfnoc_image_core.sv")
    image_core_header_path = os.path.join(build_dir, "rfnoc_image_core.vh")
    secure_core_path = os.path.join(build_dir, "secure_image_core.sv")
    dts_path = os.path.join(build_dir, "device_tree.dts")
    makefile_srcs_path = os.path.join(build_dir, "Makefile.inc")
    fpga_top_dir = os.path.join(
        yaml_utils.get_top_path(os.path.abspath(repo_fpga_path)), target_dir(device)
    )

    # If desired, dump the configuration into the build directory
    if args.get("dump_config"):
        yaml_utils.write_yaml(config, os.path.join(build_dir, f"{image_core_name}.yml"))

    device_conf = yaml_utils.IOConfig(
        yaml_utils.device_config(core_config_path, device), signatures_conf
    )
    if not hasattr(device_conf, "top_dir"):
        setattr(device_conf, "top_dir", fpga_top_dir)
    for module_type, defs in known_modules.items():
        require_schema = None if module_type == "includes" else "rfnoc_modtool_args"
        yaml_utils.resolve_io_signatures(defs, signatures_conf, require_schema)

    # Load the image core config
    try:
        builder_conf = ImageBuilderConfig(config, known_modules, device_conf, include_paths)
    except (ValueError, KeyError) as e:
        logging.error("Error parsing image configuration: %s", e)
        return 1
    if not args.get("continue_on_warnings"):
        if builder_conf.warnings or builder_conf.errors:
            logging.error(
                "Image configuration contains issues: Skipping build. "
                "Use --ignore-warnings to build despite warnings."
            )
            return 1
    # Generate verilog output
    output_list = (
        [
            ("rfnoc_image_core.sv.mako", image_core_path, builder_conf),
            ("rfnoc_image_core.vh.mako", image_core_header_path, builder_conf),
            ("Makefile.inc.mako", makefile_srcs_path, builder_conf),
        ]
        + (
            [
                ("secure_image_core.sv.mako", secure_core_path, builder_conf.secure_config),
            ]
            if hasattr(builder_conf, "secure_image_core")
            else []
        )
        + (
            [
                ("device_tree.dts.mako", dts_path, builder_conf),
            ]
            if getattr(builder_conf.device, "generate_dts", False)
            else []
        )
    )
    reuse = args.get("reuse", False)
    for tpl, path, conf in output_list:
        if not (reuse and pathlib.Path(path).exists()):
            write_verilog(conf, path, args, template=tpl)
        else:
            logging.info(
                "Skipping generation of %s: File already exists and " "reuse was requested.", path
            )
    # Are we generating the secure image core?
    netlist_files = config.get("secure_image_core", {}).get("netlist_files")
    if args.get("secure_core"):
        secure_core_def = builder_conf.get_secure_core_def()
        secure_core_yml = args.get("secure_core")
        new_netlist_files = [
            "secure_image_core.vp",
            "secure_image_core.xdc",
        ]
        secure_core_def["secure_image_core"]["netlist_files"] = new_netlist_files
        logging.info("Writing secure core YAML to %s", secure_core_yml)
        yaml_utils.write_yaml(secure_core_def, secure_core_yml)
        if not args.get("secure_key_file"):
            viv_path = get_vivado_path(fpga_top_dir, args)
            key_dir = os.path.join(viv_path, "data", "pubkey")
            keyfiles = sorted(glob.glob(os.path.join(key_dir, "xilinxt*active.v")))
            if not keyfiles:
                logging.error("No key file found in %s", key_dir)
                return 1
            logging.info("No key file provided: Using default Vivado key from %s.", keyfiles[-1])
            with open(keyfiles[-1], "r") as f:
                write_verilog(
                    {},
                    os.path.join(build_dir, "keyfile.v"),
                    {"key_info": f.read()},
                    "keyfile.v.mako",
                )
            args["secure_key_file"] = os.path.join(build_dir, "keyfile.v")
    elif netlist_files:
        # The YAML includes a set of secure image netlist files. Copy them to
        # the artifact directory.
        netlist_dir = os.path.dirname(args.get("yaml_path"))
        for file in netlist_files:
            try:
                # Assume netlist file path is relative to YAML file
                source = os.path.join(netlist_dir, file)
                destination = os.path.normpath(os.path.join(build_dir, os.path.basename(file)))
                logging.info(f"Copying {file} to {destination}")
                shutil.copy(source, destination)
            except:
                logging.error(f"Failed to copy {source} to artifact directory")
                return 1

    ret_val = build(fpga_top_dir, device, build_dir, bool(netlist_files), base_dir, **args)
    if args.get("generate_only"):
        return ret_val

    if ret_val == 0 and args.get("secure_core"):
        # We built the secure image core netlist. Copy the resulting netlist
        # and constraint files.
        secure_core_yml_dir = os.path.dirname(args.get("secure_core"))
        file_list = [os.path.join(build_dir, x) for x in new_netlist_files]
        for file in file_list:
            try:
                logging.info(f"Copying {file} to {secure_core_yml_dir}")
                shutil.copy(file, secure_core_yml_dir)
            except:
                logging.warning(f"Unable to copy netlist file {file}")

    return ret_val
