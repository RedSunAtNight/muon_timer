from __future__ import absolute_import, division, print_function

# If you plan on piping the output somewhere, you must run the
# interpreter with the -u flag (unbuffered stdio), or else it will sit
# forever before doing anything

import time
from ctypes import *

class timespec(Structure):
    _fields_ = [('tv_sec', c_long), ('tv_nsec', c_long)]

ts = timespec()

print('#cnt, ts.tv_sec, ts.tv_nsec')
with open('/dev/muon_timer', 'rb') as muons:
    cnt = 0
    while(True):
        cnt+=1
        muons.readinto(ts)
        time.sleep(100./1000.)
        print(cnt, ts.tv_sec, ts.tv_nsec)
