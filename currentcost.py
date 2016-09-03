#! /usr/bin/python

# Code to read from Current Cost originally from https://github.com/JackKelly/currentCostCosmTX
# Called by Arduino script via Bridge Process

import serial # for pulling data from Current Cost
import xml.etree.ElementTree as ET # for XML parsing
import time # for Date Time management
import sys # sending errors to stderr
import os # file management
sys.path.insert(0, '/usr/lib/python2.7/bridge')
from bridgeclient import BridgeClient as bridgeclient  #For Yun Bridge

#########################################
#            CONSTANTS                  #
#########################################

configTree  = ET.parse("config.xml") # load config from config file
SERIAL_PORT = configTree.findtext("serialport") # the serial port to which your Current Cost is attached
API_KEY     = configTree.findtext("apikey") # Your pvoutput API Key
FEED        = configTree.findtext("feed")   # Your pvoutput Feed number
#FILENAME    = configTree.findtext("scriptpath") + time.strftime("%d-%m-%Y") + ".csv" # File to save local data to
SCRIPTPATH    = configTree.findtext("scriptpath")  # Script path where files are
Device     = "Arduino" # Valid options are PC or Arduino  (debugging only)

#Instantiate Yun Bridge
bridgevalue = bridgeclient()

#####################################################################################################################################
#     SENSOR NAMES                                                                                                                  #
# Define what sensor is connected to Solar                                                                                          #
# Array index numbers align to the sensor numbers                                                                                   #
# This script assumes that the whole house sensor currently includes the generated Solar amount and removes it from sensor 1 (House)#
#####################################################################################################################################
sensorNames = ["HouseSensor","SolarSensor"]

#########################################
#          PULL FROM CURRENT COST       #
#########################################

def pullFromCurrentCost(solargeneration):
    ''' read line of XML from the CurrentCost meter and return instantaneous power consumption. '''
    # For Current Cost XML details, see currentcost.com/cc128/xml.htm
    
    # Read XML from Current Cost.  Try again if nothing is returned.
    watts  = None
    wattsch1 = None
    wattsch2 = None
    wattsch3 = None
    sensor = None

    while watts == None:
        line = ser.readline()        
        try:
            tree  = ET.XML( line )
            sensor = tree.findtext("sensor")
            wattsch1  = tree.findtext("ch1/watts")
            wattsch2  = tree.findtext("ch2/watts")
            wattsch3  = tree.findtext("ch3/watts")
            if wattsch1 is not None and wattsch2 is not None and wattsch3 is not None: #3 phase power
			    watts = int(wattsch1) + int(wattsch2) + int(wattsch3) - int(solargeneration)
            else: #Single phase power
                watts = int(wattsch1)

            #Deal with solar generation
            if sensorNames[int(sensor)] == "SolarSensor": 
                solargeneration = int(watts)
                if solargeneration < 70: watts = 0 #Solar reads a small amount of power at night time which is incorrect
            line = None

        except Exception, inst: # Catch XML errors (occasionally the current cost outputs malformed XML)
            sys.stderr.write("XML error: " + str(inst) + "\n")
            line = None
        
    ser.flushInput()
    return sensor, watts, solargeneration

	
#########################################
#          MAIN                         #
#########################################

# initialise serial port
ser = serial.Serial(SERIAL_PORT, 57600)
ser.flushInput()

solargeneration = 0

# continually pull data from current cost, write to file, print to stout and send to Cosm
while True:
    # Get data from Current Cost
    sensor, watts, solargeneration = pullFromCurrentCost(solargeneration)
    
    #Get Solar Power from Bridge
    bridgesolar = bridgevalue.get("SolarPower")
    WHToday = bridgevalue.get("WHToday")
    Arduino_Battery = bridgevalue.get("VCCBatt")
    	
    FileName = SCRIPTPATH + time.strftime("%d-%m-%Y") + ".csv"
    # open data file
    try:
        datafile = open(FileName, 'a+')
    except Exception, e:
        sys.stderr.write("""ERROR: Failed to open data file " + E + FileName Has it been configured correctly in config.xml?""")
        raise

    # Print data to the file
    UNIXtime = str ( int( round(time.time()) ) )
    PollDate = time.strftime("%d/%m/%Y")
    PollTime = time.strftime("%H:%M:%S")
    PollDateTime = PollDate + " - " + PollTime
    string   = PollDate + "," + PollTime + "," + str(sensor) + "," + sensorNames[int(sensor)] + "," + str(watts - int(bridgesolar)) + ",," + "\n"
    string   = string + PollDate + "," + PollTime + "," + "1,HouseSolar," + str(bridgesolar) + "," + str(WHToday) + "," + str(Arduino_Battery) + "\n"
    datafile.write( string )
    datafile.close()
    print string,
	
    #Write values to the Bridge for the MCU to access
    bridgevalue.put("DateTime", PollDateTime)
    if sensorNames[int(sensor)] == "HouseSensor":
        bridgevalue.put("HousePower", str(watts - int(bridgesolar)))
    #Comment this out as now getting Solar Power direct from Xantrex on the MCU
    #elif sensorNames[int(sensor)] == "SolarSensor":
    #    bridgevalue.put("SolarPower", str(watts))

    sys.stdout.flush()

