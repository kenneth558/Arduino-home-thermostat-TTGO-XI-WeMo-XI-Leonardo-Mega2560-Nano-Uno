# Arduino_home_thermostat
I use this sketch in the Arduino UNO, Mega2560, Leonardo and Wemo XI as my furnace thermostat, and you can easily modify it for A/C and humidistat.  Currently DHT11 and DHT22 only, but I'll need to eventually allow greater-longevity sensors for my own use.  It also makes Arduino digital pins to be readable and settable from an optional serial-connected host computer.  The host computer can control and read both the digital voltage levels and DHT data of any pin.  For maximum capability, the host computer can optionally run [a] capturing daemon[s] to receive and process the logging output from the Arduino.  Temperature changes logging, an option if main logging is also on, is on by default.  The host computer can utilize temperature changes data to track and analyze or to incorporate into a ceiling fan control algorithm, just to name a couple uses.  See the auxiliary host processing scripting that I use in Ubuntu in the file above titled similarly.  In Ubuntu remember to grant membership to the dialout group for the host Ubuntu computer to establish serial communications to the Arduino.

Being connected via USB to my Ubuntu headless firewall, I have remote control of it from over the Internet.  It will send alerts to the host for furnace failure to heat. I use a bash script on the host to email me in event of furnace failure.  EEPROM is utilized so the thermostat settings are persistent across power cycling.

See the screen shot of the help screen that it can display.  Sensors must be either DHT11 or DHT22 in this version.  It expects two DHT sensors - a primary and a backup (secondary) sensor for failsafe operation.  Because sensors are read without utilizing interrupts, so any digital pin other than the serial communications can be used for sensors!  

This sketch is also a wrapper to allow the host computer to read and control the Arduino digital pins with or without taking advantage of the thermostat functionality!  As examples:

-  I have my host computer calculate when my porchlights are to be turned on and off through a pin by lookup file of sunset and sunrise times

-  I have my coffee maker outlet connected to another pin and host-controlled on a schedule 

-  The host computer can read temperatures and humidities from more DHT11/22 devices on other pins.  Outdoor temperature and humidity can thus be acquired by the host computer

-  Ceiling fan control

-  Etc., etc.!  See the help screen for details.

IMPORTANT SAFETY NOTE - Proper electrical isolation by relay or opto-isolation must be observed whenever connecting to building electrical or furnace controls or other electrical equipment.

Future plans TODO: I2C, port expansion, duct damper operation for multi-thermostat-single-furnace environments.
