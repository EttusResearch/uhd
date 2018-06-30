#!/usr/bin/env python3
#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
import mpmlog
import bfrfs

LOG = mpmlog.get_main_logger().getChild('log')
B0 = bfrfs.BufferFS(b'', 256, 16, log=LOG)

B0.set_blob('foo', b'123123123')
B0.set_blob('baz', b'abcdabcdasdfasdf')

print(B0.buffer)
print(len(B0.buffer))

LOG.warn('next foo')


new_buf = open('eeprom.dat', 'rb').read()
B1 = bfrfs.BufferFS(new_buf, 256, 16, log=LOG)
print(B1.get_blob('foo'))
print(B1.get_blob('baz'))
LOG.warn('next foo')
B1.set_blob('baz', b'asdfalskdfjalksdfasdfkasdfkjh')
B1.set_blob('foo', b'asdfalskdfjalksdfasdfkasdfkjh2')
open('eeprom.dat', 'wb').write(B1.buffer)

print(B1.get_blob('foo'))
