# Arduino_UNO_home_thermostat
I use this sketch in the Arduino UNO to operate my furnace, and you can easily modify it for A/C and humidistat.  It is readable and settable from a host computer, if connected to one via USB.  For maximum capability, the host computer can optionally run [a] capturing daemon[s] to receive and process the logging output from the Arduino.  See the auxiliary host processing scripting that I use in Ubuntu in the file above titled similarly.

Being connected via USB to my Ubuntu headless firewall, I have remote control of it from over the Internet. It will send alerts to the host for furnace failure to heat, which I use by a bash script on the host to email me in that event.  

See the screen shot of the help screen that it can display.  Sensors must be either DHT11 or DHT22 in this version.  It expects two DHT sensors - a primary and a backup sensor for failsafe operation, called a secondary.  Because NO interrupts are used, the sensors can be connected to any digital pin!  

Future plans TODO: I2C and port expansion....
