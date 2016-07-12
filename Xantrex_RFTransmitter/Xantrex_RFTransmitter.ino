// Xantrex_RFTransmitter.ino
// Russell Taylor (rusty02@hotmail.com)
//
// Reads output from a Xantrex Solar Inverter and sends the value via RF to another Arduino
//
// Hardware and Software used
// Xino Basic ATMEL (DIY Arduino UNO Clone without power hungry components)
// 433 RF Transmitter (purchased from Jaycar)
// Xantrex GT 2.8 Invertor (should work with any Xantrex with serial port)
// Serial Cable with male DB9 - pinouts are: pin 2 (Tx), pin 3 (Rx), pin 5 (ground).
// IDE version 1.5.x
// VirtualWire library version 1.27 (http://www.airspayce.com/mikem/arduino/VirtualWire)

// Overview
// Read current solar power, Watt Hours for today from Invertor and battery voltage
// If solar power = zero, sleep for 1 minute before trying again.  Don't send anything over RF
// If solar power > zero, send Xantrex string over RF to a receiver. Sleep for 5sec
// Loop back to start

/////////////////////////////////////////////////////////////////////////////////////////
// 
// Transmitter code originally by Mike McCauley
// Mike McCauley (mikem@airspayce.com)
//
// Xantrex functions by Isaac Hauser (izy@zooked.com.au)
//
//                     Version Control
// _____________________________________________________________________
// | Date		| Reason								|	Author            
// | 8/1/15		| Initial Version						| Russell Taylor
// |
// |

#include <VirtualWire.h>
#include <SoftwareSerial.h>
#include <Sleep_n0m1.h>

Sleep sleep;
unsigned long sleepTimeOnline;	//Stores how long the arduino sleeps for while inverter is online
unsigned long sleepTimeOffline; //Stores how long the arduino sleeps for while inverter is offline

// Serial cable from Xantrex is connected to Pins 8(Rx) and 9(Tx).
//DB9 pinouts are: pin 2 (Tx), pin 3 (Rx), pin 5 (ground).
#define rxPin 2
#define txPin 3

SoftwareSerial xantrexSerial(rxPin, txPin);

int MAXTRIES = 50;	// set max number of times to try getting a useful value from the inverter

int DEBUG = 1;  //1 for debug messages. 0 for no debug messages

void setup()
{
	pinMode(txPin, OUTPUT);	//Serial cable to Xantrex
	pinMode(rxPin, INPUT);	//Seril cable to Xantrex
	pinMode(13, OUTPUT);	//this was used as led send/rec indicator so can remove when in production
	xantrexSerial.begin(9600);  
    
	if (DEBUG == 1) {
		Serial.begin(9600);
		Serial.println("Setup: Transmitter on UNO");
	}

    // Initialise the IO and ISR
    vw_set_ptt_inverted(true);	// Required for DR3100
    vw_setup(2000);				// Bits per sec

	sleepTimeOnline = 6000;		//set sleep time in ms, max sleep time is 49.7 days.  6000 = 6sec
	sleepTimeOffline = 60000;	//60000 = 1min
}	//End of Setup

void loop()
{
	int SolarPOutput = get_pout();	//Gets and stores Xantrex Solar power output
	int SolarWHToday = 0;			//Stores Xantrex Solare generated today amount
	long lngBatteryVcc = 0;			//Stores battery voltage
	char msg[40];					//Message to transmit over RF

	if (DEBUG == 1){
		Serial.println("Status: " + String(get_status()));  //Not working
		Serial.println("Power: " + String(get_pout()));
		Serial.println("Temp: " + String(get_temp()));
		Serial.println("Energy: " + String(get_whtoday()));
		Serial.println("Voltage: " + String(get_voltage()));
		Serial.println("vcc: " + String(readVcc()));
		Serial.println("Standby Status: " + String(get_standby()));  //Not working
	}

	if (SolarPOutput == 0) {  //Try a second time if POutput is zero
		delay(50);
		SolarPOutput = get_pout();
	}

	if (SolarPOutput > 0) {  //Invertor online so process output
		if (DEBUG == 1){ Serial.println("Power Output greater than zero"); }
		SolarWHToday = get_whtoday();
		lngBatteryVcc = readVcc();

		//Send Inverter output over RF
		//Format is Online,POUT,WHTODAY,BATVCC
		SolarWHToday = get_whtoday();
		SolarWHToday = get_whtoday();
		sprintf(msg, "%s,%i,%i,%i", "Online", SolarPOutput, SolarWHToday, lngBatteryVcc);
		if (DEBUG = 1){ Serial.println("Xantrex Output = " + String(msg)); }
		//Send message over RF
		vw_send((uint8_t *)msg, strlen(msg));
		vw_wait_tx(); 
		if (DEBUG == 1){ Serial.println("Xantrex Output Sent"); }
		Serial.flush();
		//Go to sleep to save power
		sleep.pwrDownMode(); //set sleep mode
		sleep.sleepDelay(sleepTimeOnline); //sleep (6sec)
	}

	//if (SolarPOutput == 0) {  //Inverter may be offline.  Or maybe not?, just no sun! so keep checking every minute
		//Put into Sleep mode for 1mins
	//	if (DEBUG == 1){ Serial.println("Power Output is zero.  Going to sleep..."); }
	//	sleep.pwrDownMode(); //set sleep mode
	//	sleep.sleepDelay(sleepTimeOffline); //sleep (1min)
	//}

	
}	//End of Loop

///////////////
// FUNCTIONS //
///////////////

