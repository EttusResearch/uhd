#!/usr/bin/env python
from labview_automation.client import LabVIEWClient

class vst_siggen:
    def __init__(self,host,vi_base_path,rio_device):
        self._host = host
        self._path = vi_base_path
        self._rio = rio_device
        self._caller = lv_caller(host,2552,vi_base_path)

    def __del__(self):
        self.disconnect()

    def set_freq(self, freq):
        self._caller.vst_set_freq(self._rio,freq)

    def disconnect(self):
        try:
            self._caller.vst_disconnect(self._rio)
        except:
            pass


class executive_switch:
    def __init__(self,host,vi_base_path,device_name):
        self._host = host
        self._path = vi_base_path
        self._device = device_name
        self._caller = lv_caller(host,2552,vi_base_path)

    def __del__(self):
        self.disconnect_all()

    def connect_ports(self, port0, port1):
        self._caller.switch_connect_ports(self._device,port0,port1)

    def disconnect_all(self):
        try:
            self._caller.switch_disconnect_all(self._device)
        except:
            pass



class lv_caller:
    def __init__(self, host, port, vi_base_path):
        self._host = host
        self._port = port
        self._client = LabVIEWClient(host, port)
        self._path = vi_base_path

    def vst_disconnect(self, rio_device):
        with self._client as c:
            control_values = {
                    "rio_device": rio_device,
            }
            result = c.run_vi_synchronous("".join([self._path,"vst_disconnect.vi"]),control_values)
        return result
    
    def vst_set_freq(self, rio_device, freq):
        with self._client as c:
            control_values = {
                    "rio_device": rio_device,
                    "cw_freq": freq,
                    "power": 0,
            }
            result = c.run_vi_synchronous("".join([self._path,"vst_set_freq.vi"]),control_values)
        return result
    
    def switch_connect_ports(self, switch_device, port0, port1):
        with self._client as c:
            control_values = {
                    "virtual_switch": switch_device,
                    "chan0": port0,
                    "chan1": port1,
            }
            result = c.run_vi_synchronous("".join([self._path,"switch_connect_ports.vi"]),control_values)
        return result
    
    
    def switch_disconnect_all(self, switch_device):
        with self._client as c:
            control_values = {
                    "virtual_switch": switch_device,
            }
            result = c.run_vi_synchronous("".join([self._path,"switch_disconnect.vi"]),control_values)
        return result
