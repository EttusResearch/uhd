#
# Copyright 2010 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import re
import math
from Cheetah.Template import Template

def parse_tmpl(_tmpl_text, **kwargs):
    return str(Template(_tmpl_text, kwargs))

def to_num(arg): return eval(arg)

class reg:
    def __init__(self, reg_des):
        try: self.parse(reg_des)
        except Exception, e:
            raise Exception, 'Error parsing register description: "%s"\nWhat: %s'%(reg_des, e)

    def parse(self, reg_des):
        x = re.match('^(\w*)\s*(\w*)\[(.*)\]\s*(\w*)\s*(.*)$', reg_des)
        name, addr, bit_range, default, enums = x.groups()

        #store variables
        self._name = name
        self._addr = to_num(addr)
        if ':' in bit_range: self._addr_spec = sorted(map(int, bit_range.split(':')))
        else: self._addr_spec = int(bit_range), int(bit_range)
        self._default = to_num(default)

        #extract enum
        self._enums = list()
        if enums:
            enum_val = 0
            for enum_str in map(str.strip, enums.split(',')):
                if '=' in enum_str:
                    enum_name, enum_val = enum_str.split('=')
                    enum_val = to_num(enum_val)
                else: enum_name = enum_str
                self._enums.append((enum_name, enum_val))
                enum_val += 1

    def get_addr(self): return self._addr
    def get_enums(self): return self._enums
    def get_name(self): return self._name
    def get_default(self):
        for key, val in self.get_enums():
            if val == self._default: return str.upper('%s_%s'%(self.get_name(), key))
        return self._default
    def get_stdint_type(self):\
        return 'uint%d_t'%max(2**math.ceil(math.log(self.get_bit_width(), 2)), 8)
    def get_shift(self): return self._addr_spec[0]
    def get_mask(self): return hex(int('1'*self.get_bit_width(), 2))
    def get_bit_width(self): return self._addr_spec[1] - self._addr_spec[0] + 1
