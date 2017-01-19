from __future__ import absolute_import, division, print_function

# This test code runs in "self triggering" mode; connect the pulse
# gpio output to the input gpio input before running.

# If you plan on piping the output somewhere, you must run the
# interpreter with the -u flag (unbuffered stdio), or else it will sit
# forever before doing anything

import time
with open('/dev/muon_timer', 'rb') as muons, \
         open('/sys/class/muon_timer/muon_timer/pulse', 'wb', 0) as pulse:
    cnt = 0
    while(True):
        cnt+=1
        pulse.write('1')
        time.sleep(100./1000.)
        print(cnt)
