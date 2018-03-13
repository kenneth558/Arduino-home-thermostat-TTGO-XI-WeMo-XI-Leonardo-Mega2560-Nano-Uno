You'll need to download four files: the .ino and the three .h files.  IF USING TTGO XI/WeMo XI YOU'LL ALSO NEED WHAT YOU'LL FIND AT   https://github.com/wemos/Arduino_XI  (IDE support for TTGO XI/WeMo XI), BUT IF YOU COMPILE UNDER LINUX, READ THAT ISSUES SECTION FOR THE BOARDS.TXT FILE MODIFICATION DUE TO THE DIRECTORY DELIMITER CHARACTER DIFFERENCE ( \ -> / ) 
# Arduino_home_thermostat
I use this sketch in the Arduino UNO, Mega2560, Leonardo and Wemo XI (TTGO XI) as my thermostat, and you can easily modify it for humidistat as well.  I have compiled it for all boards I could select in the IDE and ensured it would compile for the boards having at least 32K flash.  This version allows the thermostat mode of "auto" in all boards but does not have KY-013 analog sensor calibration.

Currently DHT11, DHT22, and KY-013 sensors only are supported.  Obviously, the DHT sensors can connect to Digital and dual-mode pins while the KY-013 can connect to Analog and dual-mode pins. Note that the accuracy of a KY-013 analog sensor depends on the DC voltage supplying the "-" pin (these sensors are labeled opposite what the sketch formula is!)  For the best accuracy, supply these sensors with an adjustable and stable supply of about 3.3 to 4.5 volts.  The TTGO XI/WeMo XI boards that I have gives accurate readings when the 3v3 pin supplies the sensor (3.3 volts).  Adjust the voltage for accurate readings and enjoy.

This sketch also makes Arduino digital pins to be readable and settable from an optional serial-connected host computer.  The host computer can control and read both the digital voltage levels and DHT data of any pin.  For maximum capability, the host computer can optionally run [a] capturing daemon[s] to receive and process the logging output from the Arduino.  Temperature changes logging, an option if main logging is also on, is on by default.  The host computer can utilize temperature changes data to track and analyze or to incorporate into a ceiling fan control algorithm, just to name a couple uses.  See the auxiliary host processing scripting that I use in Ubuntu in the file above titled similarly.  In Ubuntu remember to grant membership to the dialout group for the host Ubuntu computer to establish serial communications to the Arduino.

Being connected via USB to my Ubuntu headless firewall, I have remote control of it from over the Internet.  It will send alerts to the host for furnace failure to heat. I use a bash script on the host to email me in event of furnace failure.  EEPROM is utilized so the thermostat settings are persistent across power cycling.  I even update the compiled sketch into the board remotely!

See the screen shot of the help screen that it can display.  Sensors must be either DHT11, DHT22, or KY-013 in this version.  It expects two sensors - a primary and a backup (secondary) sensor for failsafe operation.  Because sensors are read without utilizing interrupts, any digital pin other than the serial communications can be used for DHT sensors!  A little-documented feature is that the pins driving sensors can be forced to only allow DHT devices by adding 128 to the EEPROM location storing the digital pin number for that sensor.  This is a safety feature: in case the DHT sensor fails, the 128 added to the pin number stored in EEPROM prevents the pin from going into analog mode and interpreting any voltage appearing on the pin as a signal from an analog (KY-013) temperature sensor.

This sketch is also a wrapper to allow the host computer to read and control the Arduino digital pins with or without taking advantage of the thermostat functionality!  As examples:

-  I have my host computer calculate when my porchlights are to be turned on and off through a pin by lookup file of sunset and sunrise times

-  I have my coffee maker outlet connected to another pin and host-controlled on a schedule 

-  The host computer can read temperatures and humidities from more DHT11/22 devices on other pins.  Outdoor temperature and humidity can thus be acquired by the host computer

-  Ceiling fan control

-  Etc., etc.!  See the help screen for details.

This sketch occupies all or nearly all available flash (program) memory in boards having 32K of flash.  Therefore, you'll not be able to add features in most 32K flash Arduinos.  Actual compiled sketch size varies in the range of 27K-28K, so boards that allow the entirety of the 32K flash for the user's sketch will have some flash space available for you to add features.  Note that certain of these boards don't even have have enough flash to both initialize EEPROM and advance to production environment operation in the same compilation.  Be prepared to compile twice for Leonardo, TTGO, Micro, et. al. - once for EEPROM intitializing and the final time for production environment operation.

TTGO XI/WeMo XI NOTE:  This board trades flash to simulate EEPROM at a cost of 2 to 1.  This sketch assumes your compiler EEPROM command line settings are set for 1K EEPROM.  That is the minimum block size of EEPROM the board allows.  Any more EEPROM by the compiler and this sketch will not fit.  Fortunately for me, my default compiler settings were correct to this requirement, otherwise I wouldn't know how to change them back to what they are.

IMPORTANT SAFETY NOTE - Proper electrical isolation by relay or opto-isolation must be observed whenever connecting to building electrical or furnace controls or other electrical equipment.  For the furnace controls, I use ralays driven by open-collector NPN transistors with a 12vdc power supply.  Don't forget the diode across the relay coil.  For 120vac switching, I use opto-coupled dc-drive SSRs, again, driven by NPN transistors and 12vdc.

TODO - Future enhancements are reserved for boards having greater than 32K flash size: I2C port expansion, duct damper operation for multi-thermostat-single-furnace environments.

WANTED:  ASSISTANCE CONDENSING THE SIZE OF THIS SKETCH BY USE OF ASM CODE.  ANY TAKERS?  POST AN ISSUE, PLEASE.
