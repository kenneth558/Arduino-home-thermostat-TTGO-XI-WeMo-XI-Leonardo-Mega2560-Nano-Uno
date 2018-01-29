# Arduino_UNO_home_thermostat
I used this sketch in the Arduino UNO for a while to operate my furnace and you can easily modify it for A/C and humidistat.  Because this version uses interrupts (uselessly, at that) to read from the sensor, it limits the senosr to being used only on pins 2 or 3, and I recommend you use my updated version.  I since have changed the DHT data acquisition code not to use interrupts so that and and ALL available digital pins can connect to a sensor.  That sketch version is found in another branch.

It is readable and settable from a host computer, if connected to one via USB.  For maximum capability, the host computer will run [a] capturing daemon[s] to receive and process the logging output from the Arduino.  See the auxiliary host processing scripting that I use in Ubuntu in the file above titled similarly.

Being connected via USB to my Ubuntu headless firewall, I have remote control of it from over the Internet. It will send alerts to the host for furnace failure to heat, which I use by a bash script on the host to email me in that event.  See the screen shot of the help screen that it can display.

Sensors must be either DHT11 or DHT22 in this version.  TODO: I2C and port expansion....
