//Logic Overview
//Start Python script to continually poll serial port
//Python script writes Power values to Bridge key pairs
//Get Power values from Bridge key pairs
//Get Solar value from RF Receiver (Arduino UNO connected to Xantrex Invertor transmitting via RF)
//Write power and solar values to LCD
//Write power and solar values to daily log file 
//Send power and solar values to PVOutput

//Hardware
//Arduino Yun 
//16x2 LCD 
//433Mhz RF Receiver
//Current Cost Envi Power Monitor with Serial-USB cable

#include <Bridge.h>
#include <LiquidCrystal.h>
#include <Process.h>
#include <VirtualWire.h>
#include <Console.h>

LiquidCrystal lcd(4, 5, 6, 7,8,9);        // select the pins used on the LCD panel

// define pins used by the buttons
const int buttononePin = 12;     
const int buttontwoPin = 11;     

// variable for reading the pushbutton status
int buttononeState = 0;
int buttontwoState = 0;

unsigned long previousPVOutputTime = 0;     // will store last time data sent to pvoutput
const long pvoutput_interval = 300000;      // Interval to send to pvoutput (in milliseconds so 300000 = 5mins)

char charBridgeHousePower[10] = "0";	//Power that the house is consuming
char charBridgeSolarPower[10] = "0";	//Power that the solar panels are generating
char charBridgeSolarWHToday[10] = "0";	//Power that the solar panels have generated for the day
char charBridgeDateTime[20] = "0";		//Current date and time 

String strXantrexPOutput = "0";   //Stores Xantrex PVOutput

int intNoRFCount = 0;  //Keep track of number of times there is no RF transmission

void setup(void)
{
	Bridge.begin();
	Serial.begin(9600);
	Console.begin();
	lcd.begin(16, 2);

	Serial.println("Setup: CurrentCost Receiver on Yun");

	//Start Python script to capture Current Cost data
	startDataCapture();

	// initialize the pushbutton pin as an input:
	pinMode(buttononePin, INPUT_PULLUP);
	pinMode(buttontwoPin, INPUT_PULLUP);
	
	// Initialise the IO and ISR
	vw_set_ptt_inverted(true); // Required for DR3100
	vw_setup(2000);			   // Bits per sec
	vw_rx_start();			   // Start the RF receiver PLL running
}  //End setup

void loop(){
	unsigned long currentPVOutputTime = millis();  //Keeps timer to send data to PVOutput website every 5 mins

	String strHousePower = "Power: ";  //String for LCD display
	String strSolarPower = "Solar: ";  //String for LCD display
	String strRFInput = "";			//Stores received RF value
	String strXantrexWHToday = "";	//Xantrex WHToday value
	String strBatteryVcc = "";		//Battery voltage for the UNO Transmitter
	String strXantrexStatus = "";	//Can remove this?

	int indexfrom = 0;	//Stores from index of the delimiter of the received RF string
	int indexto = 0;	//Store to index of the delimiter of the received RF string

	Get_LCD_Key_Pressed();

	//Receive data from RF to get solar reading from Xantrex
	//POUT (Watts),WHTODAY (Watts),BAT (mV)
	strRFInput = Receive_RF();
	
	//If nothing received from the RF Receiver, increment the counter
	if (strRFInput.length() == 0) {
		intNoRFCount++;
		
		//Once the counter hits 50, assume that the inverter is offline so ensure POutput value is 0
		if (intNoRFCount > 50) {
			strXantrexPOutput = "0";
			intNoRFCount = 0;
		}
	}
	//Serial.println("NoRFCount = " + String(intNoRFCount));
	
	//Work through RF string to retrieve values from the Inverter
	//tokenfrom and tokento used to identify delimiters in RF string
	if (strRFInput.startsWith("Online")){
		strXantrexStatus = "Online";
		
		//Get POut value
		indexfrom = strRFInput.indexOf(",")+1;
		indexto = strRFInput.indexOf(",", indexfrom);
		strXantrexPOutput = strRFInput.substring(indexfrom,indexto);
		Serial.println("POutput = " + strXantrexPOutput);
		
		//Get WHToday value
		indexfrom = strRFInput.indexOf(",", indexto)+1;
		indexto = strRFInput.indexOf(",", indexfrom);
		strXantrexWHToday = strRFInput.substring(indexfrom, indexto);
		
		//Get Battery voltage
		indexfrom = strRFInput.indexOf(",", indexto) + 1;
		strBatteryVcc = strRFInput.substring(indexfrom);
	}

	//Put inverter solar value on the Bridge
	//Do this first as Solar power now received directly from the inverter
	Bridge.put("SolarPower", String(strXantrexPOutput));
	Bridge.put("WHToday", String(strXantrexWHToday));
	
	//Query key pair values from the Bridge
	Bridge.get("HousePower", charBridgeHousePower, 10);
	Bridge.get("SolarPower", charBridgeSolarPower, 10);
	Bridge.get("SolarWHToday", charBridgeSolarWHToday, 10);
	Bridge.get("DateTime", charBridgeDateTime, 20);
	
	//Output values on LCD
	lcd.setCursor(0, 0);
	lcd.print(String(strHousePower + charBridgeHousePower + "     "));
	lcd.setCursor(0, 1);
	lcd.print(String(strSolarPower + charBridgeSolarPower + "     "));

	//Send data to PVOutput at specified time inteval
	if (currentPVOutputTime - previousPVOutputTime >= pvoutput_interval) {
		previousPVOutputTime = currentPVOutputTime;      // save the last time data was sent to PVOutput 
		Send_PVOutput();
	}

	//Debugging
	Serial.println(String(charBridgeDateTime));
	Serial.println(String(strSolarPower + charBridgeSolarPower + " POutput = " + strXantrexPOutput + " WHToday = " + strXantrexWHToday));
	//Serial.println(currentPVOutputTime);
	//Serial.println(previousPVOutputTime);
	//Serial.println(pvoutput_interval);
	//Serial.println();

	//Console.println(String(charBridgeDateTime));
	//Console.println(String(strSolarPower + charBridgeSolarPower + " POutput = " + strXantrexPOutput + " WHToday = " + strXantrexWHToday));
	//Console.println();
	delay(1000);

}//end of Loop

