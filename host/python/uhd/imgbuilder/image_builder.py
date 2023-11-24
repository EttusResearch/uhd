"""
Copyright 2019 Ettus Research, A National Instrument Brand

SPDX-License-Identifier: GPL-3.0-or-later

RFNoC image builder: All the algorithms required to turn either a YAML
description or a GRC file into an rfnoc_image_core.v file.
"""

import logging
import os
import sys
import pathlib

import mako.lookup
import mako.template
from mako import exceptions

from . import yaml_utils
from .builder_config import ImageBuilderConfig

### DATA ######################################################################
# Directory under the FPGA repo where the device directories are

USRP3_LIB_RFNOC_DIR = os.path.join('usrp3', 'lib', 'rfnoc')

# Path to the system's bash executable
BASH_EXECUTABLE = '/bin/bash' # FIXME this should come from somewhere

# Map device names to the corresponding directory under usrp3/top
DEVICE_DIR_MAP = {
    'x300': 'x300',
    'x310': 'x300',
    'e300': 'e300',
    'e310': 'e31x',
    'e320': 'e320',
    'n300': 'n3xx',
    'n310': 'n3xx',
    'n320': 'n3xx',
    'x400': 'x400',
    'x410': 'x400',
    'x440': 'x400',
}

# Picks the default make target per device
DEVICE_DEFAULTTARGET_MAP = {
    'x300': 'X300_HG',
    'x310': 'X310_HG',
    'e310': 'E310_SG3',
    'e320': 'E320_1G',
    'n300': 'N300_HG',
    'n310': 'N310_HG',
    'n320': 'N320_XG',
}

def write_verilog(config, destination, args, template):
    """
    Generates rfnoc_image_core.v file for the device.

    Mako templates from local template folder are used to generate the image
    core file. The template engine does not do any computation on the script
    parameter. Instead all necessary dependencies are resolved in this script
    to enforce early failure which is easier to track than errors in the
    template engine.
    :param config: ImageBuilderConfig derived from script parameter
    :param destination: Filepath to write to
    :param args: Dictionary of arguments for the code generation
    :return: None
    """
    template_dir = os.path.join(os.path.dirname(__file__), "templates")
    lookup = mako.lookup.TemplateLookup(directories=[template_dir])
    tpl_filename = os.path.join(template_dir, template)
    tpl = mako.template.Template(
        filename=tpl_filename,
        lookup=lookup,
        strict_undefined=True)
    try:
        block = tpl.render(**{ "config": config, "args": args, })
    except:
        print(exceptions.text_error_template().render())
        sys.exit(1)

    logging.info("Writing output to %s", destination)
    with open(destination, "w", encoding="utf-8") as image_core_file:
        image_core_file.write(block)


def gen_make_command(args, artifact_dir, device):
    """
    Generates the 'make' command that will build the desired bitfiles, including
    all the necessary options.
    """
    target = args["target"] if "target" in args else ""
    make_cmd = \
        f"make {default_target(device, target)} " \
        f"ARTIFACT_DIR={artifact_dir} " \
        f"IMAGE_CORE_NAME={args['image_core_name']} " \
        f"{'GUI=1' if 'GUI' in args and args['GUI'] else ''} "
    return make_cmd


