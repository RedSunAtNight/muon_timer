#! /usr/bin/env python
from threading import Thread
import urllib.request
import urllib.error
import queue

class RequestThread(Thread):

    bufSize = 1024 # a 1-kB buffer should be safe

    def setTargets(self, url, outfile):
        self.url = url
        self.outfile = outfile
        self.stopped = False

    def stopRead(self):
        self.stopped = True

    def run(self):
        # open the data file
        with open(self.outfile, 'wb') as writeFile:
            # GET request to host:port
            addr = "http://{}".format(self.url)
            try:
                with urllib.request.urlopen(addr) as resp:
                    if resp.getcode() != 200:
                        body = resp.read()
                        msg = "Got response code {} from {}. Response body is: {}".format(resp.getcode(), self.url, body)
                        raise RuntimeError(msg)
                    while not self.stopped:
                        buf = resp.read(self.bufSize)
                        writeFile.write(buf)
            except urllib.error.URLError as urlErr:
                msg = "Url error while attempting GET http://{}".format(self.url)
                raise RuntimeError(msg) from urlErr
            except ConnectionRefusedError as connErr:
                msg = "Connection refused while attempting GET http://{}".format(self.url)
                raise RuntimeError(msg) from connErr


class StreamReader:

    def __init__(self, urlFileDict):
        self.urlFileDict = urlFileDict
        self.threadsList = []

    def getMuonEvents(self):
        for url in self.urlFileDict:
            thread = RequestThread()
            thread.setTargets(url, self.urlFileDict[url])
            self.threadsList.append(thread)
            thread.start()

    def stop(self):
        for thr in self.threadsList:
            thr.stopRead()
        for thr in self.threadsList:
            thr.join(5.0)

        openHosts = []
        for thr in self.threadsList:
            if thr.isAlive():
                openHosts.append(thr.host)
        if len(openHosts) != 0:
            msg = "Failed to close requests to {}".format(str(openHosts))
            raise RuntimeError(msg)



# if __name__ == "__main__":
#     print("Testing code")
#     urlFileDict = {"muon4.hepnet:8090":"/home/helenka/Desktop/muon4.txt",
#                     "muon3.hepnet:8090":"/home/helenka/Desktop/muon3.txt"}
#     streamReader = StreamReader(urlFileDict)
#     reader.getMuonEvents()
#     time.sleep(30.0)
#     reader.stop()