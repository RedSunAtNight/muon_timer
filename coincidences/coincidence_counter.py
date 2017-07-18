#! /usr/bin/env python
from threading import Thread

# easiest version:
# start two asynchronous (or just in different threads) http requests, one to each receiver.
# stream the results into files.
# use Chris's softwarecoincidence code.

class RequestThread(Thread):

    def setTargets(host, port, outfile):
        self.host = host
        self.port = port
        self.outfile = outfile
        self.stopped = False

    def stop():
        self.stopped = True

    def run():
        # GET request to host:port
        # loop reading from the response and writing to the file
        # if self.stopped == True, close the HTTP connection and close the file



# create two RequestThreads
# start them
# wait for user input
# user says when to stop them
# after stopping them, use the code from softwarecoincidence.py