def build(fpga_top_dir, device, artifact_dir, **args):
    """
    Call FPGA toolchain to actually build the image

    :param fpga_top_dir: A path to the FPGA device source directory
                         (/path/to/uhd/fpga/usrp3/top/<device>)
    :param device: The device to build for (x310, x410, e320, etc). The name must
                   match the top-level directory under usrp3/top.
    :param build_make_cmd: The make command to actually build the bitfile. Targets
                           like 'cleanall' and so on are all appended by this
                           function.
    :param artifact_dir: The build directory where all the build artifacts are
                         stored.
                         (e.g., .../usrp3/x400/build-x410_XG_200_rfnoc_image_core/)
    :param **args: Additional options
                   target: The target to build (leave empty for default).
                   clean_all: passed to Makefile
                   GUI: passed to Makefile
                   save_project: passed to Makefile
                   ip_only: passed to Makefile
                   num_jobs: Number of make jobs to use
                   build_base_dir: Custom base directory for FPGA builds
                   build_output_dir: Custom bitstream output directory
                   source: The source of the build (YAML or GRC file path)
                   include_paths: List of paths to OOT modules
                   that don't follow the OOT module layout. These paths must
                   point directly to a Makefile.srcs file.
    :return: exit value of build process
    """
    ret_val = 0
    artifact_dir = os.path.abspath(artifact_dir)
    cwd = os.getcwd()
    build_base_dir = fpga_top_dir
    build_output_dir = os.path.join(build_base_dir, "build")
    if not os.path.isdir(fpga_top_dir):
        logging.error("Not a valid directory: %s", fpga_top_dir)
        return 1
    makefile_src_paths = [
        os.path.join(
            os.path.abspath(os.path.normpath(x)),
            os.path.join('fpga', 'Makefile.srcs'))
        for x in args.get("include_paths", [])
    ] + args.get("extra_makefile_srcs", [])
    logging.debug("Temporarily changing working directory to %s", fpga_top_dir)
    os.chdir(fpga_top_dir)
    setup_cmd = ". ./setupenv.sh "
    if "vivado_path" in args and args["vivado_path"]:
        setup_cmd += "--vivado-path=" + args["vivado_path"] + " "
    make_cmd = ""
    if "clean_all" in args and args["clean_all"]:
        make_cmd += "make cleanall && "
    make_cmd += gen_make_command(args, artifact_dir, device)
    if makefile_src_paths:
        make_cmd += " RFNOC_OOT_MAKEFILE_SRCS=" + "\\ ".join(makefile_src_paths)
    if "num_jobs" in args and args["num_jobs"]:
        make_cmd = make_cmd + " --jobs " + args["num_jobs"]
    if "GUI" in args and args["GUI"]:
        make_cmd = make_cmd + " GUI=1"
    if "save_project" in args and args["save_project"]:
        make_cmd = make_cmd + " PROJECT=1"
    if "ip_only" in args and args["ip_only"]:
        make_cmd = make_cmd + " IP_ONLY=1"
    if "build_base_dir" in args and args["build_base_dir"]:
        make_cmd = make_cmd + " BUILD_BASE_DIR=" + args["build_base_dir"]
        build_base_dir = args["build_base_dir"]
    if "build_output_dir" in args and args["build_output_dir"]:
        make_cmd = make_cmd + " BUILD_OUTPUT_DIR=" + args["build_output_dir"]
        build_output_dir = args["build_output_dir"]

    if args.get('generate_only'):
        logging.info("Skip build (generate only option given)")
        logging.info("Use the following command to build the image: %s", make_cmd)
        return 0

    make_cmd = setup_cmd + " && " + make_cmd
    logging.info("Launching build with the following settings:")
    logging.info(" * FPGA Directory: %s", fpga_top_dir)
    logging.info(" * Build Base Directory: %s", build_base_dir)
    logging.info(" * Artifact directory: %s", artifact_dir)
    logging.info(" * Build Output Directory: %s", build_output_dir)
    # Wrap it into a bash call:
    make_cmd = f'{BASH_EXECUTABLE} -c "{make_cmd}"'
    logging.debug("Executing the following command: %s", make_cmd)
    ret_val = os.system(make_cmd)
    os.chdir(cwd)
    return ret_val

def target_dir(device):
    """
    Target directory derived from chosen device
    :param device: device to build for
    :return: target directory (relative path)
    """
    if not device.lower() in DEVICE_DIR_MAP:
        logging.error("Unsupported device %s. Supported devices are %s",
                      device, DEVICE_DIR_MAP.keys())
        sys.exit(1)
    return DEVICE_DIR_MAP[device.lower()]

def default_target(device, target):
    """
    If no target specified, selects the default building target based on the
    targeted device
    """
    if target is None:
        return DEVICE_DEFAULTTARGET_MAP.get(device.lower())
    return target

def generate_output_path(
        output_path,
        name,
        source,
        file_ext=None,
        fname_format=None):
    """
    Creates the path where an output image core file gets to be stored.

    output_path: If not None, this is returned
    name: Device name string, used to generate default file name
    source: Otherwise, this path is returned, combined with a default file name
    """
    if output_path is not None:
        return os.path.splitext(output_path)[0] + file_ext
    source = os.path.split(os.path.abspath(os.path.normpath(source)))[0]
    return os.path.join(source, fname_format.format(name))


def load_module_yamls(config_path, include_paths):
    """
    Loads all known block, module, transport adapter, or include YAMLs that are
    available to us.
    """
    # Load separate block/module defs
    def load_module_descs(module_type):
        paths = yaml_utils.collect_module_paths(
                config_path, include_paths, module_type)
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


