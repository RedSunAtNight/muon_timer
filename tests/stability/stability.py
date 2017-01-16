from __future__ import absolute_import, division, print_function

# This test code runs in "self triggering" mode; connect the pulse
# gpio output to the input gpio input before running.

import time
from ctypes import *

class timespec(Structure):
    _fields_ = [('tv_sec', c_long), ('tv_nsec', c_long)]

ts = timespec()

# I don't like this ... why can't I open the attribute once,
# and read forever? writing works unbuffered, why not reading
def read_attr(filename):
    with open(filename, 'r') as f:
        s = f.readline().strip()
        return s
    
print('#cnt, ts.tv_sec, ts.tv_nsec, ints, missed')
with open('/dev/muon_timer', 'rb') as muons, \
         open('/sys/class/muon_timer/muon_timer/pulse', 'wb', 0) as pulse:
    cnt = 0
    while(True):
        cnt+=1
        pulse.write('1')
        muons.readinto(ts)
        time.sleep(100./1000.)
        if cnt%1000==0 :
            ints = read_attr('/sys/class/muon_timer/muon_timer/current_interrupts')
            missed = read_attr('/sys/class/muon_timer/muon_timer/current_missed')
            print(cnt, ts.tv_sec, ts.tv_nsec, ints, missed)
