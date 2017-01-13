import time, select


f = open('/dev/muon_timer', 'rb')

#time.sleep(10)

inputs = [f]
outputs = []

readable, writeable, exceptional = select.select(inputs, outputs, inputs)

print(readable)

f.close()


