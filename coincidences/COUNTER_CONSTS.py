# COUNTER_CONSTS.py

# update these values to be stricter or more lenient when counting coincidences, or to change the coarseness of the live-plotted histogram.

# time, in MICROSECONDS, by which two pulses may be separated and still be considered "coincident"
matchAllowance = 5000

# width of a bin in the live-plotted histogram, in SECONDS
histBinSec = 10

# conversion to microsec, do not change
histBinTime = histBinSec * 1000000