import time
from stream_reader import StreamReader, Requestor
import os
from logger import Logger
import sys
from matplotlib import pyplot as plt


if len(sys.argv) < 5:
    print("Usage: python coincidence_counter.py url1 url2 data_dir logfile")
    sys.exit()

urlTop = sys.argv[1]
urlBottom = sys.argv[2]
dataHome = sys.argv[3]
logFile = sys.argv[4]



storageFileTop = dataHome+"/eventsTop.dat"
storageFileBottom = dataHome+"/eventsBot.dat"
storageFileCoinc = dataHome+"/eventsCoinc.dat"

offset = 5000 #30                                                                         #sets the offset for the timing window, in microseconds
histBinTime = 10*1000000

log = Logger(logFile)
log.setup()
log.info("starting coincidence counter...")
log.info("urlTop: "+urlTop)
log.info("urlBottom: "+urlBottom)
log.info("storageFileTop: "+storageFileTop)
log.info("storageFileBottom: "+storageFileBottom)
log.info("storageFileCoinc: "+storageFileCoinc)

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

def closeUp(fileLikes):
    for handle in fileLikes:
        try:
            handle.close()
        except BaseException as ex:
            log.error("Failed to close file "+handle.name+": "+str(ex))     

coincidence = []

reqTop = Requestor(urlTop)
reqBottom = Requestor(urlBottom)

fileTop = open(storageFileTop, 'wb')
fileBottom = open(storageFileBottom, 'wb')
fileCoinc = open(storageFileCoinc, 'w')

dataTop = []
dataBottom = []

# write a header for the coincidence file
fileCoinc.write('#coinc (us)\n')

byteBufSize = 1024
topSpillover = bytearray()
bottomSpillover = bytearray()

topResp = reqTop.open()
bottomResp = reqBottom.open()

figure = plt.figure(1)
plt.ion()
plt.show()

minCoincTime = None

