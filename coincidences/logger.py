#! /usr/bin/env python
from datetime import datetime
import sys

class Logger:
    def __init__(self, filename):
        self.filename = filename
    
    def setup(self):
        try:
            self.logfile = open(self.filename, 'a', 1)
            self.useStdout = False
        except BaseException as ex:
            self.logfile = sys.stdout
            self.useStdout = True
            raise ex

    def debug(self, line):
        stamp = datetime.now().isoformat()
        self.logfile.write("{} DEBUG: {}\n".format(stamp, line))

    def info(self, line):
        stamp = datetime.now().isoformat()
        self.logfile.write("{} INFO: {}\n".format(stamp, line))

    def warn(self, line):
        stamp = datetime.now().isoformat()
        self.logfile.write("{} WARN: {}\n".format(stamp, line))

    def error(self, line):
        stamp = datetime.now().isoformat()
        self.logfile.write("{} ERROR: {}\n".format(stamp, line))

    def close(self):
        if not self.useStdout:
            self.logfile.close()