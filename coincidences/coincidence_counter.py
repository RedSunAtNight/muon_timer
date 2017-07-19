import time
from stream_reader import StreamReader
import os
from logger import Logger

logFile = "/home/helenka/Desktop/aLogFile.txt"

fifoPathTop = "/home/helenka/School/researchPopp/topfifo"
fifoPathBottom = "/home/helenka/School/researchPopp/botfifo"

urlTop = "muon4.hepnet:8090"
urlBottom = "muon3.hepnet:8090"

storageFileTop = "/home/helenka/School/researchPopp/eventsTop.dat"
storageFileBottom = "/home/helenka/School/researchPopp/eventsBot.dat"
storageFileCoinc = "/home/helenka/School/researchPopp/eventsCoinc.dat"

offset = 20                                                                         #sets the offset for the timing window, in microseconds

log = Logger(logFile)
log.setup()

def convert(row):                                                                   #combines the seconds and nanoseconds into a single number
    num = row.split()[1:]
    num = list(map(int, num))
    num[0] = (num[0] * int(1e9))                                                    #necessary to *10^9 the seconds instead of 10^-9 the nanoseconds
    return (num[0]+num[1])                                                         #for float vs int reasons (i think)

def findPositionAfter(toFind, someList):
    # binary search in a list of numbers (presumed to be in ascending order)
    # returns the index of the first item LARGER than toFind
    #print("Looking for "+str(toFind))
    maxInd = len(someList)-1
    minInd = 0
    while maxInd > minInd+1:
        spot = int((maxInd + minInd) / 2)
        if someList[spot] > toFind:
            maxInd = spot
        else:
            minInd = spot
    #print("Found       "+str(someList[maxInd])+" at index "+str(maxInd))
    return maxInd

def findCoincidences(absolute, secondary):
    absolute = [int(x/1000) for x in absolute]                                          #truncates, removes nanoseconds since we only have
    secondary = [int(x/1000) for x in secondary]                                        #microsecond resolution on our RPis
    A = set(secondary)                                                                  #converts the first data set to type Set instead of List
    B = set()
    for x in absolute:                                                                  #populates the second set with a range of numbers from the second data set
        B.update(range(x-offset, x+offset+1,1))                                         #that is dependent on the +/- of the offset
    return A.intersection(B)

def closeUp(fileLikes, streamReader):
    # close stream reader first, so it's all done 
    # before you take away the fifo it's trying to write to
    try:
        streamReader.stop()
    except BaseException as ex:
        log.error("Failed to close stream reader: "+str(ex)) 

    for handle in fileLikes:
        try:
            handle.close()
        except BaseException as ex:
            log.error("Failed to close file "+handle.name+": "+str(ex))     

coincidence = []
 # So, the steps are:

 # create two fifos -- this code will be READING from them!
os.mkfifo(fifoPathTop)
os.mkfifo(fifoPathBottom)

 # Fire up StreamReader, pointing at the muon timers, writing into the fifos
urlFileDict = {urlTop:fifoPathTop, urlBottom:fifoPathBottom}
streamReader = StreamReader(urlFileDict)
streamReader.getMuonEvents()

fifoTop = open(fifoPathTop, 'r')
fifoBottom = open(fifoPathBottom, 'r')

fileTop = open(storageFileTop, 'w')
fileBottom = open(storageFileBottom, 'w')
fileCoinc = open(storageFileCoinc, 'w')

dataTop = []
dataBottom = []

# The very top line is a header; do not attempt to treat is as data
topLine = fifoTop.readline()
fileTop.write(topLine)
botLine = fifoBottom.readline()
fileBottom.write(botLine)
fileCoinc.write('coinc (ms)\n')

numEvts = 300
try:
    while True:
        # Read X lines from the fifos into the buffers (10? 100? idk)

        # create two "buffer" lists of events
        evtBufTop = []
        evtBufBot = []
        for i in range(0, numEvts):
           # will block until this many is read
           topLine = fifoTop.readline()
           botLine = fifoBottom.readline()
           evtBufTop.append(topLine)
           fileTop.write(topLine)
           evtBufBot.append(botLine)
           fileBottom.write(botLine)

        # use Chris's logic to count coincidences in the buffers
        tmpTop = [convert(x) for x in evtBufTop]                                                
        tmpBottom = [convert(x) for x in evtBufBot]
        dataTop.extend(tmpTop)
        dataBottom.extend(tmpBottom)
        dataTop.sort()
        dataBottom.sort()

        # THE FOLLOWING WAS ORIGINALLY DEVELOPED BY CHRIS TANDOI
        if dataBottom[0] > dataTop[0]:                                                      #finds out which dataset started collecting data first
            #absolute = dataTop
            #secondary = dataBottom
            newCoincs = findCoincidences(dataTop, dataBottom)
            for x in newCoincs:
                fileCoinc.write("{}\n".format(x))
            coincidence.extend(newCoincs)
            if len(newCoincs) > 0:
                # deciding which data to move to the beginning:
                lastCoincidence = coincidence[-1]
                #      for the one considered "secondary" all events after the last coincidence go to the beginning
                newFirst = findPositionAfter((lastCoincidence*1000)+999, dataBottom)
                dataBottom = dataBottom[newFirst+1:]
                #      for the one considered "absolute", find "last coincidence + offset". This is the last place a concidence might be.
                #          now, find the first event after that number. This is the coincidence event.
                beforeFirst = findPositionAfter(((lastCoincidence+offset)*1000)+999, dataTop)
                #          all events after that go to the beginning.
                dataTop = dataTop[beforeFirst+2:]
        else:
            #absolute = dataBottom
            #secondary = dataTop
            newCoincs = findCoincidences(dataBottom, dataTop)
            for x in newCoincs:
                fileCoinc.write("{}\n".format(x))
            coincidence.extend(newCoincs)
            if len(newCoincs) > 0:
                # deciding which data to move to the beginning:
                lastCoincidence = coincidence[-1]
                #      for the one considered "secondary" all events after the last coincidence go to the beginning
                newFirst = findPositionAfter((lastCoincidence*1000)+999, dataTop)
                dataTop = dataTop[newFirst+1:]
                #      for the one considered "absolute", find "last coincidence + offset". This is the last place a concidence might be.
                #          now, find the first event after that number. This is the coincidence event.
                beforeFirst = findPositionAfter(((lastCoincidence+offset)*1000)+999, dataBottom)
                #          all events after that go to the beginning.
                dataBottom = dataBottom[beforeFirst+2:]
except KeyboardInterrupt:
    print("\nStopping...")
except BaseException as ex:
    log.error(str(ex))
    print(ex)
finally:
    closeUp([fifoTop, fifoBottom, fileTop, fileBottom, fileCoinc], streamReader)
    os.remove(fifoPathTop)
    os.remove(fifoPathBottom)
    log.close()
    numRaw = len(coincidence)
    coincSet = set(coincidence)
    numTrue = len(coincSet)
    repeats = numRaw - numTrue
    print("{} repeated values out of {} in results".format(repeats, numRaw))
    print("Done")
