#! /usr/bin/env python

from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from logger import Logger
import time
from ctypes import *

# default values
logfile = "/var/log/muon_timer/server.log"
port = 8090
# this is stupid but it works
log = Logger(logfile)

class timespec(Structure):
    _fields_ = [('tv_sec', c_long), ('tv_nsec', c_long)]

class ReqHandler(BaseHTTPRequestHandler):

    def _set_headers(self):
        self.send_response(200)
        #self.send_header('Transfer-Encoding', 'chunked')
        self.send_header('Content-Type', 'text/html')
        self.end_headers()

    def _set_fail_headers(self, code, failInfo):
        self.send_response(code)
        self.send_header('Content-Type', 'text/html')
        self.send_header('Content-Length', str(len(failInfo)))
        self.end_headers()

    def do_GET(self):
        log.info("Received request: "+self.requestline+" from "+str(self.client_address))
        self._set_headers()
        ts = timespec()
        self.wfile.write('#cnt, ts.tv_sec, ts.tv_nsec')
        try:
            with open('/dev/muon_timer', 'rb') as muons:
                cnt = 0
                while(True):
                    cnt+=1
                    muons.readinto(ts)
                    time.sleep(100./1000.)
                    self.wfile.write("{} {} {}\n".format(cnt, ts.tv_sec, ts.tv_nsec))
        except BaseException as ex:
            msg = "ERROR: "+str(ex)+"\n"
            log.error("Failed while writing response to "+str(self.client_address)+": "+msg)
            self._set_fail_headers(500, msg)
            self.wfile.write(msg)
        log.info("response ended")

    def do_HEAD(self):
        self._set_headers()
        
    def do_POST(self):
        msg = "POST requests not supported"
        self._set_fail_headers(400, msg)
        self.wfile.write(msg)

def run(server_class=HTTPServer, handler_class=ReqHandler, port=port):
    log.info("Starting up server at port "+str(port))
    server_address = ('', port)
    httpd = server_class(server_address, handler_class)
    print 'Starting httpd...'
    httpd.serve_forever()

if __name__ == '__main__':
    log.filename = logfile
    try:
        log.setup()
    except BaseException as ex:
        print ex

    try:
        run()
    except KeyboardInterrupt:
        log.info("Shutting down")
        log.close()
    except BaseException as ex:
        log.error(str(ex))
        log.info("Shutting down")
        log.close()