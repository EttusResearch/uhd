#
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
X4XX GPS Manager

Handles GPS-related tasks
"""

import re
from usrp_mpm.gpsd_iface import GPSDIfaceExtension

class X4xxGPSMgr:
    """
    Manager class for GPS-related actions for the X4XX.

    This also "disables" the sensors when the GPS is not enabled.
    """
    def __init__(self, clk_aux_board, log):
        assert clk_aux_board and clk_aux_board.is_gps_supported()
        self._clocking_auxbrd = clk_aux_board
        self.log = log.getChild('GPS')
        self.log.trace("Initializing GPSd interface")
        self._gpsd = GPSDIfaceExtension()
        # To disable sensors, we simply return an empty value if GPS is disabled.
        # For TPV, SKY, and GPGGA, we can do this in the same fashion (they are
        # very similar). gps_time is different (it returns an int) so for sake
        # of simplicity it's defined separately below.
        for sensor_name in ('gps_tpv', 'gps_sky', 'gps_gpgga'):
            sensor_api = f'get_{sensor_name}_sensor'
            setattr(
                self, sensor_api,
                lambda sensor_name=sensor_name: {
                    'name': sensor_name, 'type': 'STRING',
                    'unit': '', 'value': 'n/a'} \
                if not self.is_gps_enabled() \
                else getattr(self._gpsd, f'get_{sensor_name}_sensor')()
            )

    def extend(self, context):
        """
        Extend 'context' with the sensor methods of this class (get_gps_*_sensor).
        If 'context' already has such a method, it is skipped.

        Returns a dictionary compatible to mboard_sensor_callback_map.
        """
        new_methods = {
            re.search(r"get_(.*)_sensor", method_name).group(1): method_name
            for method_name in dir(self)
            if not method_name.startswith('_') \
            and callable(getattr(self, method_name)) \
            and method_name.endswith("sensor")}
        for method_name in new_methods.values():
            if hasattr(context, method_name):
                continue
            new_method = getattr(self, method_name)
            self.log.trace("%s: Adding %s method", context, method_name)
            setattr(context, method_name, new_method)
        return new_methods

    def is_gps_enabled(self):
        """
        Return True if the GPS is enabled/active.
        """
        return self._clocking_auxbrd.is_gps_enabled()

    def get_gps_enabled_sensor(self):
        """
        Get enabled status of GPS as a sensor dict
        """
        gps_enabled = self.is_gps_enabled()
        return {
            'name': 'gps_enabled',
            'type': 'BOOLEAN',
            'unit': 'enabled' if gps_enabled else 'disabled',
            'value': str(gps_enabled).lower(),
        }

    def get_gps_locked_sensor(self):
        """
        Get lock status of GPS as a sensor dict
        """
        gps_locked = self.is_gps_enabled() and \
                bool(self._clocking_auxbrd.get_gps_lock())
        return {
            'name': 'gps_lock',
            'type': 'BOOLEAN',
            'unit': 'locked' if gps_locked else 'unlocked',
            'value': str(gps_locked).lower(),
        }

    def get_gps_alarm_sensor(self):
        """
        Get alarm status of GPS as a sensor dict
        """
        gps_alarm = self.is_gps_enabled() and \
                bool(self._clocking_auxbrd.get_gps_alarm())
        return {
            'name': 'gps_alarm',
            'type': 'BOOLEAN',
            'unit': 'active' if gps_alarm else 'not active',
            'value': str(gps_alarm).lower(),
        }

    def get_gps_warmup_sensor(self):
        """
        Get warmup status of GPS as a sensor dict
        """
        gps_warmup = self.is_gps_enabled() and \
                bool(self._clocking_auxbrd.get_gps_warmup())
        return {
            'name': 'gps_warmup',
            'type': 'BOOLEAN',
            'unit': 'warming up' if gps_warmup else 'warmup done',
            'value': str(gps_warmup).lower(),
        }

    def get_gps_survey_sensor(self):
        """
        Get survey status of GPS as a sensor dict
        """
        gps_survey = self.is_gps_enabled() and \
                bool(self._clocking_auxbrd.get_gps_survey())
        return {
            'name': 'gps_survey',
            'type': 'BOOLEAN',
            'unit': 'survey active' if gps_survey else 'survey not active',
            'value': str(gps_survey).lower(),
        }

    def get_gps_phase_lock_sensor(self):
        """
        Get phase_lock status of GPS as a sensor dict
        """
        gps_phase_lock = self.is_gps_enabled() and \
                bool(self._clocking_auxbrd.get_gps_phase_lock())
        return {
            'name': 'gps_phase_lock',
            'type': 'BOOLEAN',
            'unit': 'phase locked' if gps_phase_lock else 'no phase lock',
            'value': str(gps_phase_lock).lower(),
        }

    def get_gps_time_sensor(self):
        """

        """
        if not self.is_gps_enabled():
            return {
                'name': 'gps_time',
                'type': 'INTEGER',
                'unit': 'seconds',
                'value': str(-1),
            }
        return self._gpsd.get_gps_time_sensor()
