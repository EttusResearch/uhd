#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
MPM Component management
"""
import os
import re
import shutil
import subprocess
from usrp_mpm.rpc_utils import no_rpc


class ZynqComponents:
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
    def _log_and_raise(self, logstr):
        self.log.error(logstr)
        raise RuntimeError(logstr)

    @classmethod
    def _parse_dts_mpm_version_tag(cls, text):
        """ parse a version line from the dts file. E.g.
        "// mpm_version component1 3.4.5" will return
        {"component1": (3, 4, 5)} """
        dts_version_re = re.compile(r'^// mpm_version\s+(?P<comp>\S+)\s+(?P<ver>\S+)$')
        match = dts_version_re.match(text)
        if match is None:
            return (None, None)

        component = match[1]
        version_list = [int(x, base=0) for x in match[2].split('.')]
        return (component, tuple(version_list))

    @classmethod
    def _parse_dts_version_info_from_file(cls, filepath):
        """
        parse all version informations from dts file and store in dict
        a c-style comment in the dts file like this
        // mpm_version component1 3.4.5
        // mpm_version other_component 3.5.0
        will return a dict:
        {"component1": (3, 4, 5), "other_component": (3, 5, 0)"}
        """
        suffix_current = "_current_version"
        suffix_oldest = "_oldest_compatible_version"

        version_info = {}
        with open(filepath) as f:
            text = f.read()
            for line in text.splitlines():
                component, version = cls._parse_dts_mpm_version_tag(line)
                if not component:
                    continue
                if component.endswith(suffix_oldest):
                    component = component[:-len(suffix_oldest)]
                    version_type = 'oldest'
                elif component.endswith(suffix_current):
                    component = component[:-len(suffix_current)]
                    version_type = 'current'
                else:
                    version_type = 'current'
                if component not in version_info:
                    version_info[component] = {}
                version_info[component][version_type] = version
        return version_info

    def _merge_updateable_components(self):
        """
        Combines updateable_components from MB and all DBs and returns the
        merged result.
        """
        def merge_dicts(origd, newd):
            """
            https://stackoverflow.com/questions/71396284/python-how-to-recursively-merge-2-dictionaries
            """
            for key, val in newd.items():
                if key not in origd:
                    origd[key] = val
                    continue
                if isinstance(val, dict):
                    if not isinstance(origd[key], dict):
                        origd[key] = {}
                    merge_dicts(origd[key], val)
                else:
                    origd[key] = val
            return origd
        upc = self.updateable_components
        for dboard in self.dboards:
            upc = merge_dicts(upc, dboard.updateable_components)
        return upc

    def _verify_compatibility(self, filebasename, update_dict):
        """
        check whether the given MPM compatibility matches the
        version information stored in the FPGA DTS file
        """
        def _get_version_string(versions_enum):
            version_strings = []
            if 'current' in versions_enum:
                version_strings.append("current: {}".format(
                    ".".join([str(x) for x in versions_enum['current']])))
            if 'oldest' in versions_enum:
                version_strings.append("oldest compatible: {}".format(
                    ".".join([str(x) for x in versions_enum['oldest']])))
            return ', '.join(version_strings)


        if update_dict.get('check_dts_for_compatibility'):
            self.log.trace("Compatibility check MPM <-> FPGA via DTS enabled")
            dtsfilepath = filebasename + '.dts'
            if not os.path.exists(dtsfilepath):
                self.log.warning(f"DTS file not found: {dtsfilepath}, cannot "
                                 f"check version of bitfile without DTS.")
                return
            self.log.trace(f"Parsing DTS file {dtsfilepath} for version information")
            fpga_versions = self._parse_dts_version_info_from_file(dtsfilepath)
            if not fpga_versions:
                self._log_and_raise("no component version information in DTS file")
            if 'compatibility' not in update_dict:
                self._log_and_raise("MPM FPGA version infos not found")
            mpm_versions = update_dict['compatibility']
            self.log.trace(f"DTS version infos: {fpga_versions}")
            self.log.trace(f"MPM version infos: {mpm_versions}")

            try:
                for component in mpm_versions.keys():
                    # check for components that aren't available in the DTS file
                    if component in fpga_versions.keys():
                        self.log.trace(f"check compatibility for: FPGA-{component}")
                        mpm_version = mpm_versions[component]
                        fpga_version = fpga_versions[component]
                        self.log.trace("mpm_version: current: {}, compatible: {}".format(
                            mpm_version['current'], mpm_version['oldest']))
                        self.log.trace("fpga_version: current: {}, compatible: {}".format(
                            fpga_version['current'], fpga_version['oldest']))
                        if mpm_version['oldest'][0] > fpga_version['current'][0]:
                            error = "Component {} is too old ({}, MPM version: {})".format(
                                component,
                                _get_version_string(fpga_version),
                                _get_version_string(mpm_version))
                            self._log_and_raise(error)
                        elif mpm_version['current'][0] < fpga_version['oldest'][0]:
                            error = "Component {} is too new ({}, MPM version: {})".format(
                                component,
                                _get_version_string(fpga_version),
                                _get_version_string(mpm_version))
                            self._log_and_raise(error)
                        self.log.trace(f"Component {component} is good!")
                    else:
                        self.log.warning(f"component {component} defined in "\
                            f"MPM but not found in FPGA info, skipping.")
            except RuntimeError as ex:
                self._log_and_raise("MPM compatibility infos suggest that the "\
                    "new bitfile is not compatible, skipping installation. {}"\
                    .format(ex))
        else:
            self.log.trace("Compatibility check MPM <-> FPGA is disabled")
            return

    @no_rpc
    def update_fpga(self, filepath, metadata):
        """
        Update the FPGA image in the filesystem and reload the overlay
        :param filepath: path to new FPGA image
        :param metadata: Dictionary of strings containing metadata
        """
        self.log.trace(f"Updating FPGA with image at {filepath}"\
            " (metadata: `{str(metadata)}')")
        file_name, file_extension = os.path.splitext(filepath)
        # Cut off the period from the file extension
        file_extension = file_extension[1:].lower()
        if file_extension not in ['bit', 'bin']:
            self._log_and_raise(f"Invalid FPGA bitfile: {filepath}")
        updc = self._merge_updateable_components()
        assert 'path' in updc['fpga']
        binfile_path = updc['fpga']['path'].format(self.device_info.get('product'))

        self._verify_compatibility(file_name, updc['fpga'])

        if file_extension == "bit":
            self.log.trace(
                f"Converting bit to bin file and writing to {binfile_path}")
            from usrp_mpm.fpga_bit_to_bin import fpga_bit_to_bin
            fpga_bit_to_bin(filepath, binfile_path, flip=True)
        elif file_extension == "bin":
            self.log.trace("Copying bin file to %s", binfile_path)
            shutil.copy(filepath, binfile_path)

        # RPC server will reload the periph manager after this.
        return True

    @no_rpc
    def update_dts(self, filepath, metadata):
        """
        Update the DTS image in the filesystem
        :param filepath: path to new DTS image
        :param metadata: Dictionary of strings containing metadata
        """
        dtsfile_path = self._merge_updateable_components()['dts']['path'].format(
            self.device_info.get('product'))
        self.log.trace("Updating DTS with image at %s to %s (metadata: %s)",
                       filepath, dtsfile_path, str(metadata))
        shutil.copy(filepath, dtsfile_path)
        dtbofile_path = self._merge_updateable_components()['dts']['output'].format(
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