// returns power output of inverter in watts
int get_pout()
{
	// loop till we get a good value
	int pout = 0;
	int trys = 0;
	while (pout == 0 && trys < MAXTRIES)
	{
		// send command to inverter
		xantrexSerial.println("POUT?");
		delay(50);

		// read response into array of chars
		char received[64];
		int count = 0;
		while (xantrexSerial.available() > 0)
		{
			received[count++] = xantrexSerial.read();
		}

		// convert array of chars into a number we can use
		pout = atoi(received);
		trys++;
	}
	return pout;
}


// returns temperature of inverter in degress C
float get_temp()
{
	// loop till we get a good value
	float temp = 0;
	int trys = 0;
	while (temp == 0 && trys < MAXTRIES)
	{
		// send command to inverter
		xantrexSerial.println("MEASTEMP?");
		delay(50);

		// read response into array of chars
		char received[64];
		int count = 0;
		while (xantrexSerial.available() > 0)
		{
			received[count++] = xantrexSerial.read();
		}

		// select only the bits of the recevied data we are interested in
		char tempArray[3] = { received[2], received[3], received[5] };
		// convert to a number we can use
		temp = atof(tempArray) / 10;
		trys++;
	}
	return temp;
}


// returns energy generated today in watt hours
int get_whtoday()
{
	// loop till we get a good value
	int whtoday = 0;
	int trys = 0;
	while (whtoday == 0 && trys < MAXTRIES)
	{
		// send command to inverter
		xantrexSerial.println("KWHTODAY?");
		delay(50);

		// read response into array of chars
		char received[64];
		int count = 0;
		while (xantrexSerial.available() > 0)
		{
			received[count++] = xantrexSerial.read();
		}

		// need to deal with 2 cases, 0.000 to 9.999kWh and 10.000 to 99.999kWh
		if (received[1] == '.') {
			// select only the bits of the recevied data we are interested in
			char whtodayArray[5] = { received[0], received[2], received[3], received[4] };
			// convert to a number we can use
			whtoday = atoi(whtodayArray);
		}
		else if (received[2] == '.') {
			// select only the bits of the recevied data we are interested in
			char whtodayArray[6] = { received[0], received[1], received[3], received[4], received[5] };
			// convert to a number we can use
			whtoday = atoi(whtodayArray);
		}
		trys++;
	}
	return whtoday;
}


// check if inverter relay is on or off which shound tell us if inverter is online
boolean get_status()
{
	int inverter = -1;
	int trys = 0;
	while (inverter == -1 && trys < MAXTRIES)
	{
		// send command to inverter
		xantrexSerial.println("RELAY?");
		delay(50);

		// read response into array of chars
		char received[64];
		int count = 0;
		while (xantrexSerial.available() > 0)
		{
			xantrexSerial.print(char(xantrexSerial.read()));
			received[count++] = xantrexSerial.read();
		}

		char statusArray[4] = { received[0], received[1], received[2] };
		//char statusArray[4] = "ERR"; // test string to see how it handles errors
		// compare array to "OFF" if it matches inverter relay is off
		if (strcmp(statusArray, "OFF") == 0)
		{
			return 0;
		}
		// compary array to "OFF" if the difference is 8 then the word is ON, so the relay is on
		else if (strcmp(statusArray, "OFF") == 8)
		{
			return 1;
		}
		// if we get down here, the inverter gave us rubbish, repeat till we get useful data
		else
		{
			trys++;
		}
	}
	return -1;
}


// returns ac voltage of inverter
float get_voltage()
{
	// loop till we get a good value
	float voltage = 0;
	int trys = 0;
	while (voltage == 0 && trys < MAXTRIES)
	{
		// send command to inverter
		xantrexSerial.println("VOUT?");
		delay(50);

		// read response into array of chars
		char received[64];
		int count = 0;
		while (xantrexSerial.available() > 0)
		{
			received[count++] = xantrexSerial.read();
		}

		// select only the bits of the recevied data we are interested in
		char voltageArray[5] = { received[0], received[1], received[2], received[4] };
		// convert to a number we can use
		voltage = atof(voltageArray) / 10;
		trys++;
	}
	return voltage;
}

long readVcc() {
	// Read 1.1V reference against AVcc
	// set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
	ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
	ADMUX = _BV(MUX3) | _BV(MUX2);
#else
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif  

	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Start conversion
	while (bit_is_set(ADCSRA, ADSC)); // measuring

	uint8_t low = ADCL; // must read ADCL first - it then locks ADCH  
	uint8_t high = ADCH; // unlocks both

	long result = (high << 8) | low;

	result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	//result = 1143550L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	return result; // Vcc in millivolts
}

// check if inverter relay is on or off which shound tell us if inverter is online
boolean get_standby()
{
	int inverter = -1;
	int trys = 0;
	while (inverter == -1 && trys < MAXTRIES)
	{
		// send command to inverter
		xantrexSerial.println("STANDBY?");
		delay(50);

		// read response into array of chars
		char received[64];
		int count = 0;
		while (xantrexSerial.available() > 0)
		{
			xantrexSerial.print(char(xantrexSerial.read()));
			received[count++] = xantrexSerial.read();
		}

		char statusArray[4] = { received[0], received[1], received[2] };
		//char statusArray[4] = "ERR"; // test string to see how it handles errors
		// compare array to "OFF" if it matches inverter relay is off
		if (strcmp(statusArray, "OFF") == 0)
		{
			return 0;
		}
		// compary array to "OFF" if the difference is 8 then the word is ON, so the relay is on
		else if (strcmp(statusArray, "OFF") == 8)
		{
			return 1;
		}
		// if we get down here, the inverter gave us rubbish, repeat till we get useful data
		else
		{
			trys++;
		}
	}
	return -1;
}



