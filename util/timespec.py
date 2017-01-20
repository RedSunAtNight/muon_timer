from ctypes import Structure, c_long
from functools import total_ordering
from math import floor

@total_ordering
class timespec(Structure):
    _fields_ = [('tv_sec', c_long), ('tv_nsec', c_long)]

    # implement two argument constructor and one (tuple,list)
    # argument, and one argument float as well?  read about it, but
    # book is at work...
    def __init__(self, s, ns):
        self.tv_sec = s
        self.tv_nsec = ns

    def __float__(self):
        return float(self.tv_sec) + float(self.tv_nsec)/1e9

    # test types?
    # version that subtracts a float?
    # return a timespec?
    def __sub__(self, rhs):
        sdiff = self.tv_sec - rhs.tv_sec
        nsdiff = self.tv_nsec - rhs.tv_nsec
        return float(sdiff) + float(nsdiff)/1e9

    # this adds a float ... need a version that adds two timespecs
    def __add__(self, rhs):
        s = self.tv_sec+floor(rhs)
        ns = self.tv_nsec + int( 1e9*(rhs-floor(rhs)) )
        if ns > 1000000000:
            s += 1
            ns -= 1000000000
        return timespec(s, ns)

    # these are easy...
    def __eq__(self, rhs):
        return self.tv_sec==rhs.tv_sec and self.tv_nsec==rhs.tv_nsec

    def __lt__(self, rhs):
        if self.tv_sec < rhs.tv_sec:
            return True
        if self.tv_sec == rhs.tv_sec and self.tv_nsec < rhs.tv_nsec:
            return True
        return False

    def __str__(self):
        return '(' + str(self.tv_sec) + ', ' + str(self.tv_nsec) + ')'

    # what should this really return?
    def __repr__(self):
        return str(self)

# should have some unit tests :-)
