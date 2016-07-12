#!/usr/bin/env python
# post Power consumption and Generation to pvoutput.org
# Original code/concept by Andrew Tridgell 2010

#Called by Arduino script via Bridge Process

import os  #for file management
import sys #sending errors to stderr
import time # for printing UNIX timecode
#import csv  #not sure what this is for, perhaps not needed
import json # for posting to PVOutput
import httplib # for posting to PVOutput
import urllib  # for posting to PVOutput
import xml.etree.ElementTree as ET # for XML parsing
#sys.path.insert(0, '/usr/lib/python2.7/bridge')
#from bridgeclient import BridgeClient as bridgeclient  #For Yun Bridge

#########################################
#            CONSTANTS                  #
#########################################
configTree   = ET.parse("config.xml")          # load config from config file
apikey       = configTree.findtext("apikey")   # Your PVOutput API Key
systemid     = configTree.findtext("systemid") # Your PVOutput System ID
host         = configTree.findtext("host")     # PVOutput hostname
service      = configTree.findtext("service")  # PVOutput service URL
debug        = True
testrun      = False
EnableBridge = True  # Used during offline development of Arduino.  Can be removed for production

# EnableBridge used during offline development of Arduino.  Can be removed for production
if EnableBridge == True:
    sys.path.insert(0, '/usr/lib/python2.7/bridge')
    from bridgeclient import BridgeClient as bridgeclient  #For Yun Bridge
    #Instantiate Yun Bridge
    bridgevalue = bridgeclient()

def post_status(date, time, SolarPower, HousePower):
    '''post status to pvoutput'''
    params = urllib.urlencode({ 'd' : date,
                                't' : time,
                                #'v1' : energy,
                                'v2' : SolarPower,
                                'v4' : HousePower})
    headers = {"Content-type": "application/x-www-form-urlencoded",
               "Accept": "text/plain",
               "X-Pvoutput-SystemId" : systemid,
               "X-Pvoutput-Apikey" : apikey}
    if debug:
        print("Connecting to %s" % host)
    conn = httplib.HTTPConnection(host)
    if debug:
        print("sending: %s" % params)
    conn.request("POST", service, params, headers)
    response = conn.getresponse()
    if response.status != 200:
        print "POST failed: ", response.status, response.reason, response.read()
        sys.exit(1)
    if debug:
        print "POST OK: ", response.status, response.reason, response.read()

##################
# Main procedure #
##################
if EnableBridge == True:
    #Get current Bridge key values
    HousePower = bridgevalue.get("HousePower")
    SolarPower = bridgevalue.get("SolarPower")
else:
    #Testing values only
    SolarPower = "300"
    HousePower = "1000"
#e_today = res['GriEgyTdy']*1000
date_str = time.strftime("%Y%m%d")
time_str = time.strftime("%H:%M")
if testrun:
    print(date_str, time_str, SolarPower, HousePower)
else:
    print(date_str, time_str, SolarPower, HousePower)
    if HousePower > 0.0:
        post_status(date_str, time_str, SolarPower, HousePower)

#Delete status for a given date
#http://pvoutput.org/service/r2/deletestatus.jsp?sid=34100&key=bdb3ca94cdae6db070721d412cee4ce437131256&d=20141230