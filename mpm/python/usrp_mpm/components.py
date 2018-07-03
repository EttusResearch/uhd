#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
MPM Component management
"""
import os
import shutil
import subprocess
from usrp_mpm.rpc_server import no_rpc


class ZynqComponents(object):
    """
    Mixin class that update Zynq FPGA and devicetree components.

    Note: this assumes that the class that you are mixing ZynqComponents into
    initializes `updateable_components`, `device_info`, and `log`.
    """
    # Declare required members here
    updateable_components = {}
    device_info = {}
    # Note: the logger is created by derived class (the class we are mixing
    #       into), so all logs in the ZynqComponents class will be under the
    #       derived class's category.
    log = None

    ###########################################################################
    # Component updating
    ###########################################################################
    @no_rpc
    def update_fpga(self, filepath, metadata):
        """
        Update the FPGA image in the filesystem and reload the overlay
        :param filepath: path to new FPGA image
        :param metadata: Dictionary of strings containing metadata
        """
        self.log.trace("Updating FPGA with image at {} (metadata: `{}')"
                       .format(filepath, str(metadata)))
        _, file_extension = os.path.splitext(filepath)
        # Cut off the period from the file extension
        file_extension = file_extension[1:].lower()
        binfile_path = self.updateable_components['fpga']['path'].format(
            self.device_info.get('product'))
        if file_extension == "bit":
            self.log.trace("Converting bit to bin file and writing to {}"
                           .format(binfile_path))
            from usrp_mpm.fpga_bit_to_bin import fpga_bit_to_bin
            fpga_bit_to_bin(filepath, binfile_path, flip=True)
        elif file_extension == "bin":
            self.log.trace("Copying bin file to %s", binfile_path)
            shutil.copy(filepath, binfile_path)
        else:
            self.log.error("Invalid FPGA bitfile: %s", filepath)
            raise RuntimeError("Invalid N3xx FPGA bitfile")
        # RPC server will reload the periph manager after this.
        return True

    @no_rpc
    def update_dts(self, filepath, metadata):
        """
        Update the DTS image in the filesystem
        :param filepath: path to new DTS image
        :param metadata: Dictionary of strings containing metadata
        """
        dtsfile_path = self.updateable_components['dts']['path'].format(
            self.device_info.get('product'))
        self.log.trace("Updating DTS with image at %s to %s (metadata: %s)",
                       filepath, dtsfile_path, str(metadata))
        shutil.copy(filepath, dtsfile_path)
        dtbofile_path = self.updateable_components['dts']['output'].format(
            self.device_info.get('product'))
        self.log.trace("Compiling to %s...", dtbofile_path)
        dtc_command = [
            'dtc',
            '--symbols',
            '-O', 'dtb',
            '-q',  # Suppress warnings
            '-o',
            dtbofile_path,
            dtsfile_path,
        ]
        self.log.trace("Executing command: `$ %s'", " ".join(dtc_command))
        try:
            out = subprocess.check_output(dtc_command)
            if out.strip() != "":
                # Keep this as debug because dtc is an external tool and
                # something could go wrong with it that's outside of our control
                self.log.debug("`dtc' command output: \n%s", out)
        except OSError:
            self.log.error("Could not execute `dtc' command. Binary probably "
                           "not installed. Please compile DTS by hand.")
            # No fatal error here, in order not to break the current workflow
        except subprocess.CalledProcessError as ex:
            self.log.error("Error executing `dtc': %s", str(ex))
            return False
        return True
