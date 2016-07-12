I’ve had a Current Cost Power Monitor for a number of years which worked well until we put solar on the roof.  The Current Cost can’t determine which direction the power is flowing and so power generation and consumption are added together on the Current Cost display rendering it unusable.
The solution I came up with is actually two projects that communicate via RF.  
1.	Arduino Yun connects to the Current Cost via the USB port to capture power consumption.  
2.	Arduino Uno/Xino Basic for Atmel connects to a Xantrex Grid Tie Solar Inverter to capture power generation
Power consumption is calculated by the difference between the CurrentCost and the Xantrex Inverter.  Results are displayed via a LCD screen.