////////////////
// FUNCTIONS //
///////////////
void startDataCapture() {
	// Launch "currentcost.sh" command to start capture data from Current Cost
	// This Python script will constantly run on the CPU processing 6sec data from the CurrentCost
	Serial.println("Starting python script...");
	Process command;        // Create a process and call it "command"
	//ensure Process is running before continuing
	while (!command.running()){
		command.runShellCommandAsynchronously("/mnt/sda1/currentcost/currentcost.sh");
		delay(5000); 
	}
	Serial.println("Python script running...");
}

void Send_PVOutput() {
	// Launch "pvoutput.sh" command to send data to PVOutput
	Serial.println("Starting PVOutput python script...");
	Process command;        // Create a process and call it "command"
	command.runShellCommandAsynchronously("/mnt/sda1/currentcost/pvoutput.sh");
	Serial.println("Data sent to PVOutput...");
}

void Get_LCD_Key_Pressed(){
	buttononeState = digitalRead(buttononePin);
	buttontwoState = digitalRead(buttontwoPin);

	// check if the pushbutton is pressed.
	// if it is, the buttonState is HIGH:
	if (buttononeState == HIGH)  {
		// turn LED off:
		//digitalWrite(ledPin, LOW);
	}
	else if (buttononeState == LOW) {
		// turn LED on:
		//digitalWrite(ledPin, HIGH);
		//Serial.println("Button one pressed");
		//Console.println("Button one pressed");
	}
	if (buttontwoState == HIGH) {
		// turn LED off:
		//digitalWrite(ledPin, LOW);
	}
	else if (buttontwoState == LOW){
		// turn LED on:
		//digitalWrite(ledPin, HIGH);
		//Serial.println("Button two pressed");
		//Console.println("Button two pressed");
	}
}

String Receive_RF(){
	uint8_t buf[VW_MAX_MESSAGE_LEN];
	uint8_t buflen = VW_MAX_MESSAGE_LEN;
	String strRFReading;

        //Serial.println("Polling RF Signal");

	if (vw_get_message(buf, &buflen)) // Non-blocking
	{
		int i;

		digitalWrite(13, true); // Flash a light to show received good message
		// Message with a good checksum received, dump it.
		Serial.print("Got: ");

		for (i = 0; i < buflen; i++)
		{
			strRFReading = strRFReading + char(buf[i]);
			//Serial.print(char(buf[i]));
			//Console.print(" ");
		}
		Serial.println(strRFReading);
		digitalWrite(13, false);
		return strRFReading;
	}
}

