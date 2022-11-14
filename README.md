# CargaSEG
CargaSEG is a software for monitoring pallets with fragile cargo that needs to be refrigerated and cennot be tipped over.

It is implemented using MbedOS v2, running on the ARM microcontroller development board Kinetis FRDM-KL25Z.


## Sensors used
### Sensors from the board itself
 - TSI (capacitive touch slider);
 - RGB LED;
 - MMA8451Q accelerometer.
 
### External sensors and components
 - LCD 16x2 Display;
 - LM35 temperature sensor;
 - Tactile button + 10k resistor;
 - Toshiba ULN2003APG Darlington  stepper motor driver;
 - 28BYJ-48 stepper motor (5V).
