# We use python2.7 instead of the infinitely preferable 3.5+ because
# our dev board installation only includes 2.7 :-()

# this uses the pulse output to drive the interrupt input; make sure
# to connect the pulse gpio output to the input gpio input pin before
# running.

from __future__ import absolute_import, division, print_function

import time, ctypes

class timespec(ctypes.Structure):
    _fields_ = [('tv_sec', ctypes.c_long), ('tv_nsec', ctypes.c_long)]


ts = timespec()
with open('/dev/muon_timer', 'rb', 0) as f:
    with open('/sys/class/muon_timer/muon_timer/pulse', 'wb', 0) as p:
        for i in range(10):
            p.write('1')
            f.readinto(ts)
            print(ts.tv_sec, ts.tv_nsec)