def build_image(config, repo_fpga_path, config_path, device, **args):
    """
    Generate image dependent Verilog code and run FPGA toolchain, if
    requested.

    :param config: A dictionary containing the image configuration options.
                   This must obey the rfnoc_image_builder_args schema.
    :param repo_fpga_path: A path that holds the FPGA sources (/path/to/uhd/fpga).
                           Under this path, there should be a usrp3/top/
                           directory.
    :param device: The device to build for.
    :param **args: Additional options including
                   target: The target to build (leave empty for default).
                   generate_only: Do not build the code after generation.
                   clean_all: passed to Makefile
                   GUI: passed to Makefile
                   save_project: passed to Makefile
                   ip_only: passed to Makefile
                   build_base_dir: passed to Makefile
                   build_output_dir: passed to Makefile
                   include_paths: Paths to additional blocks
    :return: Exit result of build process or 0 if generate-only is given.
    """
    assert 'source' in args
    logging.info("Selected device %s", device)
    image_core_name = args.get('image_core_name')
    if not image_core_name:
        image_core_name = os.path.splitext(os.path.basename(args['source']))[0]
        args['image_core_name'] = image_core_name
    logging.debug("Image core name: %s", image_core_name)
    if 'image_core_output' in args:
        artifact_dir = args['image_core_output']
    else:
        build_subdir = "build-" + image_core_name
        artifact_dir = os.path.join(
            os.path.normpath(os.path.split(args.get('source'))[0]),
            build_subdir)
    logging.debug("Using artifact directory: %s", artifact_dir)
    if pathlib.Path(artifact_dir).is_file():
        logging.error("Artifact directory is pointing to a file: %s", artifact_dir)
    elif pathlib.Path(artifact_dir).is_dir():
        logging.debug("Artifact directory already exists.")
    else:
        logging.debug("Creating artifact directory...")
        pathlib.Path(artifact_dir).mkdir(parents=True, exist_ok=True)
    image_core_path = os.path.join(artifact_dir, "rfnoc_image_core.sv")
    image_core_header_path = os.path.join(artifact_dir, "rfnoc_image_core.vh")
    secure_core_path = os.path.join(artifact_dir, "secure_image_core.sv")
    dts_path = os.path.join(artifact_dir, "device_tree.dts")
    makefile_srcs_path = os.path.join(artifact_dir, "Makefile.inc")
    fpga_top_dir = os.path.join(
            yaml_utils.get_top_path(os.path.abspath(repo_fpga_path)),
            target_dir(device))

    # Now load core configs
    core_config_path = yaml_utils.get_core_config_path(config_path)

    known_modules = load_module_yamls(config_path, args.get('include_paths', []))

    # resolve signature after modules have been loaded (the module YAML files
    # may contain signatures themselves)
    signatures_conf = yaml_utils.io_signatures(core_config_path,
        *list(known_modules.values()))
    device_conf = yaml_utils.IOConfig(
        yaml_utils.device_config(core_config_path, device), signatures_conf)
    if not hasattr(device_conf, 'top_dir'):
        setattr(device_conf, 'top_dir', fpga_top_dir)
    for module_type, defs in known_modules.items():
        require_schema = None if module_type == "includes" else "rfnoc_modtool_args"
        yaml_utils.resolve_io_signatures(defs, signatures_conf, require_schema)

    # Load the image core config
    builder_conf = ImageBuilderConfig(
        config,
        known_modules,
        device_conf,
        args.get("include_paths", []))
    # Generate verilog output
    output_list = [
        ("rfnoc_image_core.sv.mako", image_core_path, builder_conf),
        ("rfnoc_image_core.vh.mako", image_core_header_path, builder_conf),
        ("Makefile.inc.mako", makefile_srcs_path, builder_conf),
    ] + ([
        ("secure_image_core.sv.mako", secure_core_path, builder_conf.secure_config),
    ] if hasattr(builder_conf, 'secure_image_core') else []) + ([
        ("device_tree.dts.mako", dts_path, builder_conf),
    ] if getattr(builder_conf.device, 'generate_dts', False) else [])
    reuse = args.get('reuse', False)
    for tpl, path, conf in output_list:
        if not (reuse and pathlib.Path(path).exists()):
            write_verilog(conf, path, args, template=tpl)
        else:
            logging.info("Skipping generation of %s: File already exists and "
                         "reuse was requested.", path)
    if args.get('build_secure_core'):
        secure_core_def = builder_conf.get_secure_core_def()
        secure_core_yml = f"{args.get('build_secure_core')}.yml"
        logging.info("Writing secure core YAML to %s.", secure_core_yml)
        yaml_utils.write_yaml(secure_core_def, secure_core_yml)
        logging.warning("SECURE IMAGE BUILDING NOT YET IMPLEMENTED.")
        return 0

    ret_val = build(fpga_top_dir, device, artifact_dir, bool(netlist_files), **args)
    if args.get('generate_only'):
        return ret_val

    if ret_val == 0 and args.get('secure_core'):
        # We built the secure image core netlist. Copy the resulting netlist
        # and constraint files.
        secure_core_yml_dir = os.path.dirname(args.get('secure_core'))
        file_list = [os.path.join(artifact_dir, x) for x in new_netlist_files]
        for file in file_list:
            try:
                logging.info(f"Copying {file} to {secure_core_yml_dir}")
                shutil.copy(file, secure_core_yml_dir)
            except:
                logging.warning(f"Unable to copy netlist file {file}")

    return ret_val