print("... ready")
log.info("... ready")
firstGo = True
try:
    while True:
        # simpler approach: dump everything straight from the data stream into a file
        # THEN break it into events for counting
        bytesTop = topResp.read(byteBufSize)
        bytesBottom = bottomResp.read(byteBufSize)
        fileTop.write(bytesTop)
        fileBottom.write(bytesBottom)


        # Read X lines from the fifos into the buffers (10? 100? idk)
        bytesTop = topSpillover + bytesTop
        bytesBottom = bottomSpillover + bytesBottom
        strTop = str(bytesTop, 'utf-8')
        strBot = str(bytesBottom, 'utf-8')

        isTopRagged = False
        isBottomRagged = False

        if strTop[-1] is not '\n':
            # we didn't end on a newline, which means we have a chopped-off piece of line
            isTopRagged = True
        if strBot[-1] is not '\n':
            # we didn't end on a newline, which means we have a chopped-off piece of line
            isBottomRagged = True

        evtBufTop = strTop.split('\n')
        evtBufTop = list(filter(None, evtBufTop))   # remove empty strings from the list
        evtBufBot = strBot.split('\n')
        evtBufBot = list(filter(None, evtBufBot))

        # if the end is not a full line, throw that part back into the byte buffer
        if isTopRagged:
            topSpillover = bytearray(evtBufTop.pop(), 'utf-8')
        if isBottomRagged:
            bottomSpillover = bytearray(evtBufBot.pop(), 'utf-8')

        # The first line is a header, we don't want that in out calculations
        #if evtBufTop[0]
        if firstGo:
            print("removing header line...")
            evtBufTop.pop(0)
            evtBufBot.pop(0)
            firstGo = False
            print("removed the header line")

        # use Chris's logic to count coincidences in the buffers
        tmpTop = []
        if len(evtBufTop) > 0:
            tmpTop = [convert(x) for x in evtBufTop]                                                
        tmpBottom = []
        if len(evtBufBot) > 0:
            tmpBottom = [convert(x) for x in evtBufBot]
        dataTop.extend(tmpTop)
        dataBottom.extend(tmpBottom)
        dataTop.sort()
        dataBottom.sort()

        # THE FOLLOWING WAS ORIGINALLY DEVELOPED BY CHRIS TANDOI
        if dataBottom[0] > dataTop[0]:                                                      #finds out which dataset started collecting data first
            newCoincs = findCoincidences(dataTop, dataBottom)
            if minCoincTime is None and len(newCoincs) > 0:
                minCoincTime = min(newCoincs)
            for x in newCoincs:
                fileCoinc.write("{}\n".format(x))
                x -= minCoincTime
                coincidence.append(x)
            coincidence.sort()
            if len(newCoincs) > 0:
                # deciding which data to move to the beginning:
                lastCoincidence = coincidence[-1]
                #      for the one considered "secondary" all events after the last coincidence go to the beginning
                newFirst = findPositionAfter((lastCoincidence*1000)+999, dataBottom)
                #print("new first is {}, length is {}".format(newFirst, len(dataBottom)))
                dataBottom = dataBottom[newFirst:]
                #      for the one considered "absolute", find "last coincidence + offset". This is the last place a concidence might be.
                #          now, find the first event after that number. This is the coincidence event.
                beforeFirst = findPositionAfter(((lastCoincidence-offset)*1000), dataTop)
                # just for now... how many events COULD have been the coincidental one
                beforeLast = findPositionAfter(((lastCoincidence+offset)*1000)+999, dataTop)
                #print("{} events could have been the coincidence we're looking for".format((beforeLast-beforeFirst)))
                #          all events after that go to the beginning.
                dataTop = dataTop[beforeFirst+1:]
        else:
            newCoincs = findCoincidences(dataBottom, dataTop)
            if minCoincTime is None and len(newCoincs) > 0:
                minCoincTime = min(newCoincs)
            for x in newCoincs:
                fileCoinc.write("{}\n".format(x))
                x -= minCoincTime
                coincidence.append(x)
            coincidence.sort()
            if len(newCoincs) > 0:
                # deciding which data to move to the beginning:
                lastCoincidence = coincidence[-1]
                #      for the one considered "secondary" all events after the last coincidence go to the beginning
                newFirst = findPositionAfter((lastCoincidence*1000)+999, dataTop)
                #print("new first is {}, length is {}".format(newFirst, len(dataTop)))
                dataTop = dataTop[newFirst:]
                #      for the one considered "absolute", find "last coincidence + offset". This is the last place a concidence might be.
                #          now, find the first event after that number. This is the coincidence event.
                beforeFirst = findPositionAfter(((lastCoincidence-offset)*1000), dataBottom)
                # just for now... how many events COULD have been the coincidental one
                beforeLast = findPositionAfter(((lastCoincidence+offset)*1000)+999, dataBottom)
                #print("{} events could have been the coincidence we're looking for".format((beforeLast-beforeFirst)))
                #          all events after that go to the beginning.
                dataBottom = dataBottom[beforeFirst+1:]
        if len(coincidence) > 0:
            numBins = int((max(coincidence) - min(coincidence)) / histBinTime) + 1
            thebins = [min(coincidence)+(histBinTime*n) for n in range(0, numBins+1)]
            graphList = list(set(coincidence))
            figure.clear()
            vals, bins, other = plt.hist(graphList, bins=thebins)
            plt.title("Muon hit coincidences over time")
            plt.ylabel("Num hit coincidences")
            plt.xlabel("Time (microsec)")
            plt.draw()
            plt.pause(0.5)
except KeyboardInterrupt:
    print("\nStopping...")
except BaseException as ex:
    log.error(str(ex))
    print(ex)
finally:
    log.info("Shutting down...")
    closeUp([fileTop, fileBottom, fileCoinc, topResp, bottomResp])
    log.info("Saving plot to {}/coincidences.png".format(dataHome))
    log.close()
    numRaw = len(coincidence)
    coincSet = set(coincidence)
    numTrue = len(coincSet)
    repeats = numRaw - numTrue
    print("Saving plot to {}/coincidences.png".format(dataHome))
    plt.savefig(dataHome+"/coincidences.png")
    print("{} repeated values out of {} in {} (no repeats in histogram).".format(repeats, numRaw, storageFileCoinc))
    print("Done")

