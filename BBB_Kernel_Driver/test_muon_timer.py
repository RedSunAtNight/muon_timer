import time


f = open('/dev/muon_timer', 'rb')

time.sleep(10)

d = f.read(8)

print(d)

f.close()


