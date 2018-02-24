#define VERSION "0.0.0043"//;//TODO:  add labels to pins **************************WORKING ON LINES AT OR NEAR 1214-1225, must convert to remove all string objects due to wemo libraries not being string compatible
#ifndef __LGT8FX8E__
    short unsigned _baud_rate_ = 57600;//Very much dependent upon the capability of the host computer to process talkback data, not just baud rate of its interface
#else
    short unsigned _baud_rate_ = 19200;//In production environment the XI tends to power up at baud 19200 so we can't risk setting baud to anything but that
#endif
/*
...TODO: Add more externally-scripted functions, like entire port pin changes, watches on pins with routines that will execute routines to any combo of pins upon pin[s] conditions,
...TODO: alert when back pressure within furnace indicates to change filter
...TODO: damper operation with multiple temp sensors
https://github.com/wemos/Arduino_XI 
*/
//All temps are shorts until displayed
bool mswindows = false;  //Used for line-end on serial outputs.  Make true to print line ends as MS Windows needs  
#include <EEPROM.h>
#ifndef u8
    #define u8 uint8_t
#endif
#ifndef u16
    #define u16 uint16_t
#endif
#include "DHTdirectRead.h"

//Where are other indoor sensors?
//What about utilizing PCI ISRs and how they control which pins to use?

// INTENT: Access these values by reference and pointers.  They will be hardcoded here only.  The run-time code can only find out what they are, how many there are, and their names by calls to this function
/*
Whenever reset, run-time code searches for pin names containing the character strings: "in... temp" ( case-insensitive this pin only) = pin 2, "ux... furn" = pin 6, "urn... start... pri" = pin 3, "urn... start... sec" = pin 4,
   "urn... start... ter" = pin 5, "urn... humi" = pin 7, "umi... start" = pin 8, "cond... start... pri" = pin 9, "cond... start... sec" = pin 10, "cond... start... ter" = pin 11, "ut... temp" = pin 12, "ower cycle" = pin 13

uint8_t i_t_sens_pin_0 = strFactoryDefaults.indexOf( F( "n" ) );
uint9_t i_t_sens_pin_1 = strFactoryDefaults.indexOf( F( "temp" ) );
if( i_t_sens_pin_0 < i_t_sens_pin_1 && i_t_sens_pin_0 )
uint8_t primary_temp_sensor_pin = 
*/
//const PROGMEM char only_options_after_space[] = "That space you entered also then requires a valid mode. The only valid characters allowed after that space are the options lower case ";
//  The following are addresses in EEPROM of values that need to be persistent across power outages.
//The first two address locations in EEPROM store a tatoo used to ensure EEPROM contents are valid for this sketch
u8 primary_temp_sensor_address = 2;
u8 furnace_address = 3;
u8 furnace_fan_pin_address = 4;
u8 power_cycle_address = 5;
u8 thermostat_address = 6;
u8 fan_mode_address = 7;
u8 secondary_temp_sensor_address = 8;

#ifndef __LGT8FX8E__
    u16 EEPROMlength = EEPROM.length();
#else
    #define LED_BUILTIN 12
    u16 EEPROMlength = 1024;
#endif
#ifndef SERIAL_PORT_HARDWARE
    u8 SERIAL_PORT_HARDWARE = 0;
#endif
unsigned int logging_address = EEPROMlength - sizeof( boolean );
unsigned int upper_furnace_temp_address = logging_address - sizeof( short );//EEPROMlength - 2;
unsigned int lower_furnace_temp_address = upper_furnace_temp_address - sizeof( short );//EEPROMlength - 3;
unsigned int logging_temp_changes_address = lower_furnace_temp_address - sizeof( boolean );//EEPROMlength - 4;

// So we don't assume every board has exactly three
//in case of boards that have more than 24 pins ( three virtual 8-bit ports )
//int number_of_ports_that_present_pins = EEPROM.read( EEPROMlength - 5 );  // may never need this

// main temperature sensor, furnace, auxiliary furnace device, system power cycle, porch light, dining room coffee outlet
//  The following values need to be stored persistent through power outages.  Sadly, the __LGT8FX8E__ will not read EEPROM until the setup() loop starts executing, so these values get set there for all boards for simplicity
u8 primary_temp_sensor_pin;// = EEPROM.read( primary_temp_sensor_address ); 
u8 secondary_temp_sensor_pin;// = EEPROM.read( secondary_temp_sensor_address ); 
u8 furnace_pin;// = EEPROM.read( furnace_address );
u8 furnace_fan_pin;// = EEPROM.read( furnace_fan_pin_address );
u8 power_cycle_pin;// = EEPROM.read( power_cycle_address );
boolean logging;// = ( boolean )EEPROM.read( logging_address );
boolean logging_temp_changes;// = ( boolean )EEPROM.read( logging_temp_changes_address );
float lower_furnace_temp_floated;//filled in setup
float upper_furnace_temp_floated;
char thermostat;// = ( char )EEPROM.read( thermostat_address );
char fan_mode;// = ( char )EEPROM.read( fan_mode_address );//a';//Can be either auto (a) or on (o)


//bool mswindows = false;  //Used for line-end on serial outputs.  FUTURE Will be determined true during run time if a 1 Megohm ( value not at all critical as long as it is large enough ohms to not affect operation otherwise )resistor is connected from pin LED_BUILTIN to PIN_A0


/*  The following info plus NUM_DIGITAL_PINS, digitalPinToInterrupt and other digitalPinToxxxx macros, etc. can be used to determine board type at run time:

      Board            int.0   int.1   int.2   int.3   int.4   int.5
      Uno, Ethernet    2        3
            Mega2560   2        3          21    20       19    18
            Leonardo   3        2           0    1
       Due            ( any pin, more info http://arduino.cc/en/Reference/AttachInterrupt )

           Board                     Digital Pins Usable For Interrupts
 Uno, Nano, Mini, other 328-based    2, 3
          Mega, Mega2560, MegaADK    2, 3, 18, 19, 20, 21
Micro, Leonardo, other 32u4-based    0, 1, 2, 3, 7
                             Zero    all digital pins, except 4
                    MKR1000 Rev.1    0, 1, 4, 5, 6, 7, 8, 9, A1, A2
                              Due    all digital pins
                              101    all digital pins
 This example, as opposed to the other, makes use of the new method acquireAndWait()
 Leonardo, Leonardo ETH and Micro allow pins 0 and 1 to be changed by user
 */

//int idDHTLibPin = primary_temp_sensor_pin; //Digital pin for communications
//int idDHTLibIntNumber = digitalPinToInterrupt( idDHTLibPin );

//declaration
//void dhtLib_wrapper(); // must be declared before the lib initialization

// Lib instantiate
//idDHTLib DHTLib( idDHTLibPin, idDHTLibIntNumber, dhtLib_wrapper );

//String str;
String strFull = "";
//String pin_specified_str;
//String temp_specified_str;
float last_three_temps[] = { -100, -101, -102 };
float furnace_started_temp_x_3 = last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ];
u8 pin_specified;
float temp_specified_floated;
boolean fresh_powerup = true;
long unsigned timer_alert_furnace_sent = 0;
const PROGMEM u8 factory_setting_primary_temp_sensor_pin = 2;
const PROGMEM u8 factory_setting_furnace_pin = 3;
const PROGMEM u8 factory_setting_furnace_fan_pin = 4;
const PROGMEM u8 factory_setting_power_cycle_pin = 5;
const PROGMEM bool factory_setting_logging_setting = true;
const PROGMEM bool factory_setting_logging_temp_changes_setting = true;
const PROGMEM char factory_setting_thermostat_mode = 'a';
const PROGMEM char factory_setting_fan_mode = 'a';
const PROGMEM float factory_setting_lower_furnace_temp_floated = 21.3;
const PROGMEM float factory_setting_upper_furnace_temp_floated = 22.4;
const PROGMEM u8 factory_setting_secondary_temp_sensor_pin = 8;
const PROGMEM float minutes_furnace_should_be_effective_after = 5.5; //Can be decimal this way
const PROGMEM unsigned long loop_cycles_to_skip_between_alert_outputs = 5 * 60 * 30;//estimating 5 loops per second, 60 seconds per minute, 30 minutes per alert

boolean IsValidPinNumber( const char* str )
{
//            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//            Serial.print( F( ":" ) );
    u8 i = 0;
    while( str[ i ] == 32 ) i++;
//            Serial.print( str[ i ] );//
//            Serial.print( F( ":" ) );
//            Serial.print( i );//
//           Serial.print( F( ":" ) );
//            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    if( str[ i ] == '.' && str[ i + 1 ] == 0 )
    {
        pin_specified = 0;
        return true;
    }
    u8 j = i;
    while( isdigit( str[ j ] ) ) j++;
//    str[ j ] = 0;//calling code needs to ensure there is room for this!
    if( j == i )
    {
        Serial.print( F( "Command must begin or end with a pin number as specified in help screen" ) );
//        Serial.print( str[ i ] );
//        Serial.print( str[ j ] );
//        Serial.print( ( u8 )str[ j ] );
//        Serial.print( i );
//        Serial.print( j );
//            Serial.print( F( ">" ) );
//            Serial.print( ( u8 )str[i] );
//            Serial.print( str[i] );
//            Serial.print( F( "," ) );
//            Serial.print( i );
            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        return false;
    }
/* */
//    Serial.print( F( "Pin=" ) );
//Serial.print( str[ i ] );
    pin_specified = ( u8 )atoi( str );
//Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//    Serial.print( F( " in char, but " ) );
//    Serial.print( F( "numeric Pin=" ) );
//Serial.print( pin_specified );
//Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    if( pin_specified < 0 || pin_specified >= NUM_DIGITAL_PINS )
    {
        Serial.print( F( "Pin number must be 0 through " ) );
        Serial.print( NUM_DIGITAL_PINS - 1 );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        return false;
    }
    return true;
}

boolean isanoutput( int pin, boolean reply )
{
uint8_t bit = digitalPinToBitMask( pin );
volatile uint8_t *reg = portModeRegister( digitalPinToPort( pin ) );
if( *reg & bit ) return true;
if( reply )
{
    Serial.print( F( "Sorry, but pin " ) );
    Serial.print( pin );
    Serial.print( F( " is not yet set to output" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
}
return false;
}

boolean IsValidTemp( const char* str )
{
//    for( unsigned int i = 0; i < str->length(); i++ )
    for( unsigned int i = 0; i < strlen( str ); i++ )
    {
        if( !( ( isdigit( str[ i ] ) || str[ i ] == '.' ) && !( !i && str[ 0 ] == '-' ) ) || ( strchr( str, '.' ) != strrchr( str, '.' ) ) ) //allow one and only one decimal point in str
        {
            Serial.print( F( "Command must begin with a temperature in Celsius" ) );
            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
            return false;
        }
    }
    temp_specified_floated = ( float )atoi( str );
    if( strchr( str, '.' ) ){ str = strchr( str, '.' ) + 1; temp_specified_floated += ( ( float )atoi( str ) ) / 10; };
//    Serial.print( F( "Temperature entered:" ) );
//    Serial.print( temp_specified_floated );
//    Serial.print( F( ", decimal portion of Temperature entered:" ) );
//    Serial.print( str );
//    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    signed char upper_limit = 27;
    signed char lower_limit = 10;
    if(  temp_specified_floated < lower_limit || temp_specified_floated > upper_limit )//This check should be called for the rooms where it matters
    {
        Serial.print( F( "Temperature must be Celsius " ) );
        Serial.print( lower_limit );
        Serial.print( F( " through " ) );
        Serial.print( upper_limit );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        return false;
    }
    return true;
}

void printBasicInfo()
{
    Serial.print( F( ".." ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    if( fresh_powerup )
    {
       Serial.print( F( "A way to save talkback into a file in Ubuntu and Mint Linux is: (except change \"TIME_STAMP_THIS\" to lower case, not shown)" ) );
       Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
       Serial.print( F( "    nohup stty -F \$(ls /dev/ttyA* /dev/ttyU* 2>/dev/null|tail -n1) " ) );
       Serial.print( _baud_rate_ );
//The following would get time stamped inadvertently:
//       Serial.println( F( " -echo;while true;do cat \$(ls /dev/ttyA* /dev/ttyU* 2>/dev/null|tail -n1)|while IFS= read -r line;do if ! [[ -z \"\$line\" ]];then echo \"\$line\"|sed \'s/\^time_stamp_this/\'\"\$(date )\"\'/g\';fi;done;done >> /log_directory/arduino.log 2>/dev/null &" ) );
//So we have to capitalize time_stamp_this
       Serial.print( F( " -echo;while true;do cat \$(ls /dev/ttyA* /dev/ttyU* 2>/dev/null|tail -n1)|while IFS= read -r line;do if ! [[ -z \"\$line\" ]];then echo \"\$line\"|sed \'s/\^TIME_STAMP_THIS/\'\"\$(date )\"\'/g\';fi;done;done >> /log_directory/arduino.log 2>/dev/null &" ) );
       Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
       Serial.print( F( "." ) );
       Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
       Serial.print( F( "time_stamp_this Newly powered on:" ) );
       Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    }
    Serial.print( F( "Version: " ) );
    Serial.print( F( VERSION ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Operating mode (heating/cooling/auto/off) = " ) );
    Serial.print( thermostat );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Fan mode = " ) );
    Serial.print( fan_mode );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "lower_furnace_temp = " ) );
    Serial.print( lower_furnace_temp_floated, 1 );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "upper_furnace_temp = " ) );
    Serial.print( upper_furnace_temp_floated, 1 );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Talkback for logging is turned o" ) );
    if( logging ) Serial.print( F( "n" ) );
    else  Serial.print( F( "ff" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Talkback for logging temp changes is turned o" ) );
    if( logging_temp_changes ) Serial.print( F( "n" ) );
    else  Serial.print( F( "ff" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "primary_temp_sensor_pin = " ) );
    Serial.print( primary_temp_sensor_pin );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "secondary_temp_sensor_pin = " ) );
    Serial.print( secondary_temp_sensor_pin );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "furnace_pin = " ) );
    Serial.print( furnace_pin );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "furnace_fan_pin = " ) );
    Serial.print( furnace_fan_pin );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "automation system power_cycle_pin = " ) );
    Serial.print( power_cycle_pin );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "LED_BUILTIN pin = " ) );
    Serial.print( LED_BUILTIN );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "." ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Valid commands (cAsE sEnSiTiVe, minimal sanity checking, one per line) are:" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "." ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "help (re-display this information)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "ther[mostat][ a[uto]/ o[ff]/ h[eat]/ c[ool]] (to read or set thermostat)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "fan[ a[uto]/ o[n]] (to read or set fan)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "<pin number> set pin [to] output (or ...pin set)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "<pin number> set pin [to] input [pers] (or ...pin set...) optional persistence" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "<pin number> set pin [to] input with pullup [pers] (or ...pin set...) optional persistence" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "<pin number> read pin (or ...pin read...)(obtain the name if any, setting and voltage)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "read pins (or pins read )(obtain the names, settings and voltages of ALL pins)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "<pin number> set pin [to] low [pers] (or ...pin set...)(only allowed to pins assigned as output) optional persistence" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "<pin number> set pin [to] high [pers] (or ...pin set...)(only allowed to pins assigned as output) optional persistence" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "<°C> set lower furnace temp (to turn furnace on at any lower temperature than this, always persistent)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "<°C> set upper furnace temp (to turn furnace off at any higher temperature than this, always persistent)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "talkback[ on/off] (or logging on/off)" ) );//(for the host system to log when each output pin gets set high or low, always persistent)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "talkback temp change[s[ on/off]] (or logging temp changes on/off)(requires normal talkback on)" ) );// - for the host system to log whenever the main room temperature changes, always persistent)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "report master room temp" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "power cycle (or cycle power)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "ch[ange] pers[istent memory] <address> <value>" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "ch[ange] pers[istent memory] <StartingAddress> \"<character string>[\"[ 0]] (store character string as long as desired, optional null-terminated. Reminder: echo -e and escape the quote[s])" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "vi[ew] pers[istent memory] <StartingAddress>[ <EndingAddress>]" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
#ifndef __LGT8FX8E__
    Serial.print( F( "vi[ew] fact[ory defaults] (so you can see what would happen before you reset to them)" ) );//Wemo XI does not have enough memory for this
#endif
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "reset (factory defaults: pure, simple and absolute)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "test alert (sends an alert message to host for testing purposes)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "dht pin <pin number> (retrieves DHT device reading, a period with due care in place of pin number for all pins)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
/*
#ifdef LED_BUILTIN_NO
    Serial.print( F( "test hidden function <pin number> (for experimental code debugging)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
#endif
*/
    Serial.print( F( ".." ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
}

void restore_factory_defaults()
{
/* no EEPROM updates allowed while in interrupts */
    primary_temp_sensor_pin = factory_setting_primary_temp_sensor_pin;
    furnace_pin = factory_setting_furnace_pin;
    furnace_fan_pin = factory_setting_furnace_fan_pin;
    power_cycle_pin = factory_setting_power_cycle_pin;
    logging = factory_setting_logging_setting;
    logging_temp_changes = factory_setting_logging_temp_changes_setting;
    secondary_temp_sensor_pin = factory_setting_secondary_temp_sensor_pin;
    thermostat = factory_setting_thermostat_mode;
    timer_alert_furnace_sent = 0;
    fan_mode = factory_setting_fan_mode;
    
    lower_furnace_temp_floated = factory_setting_lower_furnace_temp_floated;
    upper_furnace_temp_floated = factory_setting_upper_furnace_temp_floated;
    if( fan_mode == 'o' ) digitalWrite( furnace_fan_pin, HIGH );
    else digitalWrite( furnace_fan_pin, LOW );
    if( thermostat == 'o' ) digitalWrite( furnace_pin, LOW );//Need to do the same for A/C pin here, when known
    Serial.print( F( "Storing thermostat-related.  Primary temperature sensor goes on pin " ) );
    Serial.print( factory_setting_primary_temp_sensor_pin );
    Serial.print( F( ", secondary senosr on pin " ) );
    Serial.print( factory_setting_secondary_temp_sensor_pin );
    Serial.print( F( ", furnace controlled by pin " ) );
    Serial.print( factory_setting_furnace_pin );
    Serial.print( F( ", aux furnace fan or whatnot controlled by pin " ) );
    Serial.print( factory_setting_furnace_fan_pin );
    Serial.print( F( ", power cycle of automation system or whatnot controlled by pin " ) );
    Serial.print( factory_setting_power_cycle_pin );
    Serial.print( F( ", logging o" ) );
    if( factory_setting_logging_setting ) Serial.print( F( "n" ) );
    else Serial.print( F( "ff" ) );
    Serial.print( F( ", logging temp changes o" ) );
    if( factory_setting_logging_temp_changes_setting ) Serial.print( F( "n" ) );
    else Serial.print( F( "ff" ) );
    Serial.print( F( ", lower furnace temp=" ) );
    Serial.print( factory_setting_upper_furnace_temp_floated, 1 );
    Serial.print( F( ", upper furnace temp=" ) );
    Serial.print( factory_setting_lower_furnace_temp_floated, 1 );
    Serial.print( F( ", furnace mode=" ) );
    if( factory_setting_thermostat_mode == 'a' )
        Serial.print( F( "auto" ) );
    else if( factory_setting_thermostat_mode == 'o' )
        Serial.print( F( "off" ) );
    else if( factory_setting_thermostat_mode == 'h' )
        Serial.print( F( "heat" ) );
    else if( factory_setting_thermostat_mode == 'c' )
        Serial.print( F( "cool" ) );

    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
#ifndef __LGT8FX8E__
    EEPROM.update( primary_temp_sensor_address, factory_setting_primary_temp_sensor_pin );//2 );
#else
    EEPROM.write( primary_temp_sensor_address, factory_setting_primary_temp_sensor_pin );//2 );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( furnace_address, factory_setting_furnace_pin );//3 );
#else
    EEPROM.write( furnace_address, factory_setting_furnace_pin );//3 );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( furnace_fan_pin_address, factory_setting_furnace_fan_pin );//4 );
#else
    EEPROM.write( furnace_fan_pin_address, factory_setting_furnace_fan_pin );//4 );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( power_cycle_address, factory_setting_power_cycle_pin );//5);
#else
    EEPROM.write( power_cycle_address, factory_setting_power_cycle_pin );//5);
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( logging_address, factory_setting_logging_setting );//( uint8_t )true );
#else
    EEPROM.write( logging_address, factory_setting_logging_setting );//( uint8_t )true );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( logging_temp_changes_address, factory_setting_logging_temp_changes_setting );//( uint8_t )true );
#else
    EEPROM.write( logging_temp_changes_address, factory_setting_logging_temp_changes_setting );//( uint8_t )true );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( thermostat_address, factory_setting_thermostat_mode );//'a' );
#else
    EEPROM.write( thermostat_address, factory_setting_thermostat_mode );//'a' );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( fan_mode_address, factory_setting_fan_mode );//'a' );
#else
    EEPROM.write( fan_mode_address, factory_setting_fan_mode );//'a' );
#endif
//These two must be put's because their data types are longer than one.  Data types of length one can be either updates or puts.
    short factory_setting_lower_furnace_temp_shorted_times_ten = ( short )( factory_setting_lower_furnace_temp_floated * 10 );
    short factory_setting_upper_furnace_temp_shorted_times_ten = ( short )( factory_setting_upper_furnace_temp_floated * 10 );
#ifndef __LGT8FX8E__
    EEPROM.put( lower_furnace_temp_address, factory_setting_lower_furnace_temp_shorted_times_ten );//21 );
#else
    EEPROM.write( lower_furnace_temp_address, ( u8 )factory_setting_lower_furnace_temp_shorted_times_ten );
    EEPROM.write( lower_furnace_temp_address + 1, ( u8 )( factory_setting_lower_furnace_temp_shorted_times_ten >> 8 ) );
#endif
#ifndef __LGT8FX8E__
    EEPROM.put( upper_furnace_temp_address, factory_setting_upper_furnace_temp_shorted_times_ten );//21 );
#else
    EEPROM.write( upper_furnace_temp_address, ( u8 )factory_setting_upper_furnace_temp_shorted_times_ten );
    EEPROM.write( upper_furnace_temp_address + 1, ( u8 )( factory_setting_upper_furnace_temp_shorted_times_ten >> 8 ) );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( secondary_temp_sensor_address, factory_setting_secondary_temp_sensor_pin );//2 );
#else
    EEPROM.write( secondary_temp_sensor_address, factory_setting_secondary_temp_sensor_pin );//2 );
#endif
#ifndef __LGT8FX8E__
    EEPROM.put( 0, ( NUM_DIGITAL_PINS + 1 ) * 3 );//Tattoo the board
#else
    EEPROM.write( 0, ( u8 )( ( NUM_DIGITAL_PINS + 1 ) * 3 ) );
    EEPROM.write( 1, ( u8 )( ( ( NUM_DIGITAL_PINS + 1 ) * 3 ) >> 8 ) );
#endif
//    EEPROM.update( 1, ( u8 )( ( ( NUM_DIGITAL_PINS + 1 ) * 3 ) >> 8 ) );//Tattoo the board
//    EEPROM.update( EEPROMlength - 4, 114 );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Factory initialization complete.  Now allowing you 10-13 seconds to unplug the Arduino in case this environment is for programming only." ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    //EEPROM.update( EEPROMlength - 5, 3 );//not used for now, forgot what the 3 means.  Mode or ID of some sort?
//Location 1 should contain  MSB of the EEPROM address that contains the value of ( NUM_DIGITAL_PINS + 1 ) * 3 )
    
    //MAKE THE FOLLOWING INTO A STANDALONE ROUTINE INSTEAD OF EXPLICIT HERE because they can also be needed if end user wants to reset to factory defaults
    delay( 10000 );
      printBasicInfo();
/* */
}

void print_factory_defaults()
{
/*
factory_setting_primary_temp_sensor_pin;
factory_setting_furnace_pin;
factory_setting_furnace_fan_pin;
factory_setting_power_cycle_pin;
factory_setting_logging_setting;
factory_setting_thermostat_mode;
factory_setting_lower_furnace_temp;
factory_setting_upper_furnace_temp;
*/
    Serial.print( F( "Version: " ) );
    Serial.print( F( VERSION ) );
    Serial.print( F( " Factory defaults:" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Operating mode (heating/cooling/auto/off) = " ) );
    if( factory_setting_thermostat_mode == 'a' )
        Serial.print( F( "auto" ) );
    else if( factory_setting_thermostat_mode == 'o' )
        Serial.print( F( "off" ) );
    else if( factory_setting_thermostat_mode == 'h' )
        Serial.print( F( "heat" ) );
    else if( factory_setting_thermostat_mode == 'c' )
        Serial.print( F( "cool" ) );
//    Serial.print( factory_setting_thermostat_mode );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Fan mode = " ) );
    Serial.print( factory_setting_fan_mode );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "lower_furnace_temp = " ) );
    Serial.print( factory_setting_lower_furnace_temp_floated, 1 );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "upper_furnace_temp = " ) );
    Serial.print( factory_setting_upper_furnace_temp_floated, 1 );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Talkback for logging o" ) );
    if( factory_setting_logging_setting ) Serial.print( F( "n" ) );
    else   Serial.print( F( "ff" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Talkback for logging temp changes is turned o" ) );
    if( factory_setting_logging_temp_changes_setting ) Serial.print( F( "n" ) );
    else  Serial.print( F( "ff" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "primary_temp_sensor_pin = " ) );
    Serial.print( factory_setting_primary_temp_sensor_pin );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "secondary_temp_sensor_pin = " ) );
    Serial.print( factory_setting_secondary_temp_sensor_pin );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "furnace_pin = " ) );
    Serial.print( factory_setting_furnace_pin );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "furnace_fan_pin = " ) );
    Serial.print( factory_setting_furnace_fan_pin );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "automation system power_cycle_pin = " ) );
    Serial.print( factory_setting_power_cycle_pin );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "LED_BUILTIN pin = " ) );
    Serial.print( LED_BUILTIN );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
}

/*
char* factory_default( name )
{

}
*/

void setup()
{
    Serial.begin( _baud_rate_ );
    Serial.setTimeout( 10 );
    //read EEPROM addresses 0 (LSB)and 1 (MSB).  MSB combined with LSB should always contain ( NUM_DIGITAL_PINS + 1 ) * 3.
    u16 tattoo = 0;
#ifndef __LGT8FX8E__
    EEPROM.get( 0, tattoo );
#else
    tattoo = EEPROM.read( 0 );
    tattoo += ( u16 )( EEPROM.read( 1 ) << 8 );
    delay( 10000 );//Needed for this board for Serial communications to be reliable
#endif
    //tattoo |= EEPROM.read( 0 ); //Location 0 should contain 
    if( tattoo != ( NUM_DIGITAL_PINS + 1 ) * 3 ) // Check for tattoo
    {
        while ( !Serial ); // wait for serial port to connect. Needed for Leonardo's native USB
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        Serial.print( F( "Detected first time run so initializing to factory default pin names, power-up states and thermostat assignments.  Please wait..." ) );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    
       restore_factory_defaults();
    }
    primary_temp_sensor_pin = EEPROM.read( primary_temp_sensor_address ); 
    secondary_temp_sensor_pin = EEPROM.read( secondary_temp_sensor_address ); 
    furnace_pin = EEPROM.read( furnace_address );
    furnace_fan_pin = EEPROM.read( furnace_fan_pin_address );
    power_cycle_pin = EEPROM.read( power_cycle_address );
    logging = ( boolean )EEPROM.read( logging_address );
    logging_temp_changes = ( boolean )EEPROM.read( logging_temp_changes_address );
    thermostat = ( char )EEPROM.read( thermostat_address );
    fan_mode = ( char )EEPROM.read( fan_mode_address );//a';//Can be either auto (a) or on (o)
    pinMode( power_cycle_pin, OUTPUT );
    digitalWrite( power_cycle_pin, LOW );
    pinMode( furnace_pin, OUTPUT );
    digitalWrite( furnace_pin, LOW );
    pinMode( furnace_fan_pin, OUTPUT );
    if( fan_mode == 'o' ) digitalWrite( furnace_fan_pin, HIGH );
    else digitalWrite( furnace_fan_pin, LOW );
    short lower_furnace_temp_shorted_times_ten = 0;
#ifndef __LGT8FX8E__
    EEPROM.get( lower_furnace_temp_address, lower_furnace_temp_shorted_times_ten );
#else
    lower_furnace_temp_shorted_times_ten = EEPROM.read( lower_furnace_temp_address );
    lower_furnace_temp_shorted_times_ten += ( u16 )( EEPROM.read( lower_furnace_temp_address + 1 ) << 8 );
#endif
    lower_furnace_temp_floated = ( float )( ( float )( lower_furnace_temp_shorted_times_ten ) / 10 );
    short upper_furnace_temp_shorted_times_ten = 0;
#ifndef __LGT8FX8E__
    EEPROM.get( upper_furnace_temp_address, upper_furnace_temp_shorted_times_ten );
#else
    upper_furnace_temp_shorted_times_ten = EEPROM.read( upper_furnace_temp_address );
    upper_furnace_temp_shorted_times_ten += ( u16 )( EEPROM.read( upper_furnace_temp_address + 1 ) << 8 );
    logging = ( boolean )EEPROM.read( logging_address );
#endif
    upper_furnace_temp_floated = ( float )( ( float )( upper_furnace_temp_shorted_times_ten ) / 10 );
      delay( 3000 ); // The sensor needs time to initialize, if you have some code before this that make a delay, you can eliminate this delay
      //( pin_specified*3 )and ( pin_specified*3 )+1 contains the EEPROM address where the pin's assigned name is stored.  Pin 0 will always have its name stored at EEPROM address (NUM_DIGITAL_PINS+1 )*3, so address (NUM_DIGITAL_PINS+1 )*3 will always be stored in EEPROM addresses 0 and 1; 0 = ( pin_number*3 )and 1 = (( pin_number*3 )+1 ) )]
      // That will be the way we determine if the EEPROM is configured already or not
      // ( pin_specified*3 )+2 is EEPROM address where the pin's desired inital state is stored
}

void check_for_serial_input( char result )
{
    if( Serial.available() > 0 )
    {
//  digitalWrite(LED_BUILTIN, HIGH );
//        str = Serial.readStringUntil( '\n' );
//        strFull = strFull + str;
        strFull.concat( Serial.readStringUntil( '\n' ) );
//        strFull.replace( F( "talkback" ), F( "logging" ) );
        char *hit;
        hit = strstr( strFull.c_str(), "talkback" );
        if( hit )
        {
            strcpy( hit, "logging" ); 
            memmove( hit + 7, hit + 8, sizeof( hit + 8 ) );
        }
//        strFull.replace( F( "pin set" ), F( "set pin" ) );
        hit = strstr( strFull.c_str(), "pin set" );
        if( hit )
        {
            strncpy( hit, "set pin", 7 ); 
        }  
//        strFull.replace( F( "set pin to" ), F( "set pin" ) );
        hit = strstr( strFull.c_str(), "set pin to" );
        if( hit )
        {
            memmove( hit + 7, hit + 10, strlen( hit + 10 ) );
//            hit[ strstr( strFull.c_str(), "put" ] = 0x0;
        }
//        strFull.replace( F( "view" ), F( "vi" ) );
        hit = strstr( strFull.c_str(), "view" );
        if( hit )
        {
            memmove( hit + 2 , hit + 4, sizeof( hit + 4 ) );
        }
//        strFull.replace( F( "persistent memory" ), F( "pers" ) );
        hit = strstr( strFull.c_str(), "persistent memory" );
        if( hit )
        {
            memmove( hit + 4 , hit + 17, sizeof( hit + 17 ) );
        }
//        strFull.replace( F( "changes" ), F( "ch" ) );
        hit = strstr( strFull.c_str(), "changes" );
        if( hit )
        {
            memmove( hit + 2, hit + 7, sizeof( hit + 7 ) );
        }
//        strFull.replace( F( "change" ), F( "ch" ) );
        hit = strstr( strFull.c_str(), "change" );
        if( hit )
        {
            memmove( hit + 2, hit + 6, sizeof( hit + 6 ) );
        }
//        strFull.replace( F( "thermostat" ), F( "ther" ) );
        hit = strstr( strFull.c_str(), "thermostat" );
        if( hit )
        {
            memmove( hit + 4, hit + 10, sizeof( hit + 10 ) );
        }
//        strFull.replace( F( "fan auto" ), F( "fan a" ) );
        hit = strstr( strFull.c_str(), "fan auto" );
        if( hit )
        {
            memmove( hit + 5, hit + 8, sizeof( hit + 8 ) );
        }
//        strFull.replace( F( "fan on" ), F( "fan o" ) );
        hit = strstr( strFull.c_str(), "fan on" );
        if( hit )
        {
            memmove( hit + 5, hit + 6, sizeof( hit + 6 ) );
        }
//        strFull.replace( F( "factory" ), F( "fact" ) );
        hit = strstr( strFull.c_str(), "factory" );
        if( hit )
        {
            memmove( hit + 4, hit + 7, sizeof( hit + 7 ) );
        }
        char *number_specified_str_end;
        char *address_str;
        char *data_str;
//        if( strFull.indexOf( F( "power cycle" ) ) >= 0 || strFull.indexOf( F( "cycle power" ) ) >= 0 )
        if( strstr( strFull.c_str(), "power cycle" ) || strstr( strFull.c_str(), "power cycle" ) )
        {
//          Serial.print( digitalRead( power_cycle_pin ) );
          digitalWrite( power_cycle_pin, HIGH );
//          Serial.print( digitalRead( power_cycle_pin ) );
          if( logging )
          {
               Serial.print( F( "time_stamp_this powered off, power control pin " ) );
               Serial.print( power_cycle_pin );
               Serial.print( F( " = " ) );
               Serial.print( digitalRead( power_cycle_pin ) );
          }
          delay( 1500 );
          digitalWrite( power_cycle_pin, LOW );            
          if( logging )
          {
              Serial.print( F( ", powered back on, power control pin " ) );
              Serial.print( power_cycle_pin );
              Serial.print( F( " = " ) );
              Serial.print( digitalRead( power_cycle_pin ) );
              Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
          }
          strFull = "";
        }
        else if( strstr( strFull.c_str(), "set lower furnace temp" ) )
        {
           number_specified_str_end = strchr( strFull.c_str(), ' ' );
           *number_specified_str_end = 0;
           if( IsValidTemp( strFull.c_str() ) )
           {
              if( temp_specified_floated <= upper_furnace_temp_floated )
              {
                lower_furnace_temp_floated = temp_specified_floated;
                timer_alert_furnace_sent = 0;
                short temp_specified_shorted_times_ten = ( short )( temp_specified_floated * 10 );
#ifndef __LGT8FX8E__
                EEPROM.put( lower_furnace_temp_address, temp_specified_shorted_times_ten );
#else
                EEPROM.write( lower_furnace_temp_address, ( u8 )temp_specified_shorted_times_ten );
                EEPROM.write( lower_furnace_temp_address + 1, ( u8 )( temp_specified_shorted_times_ten >> 8 ) );
#endif
              }
              else 
              {
                Serial.print( F( "Upper furnace temp too low for this value. Raise that setting before retrying this value" ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
              }
            }
            strFull = "";
        }
        else if( strstr( strFull.c_str(), "set upper furnace temp" ) )
        {
           number_specified_str_end = strchr( strFull.c_str(), ' ' );
           *number_specified_str_end = 0;
           if( IsValidTemp( strFull.c_str() ) )
           {
              if( temp_specified_floated >= lower_furnace_temp_floated )
              {
                upper_furnace_temp_floated = temp_specified_floated;
                timer_alert_furnace_sent = 0;
                short temp_specified_shorted_times_ten = ( short )( temp_specified_floated * 10 );
#ifndef __LGT8FX8E__
                EEPROM.put( upper_furnace_temp_address, temp_specified_shorted_times_ten );
#else
                EEPROM.write( upper_furnace_temp_address, ( u8 )temp_specified_shorted_times_ten );
                EEPROM.write( upper_furnace_temp_address + 1, ( u8 )( temp_specified_shorted_times_ten >> 8 ) );
#endif
              }
              else 
              {
                Serial.print( F( "Lower furnace temp too high for this value. Lower that setting before retrying this value" ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
              }
           }
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "report master room temp" ) )
        {
           if( result == DEVICE_READ_SUCCESS )
           {
               Serial.print( F( "Humidity (%): " ) );
               Serial.print( _HumidityPercent, 1 );
               Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
               Serial.print( F( "Temperature (°C): " ) );
               Serial.print( _TemperatureCelsius, 1 );
               Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );

               
               Serial.print( F( "Temperature furnace is set to start: " ) );
               Serial.print( lower_furnace_temp_floated, 1 );
               Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
               Serial.print( F( "Temperature furnace is set to stop: " ) );
               Serial.print( upper_furnace_temp_floated, 1 );
               Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
               Serial.print( F( "Furnace output: " ) );
               Serial.print( digitalRead( furnace_pin ) );
               Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
               Serial.print( F( "Furnace fan: " ) );
               Serial.print( digitalRead( furnace_fan_pin ) );
           }
           else
           {
                Serial.print( F( "Temperature sensor didn't read" ) );
           }
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
            strFull = "";
        }
        else if( strstr( strFull.c_str(), "read pins" ) || strstr( strFull.c_str(), "pins read" ) )
        {
//          int desired_pin = 0;
          for ( pin_specified = 0; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
          {
              Serial.print( F( "Pin " ) );
              Serial.print( pin_specified );
              if( isanoutput( pin_specified, false ) )
              {
                 Serial.print( F( " is an output having a digital " ) );
              }
              else
              {
                 Serial.print( F( " is an input that reads a digital " ) );
              }
              Serial.print( digitalRead( pin_specified ) );
/*
              Serial.print( F( ", on port " ) );
              uint8_t port = digitalPinToPort( pin_specified );
              char * port_str = "PORTD";
              if( port == 1 )
              {
                port_str = "PORTA";
              }
              else if( port == 2 )
              {
                port_str = "PORTB";
              }
              else if( port == 3 )
              {
                port_str = "PORTC";
              }
              Serial.print( port_str );
              Serial.print( F( ", with bitmask " ) );
              uint8_t bmask = digitalPinToBitMask( pin_specified );
              for ( int i=0;i<8;i++)
              {
                  Serial.print( bitRead(bmask, 7-i ) );
              }
*/
              Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
          }
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "read pin" ) || strstr( strFull.c_str(), "pin read" ) )
        {
           number_specified_str_end = strchr( strFull.c_str(), ' ' );
           *number_specified_str_end = 0;
           if( IsValidPinNumber( strFull.c_str() ) )
           {
              if( pin_specified == primary_temp_sensor_pin )
              {
                Serial.print( F( " connected to temperature sensor ( pin " ) );
              }
              else
              {
                 Serial.print( F( "Pin " ) );
                 Serial.print( pin_specified );
                 Serial.print( F( " (name functionality coming in a future sketch version)" ) );
/*
                 char data = EEPROM.read( pin_specified*3 );
                 char data2 = EEPROM.read(( pin_specified*3 )+1 );
                 if( data < EEPROMlength - 1 )
                 {
                     Serial.print( F( " is named " ) );
                     Serial.print();
                 }
*/
                 if( isanoutput( pin_specified, false ) )
                 {
                     Serial.print( F( " is an output having a digital " ) );
                 }
                 else
                 {
                     Serial.print( F( " is an input that reads a digital " ) );
                 }
              }
              Serial.print( digitalRead( pin_specified ) );
              Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );

           }
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "set pin high" ) )
        {
           number_specified_str_end = strchr( strFull.c_str(), ' ' );
           *number_specified_str_end = 0;
           if( IsValidPinNumber( strFull.c_str() ) )
           {
              if( isanoutput( pin_specified, true ) )
              {
                 digitalWrite( pin_specified, HIGH );
                 if( logging )
                 {
                    if( pin_specified == primary_temp_sensor_pin )
                    {
                      Serial.print( F( "time_stamp_this Temperature sensor connection, pin " ) );
                    }
                    else
                    {
                       Serial.print( F( "time_stamp_this Pin " ) );
                    }
                    Serial.print( pin_specified );
                    Serial.print( F( " set to high level" ) );
                    int pinState = digitalRead( pin_specified );
                    if( pinState == LOW ) Serial.print( F( " but is reading back a 0 because something was/is forcing that line low. DANGER WILL ROBINSON! That pin voltage isn't right" ) );
                    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                 }
              }
              else
              {
                 if( logging )
                 {
                    Serial.print( F( "time_stamp_this Sorry, unable to control that pin because it hasn't been made into an output, yet" ) );
                    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                 }
              }
           }
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "set pin low" ) )
        {
           number_specified_str_end = strchr( strFull.c_str(), ' ' );
           *number_specified_str_end = 0;
           if( IsValidPinNumber( strFull.c_str() ) )
           {
              if( isanoutput( pin_specified, true ) )
              {
                 digitalWrite( pin_specified, LOW );
                 if( logging )
                 {
                    if( pin_specified == primary_temp_sensor_pin )
                    {
                      Serial.print( F( "time_stamp_this Temperature sensor connection, pin " ) );
                    }
                    else
                    {
                       Serial.print( F( "time_stamp_this Pin " ) );
                    }
                    Serial.print( pin_specified );
                    Serial.print( F( " set to low level" ) );
                    int pinState = digitalRead( pin_specified );
                    if( pinState == HIGH ) Serial.print( F( ". DANGER WILL ROBINSON! That pin voltage isn't right.  Something off the board was/is forcing that line high and conflicting with the low voltage that the board is trying to put on it" ) );
                    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                 }
              }
              else
              {
                 if( logging )
                 {
                    Serial.print( F( "time_stamp_this Sorry, unable to control that pin because it hasn't been made into an output, yet" ) );
                    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                 }
              }
           }
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "set pin output" ) )
        {
           number_specified_str_end = strchr( strFull.c_str(), ' ' );
           *number_specified_str_end = 0;
           if( IsValidPinNumber( strFull.c_str() ) && pin_specified != SERIAL_PORT_HARDWARE )
           {
              pinMode( pin_specified, OUTPUT );
              if( logging )
              {
                    if( pin_specified == primary_temp_sensor_pin )
                    {
                      Serial.print( F( "time_stamp_this Temperature sensor connection, pin " ) );
                    }
                    else
                    {
                       Serial.print( F( "time_stamp_this Pin " ) );
                    }
                 Serial.print( pin_specified );
                 Serial.print( F( " now set to output and has a digital " ) );
                 Serial.print( digitalRead( pin_specified ) );
                 Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
              }
           }
           else
           {
illegal_attempt_SERIAL_PORT_HARDWARE:; 
              Serial.print( F( "time_stamp_this Sorry, but pin " ) );
              Serial.print( SERIAL_PORT_HARDWARE );
              Serial.print( F( " is dedicated to receive communications from the host" ) );
              Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           }
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "set pin input with pullup" ) )
        {
           number_specified_str_end = strchr( strFull.c_str(), ' ' );
           *number_specified_str_end = 0;
           if( IsValidPinNumber( strFull.c_str() ) && pin_specified != SERIAL_PORT_HARDWARE )
           {
              if( pin_specified == power_cycle_pin || pin_specified == furnace_fan_pin || pin_specified == furnace_pin )
              {
                 if( logging )
                 {
                   Serial.print( F( "Sorry, unable to allow that pin to be made an input. It is dedicated as output only for " ) );
                   if( pin_specified == power_cycle_pin ) Serial.print( F( "cycling the power to the host system." ) );
                   if( pin_specified == furnace_fan_pin ) Serial.print( F( "the furnace blower fan." ) );
                   if( pin_specified == furnace_pin ) Serial.print( F( "the furnace." ) );
                   Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                 }
              }
              else
              {
                 pinMode( pin_specified, INPUT_PULLUP );
                 if( logging )
                 {
                    if( pin_specified == primary_temp_sensor_pin )
                    {
                      Serial.print( F( "time_stamp_this Temperature sensor connection, pin " ) );
                    }
                    else
                    {
                       Serial.print( F( "time_stamp_this Pin " ) );
                    }
                    Serial.print( pin_specified );
                    Serial.print( F( " now set to input with pullup" ) );
                    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                 }
              }
           }
           else goto illegal_attempt_SERIAL_PORT_HARDWARE;
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "set pin input" ) )
        {
           number_specified_str_end = strchr( strFull.c_str(), ' ' );
           *number_specified_str_end = 0;
           if( IsValidPinNumber( strFull.c_str() ) && pin_specified != SERIAL_PORT_HARDWARE )
           {
              if( pin_specified == power_cycle_pin || pin_specified == furnace_fan_pin || pin_specified == furnace_pin )
              {
                Serial.print( F( "Sorry, unable to allow that pin to be made an input. You've dedicated output only for " ) );
//                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                if( pin_specified == power_cycle_pin ) Serial.print( F( "cycling the power to the host system." ) );
                else if( pin_specified == furnace_fan_pin ) Serial.print( F( "the furnace blower fan." ) );
                else if( pin_specified == furnace_pin ) Serial.print( F( "the furnace." ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
              }
              else
              {
                 pinMode( pin_specified, OUTPUT );
                 digitalWrite( pin_specified, LOW );
                 int pinState = Serial.print( digitalRead( pin_specified ) );
                 Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                 pinMode( pin_specified, INPUT );
                 if( logging )
                 {
                    if( pin_specified == primary_temp_sensor_pin )
                    {
                      Serial.print( F( "time_stamp_this Temperature sensor connection, pin " ) );
                    }
                    else
                    {
                       Serial.print( F( "time_stamp_this Pin " ) );
                    }
                    Serial.print( pin_specified );
                    Serial.print( F( " now set to input" ) );
                    if( pinState == HIGH ) Serial.print( F( " with pullup because something was/is forcing that line high. DANGER WILL ROBINSON! That pin voltage isn't right" ) );
                    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                 }
              }
           }
           else goto illegal_attempt_SERIAL_PORT_HARDWARE;
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "ther a" ) || strstr( strFull.c_str(), "ther o" ) || strstr( strFull.c_str(), "ther h" ) || strstr( strFull.c_str(), "ther c" ) )
        {
           Serial.print( F( "Thermostat being set to " ) );
//           int charat = strFull.indexOf( F( "ther " ) ) + 5;
           int charat = strstr( strFull.c_str(), "ther " ) - strFull.c_str() + 5;
           if( strFull.charAt( charat ) == 'a' ) Serial.print( F( "auto" ) );
           else if( strFull.charAt( charat ) == 'o' ) 
           {
                Serial.print( F( "off" ) );
                digitalWrite( furnace_pin, LOW );//Need to do the same for A/C pin here, when known
           }
           else if( strFull.charAt( charat ) == 'h' ) Serial.print( F( "heat" ) );
           else if( strFull.charAt( charat ) == 'c' ) Serial.print( F( "cool" ) );
           else { Serial.print( F( "<fault>. No change made" ) ); goto after_change_thermostat; } //Making extra sure that no invalid mode gets saved
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//change_thermostat:
           thermostat = strFull.charAt( charat );
            timer_alert_furnace_sent = 0;           
#ifndef __LGT8FX8E__
           EEPROM.update( thermostat_address, strFull.charAt( charat ) );
#else
           EEPROM.write( thermostat_address, strFull.charAt( charat ) );
#endif
after_change_thermostat:
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "ther" ) )
        {
//           int charat = strstr( strFull.c_str(), "logging temp ch o" ) ) - strFull.c_str() + 17;
            if( strchr( &strFull.c_str()[ 4 ], ' ' ) )
//            if( strFull.indexOf( F( " " ) ) >= 4 )
            {
//                Serial.print( strchr( &strFull.c_str()[ 4 ], ' ' ) );
//                Serial.print( F( "<" ) );
//                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//                Serial.print( only_options_after_space );
                Serial.print( F( "That space you entered also then requires a valid mode. The only valid characters allowed after that space are the options lower case a, o, h, or c.  They mean auto, off, heat, and cool and optionally may be spelled out completely" ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
            }
            Serial.print( F( "Thermostat is " ) );
            if( thermostat == 'a' ) Serial.print( F( "auto" ) );
            else if( thermostat == 'o' ) Serial.print( F( "off" ) );
            else if( thermostat == 'h' ) Serial.print( F( "heat" ) );
            else if( thermostat == 'c' ) Serial.print( F( "cool" ) );
            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "fan a" ) || strstr( strFull.c_str(), "fan o" ) )
        {
           Serial.print( F( "Fan being set to " ) );
           int charat = charat = strstr( strFull.c_str(), "fan " ) - strFull.c_str() + 4;
           if( strFull.charAt( charat ) == 'a' ) Serial.print( F( "auto" ) );
           else if( strFull.charAt( charat ) == 'o' ) Serial.print( F( "on" ) );
           else { Serial.print( F( "<fault>. No change made" ) ); goto after_change_fan; } //Making extra sure that no invalid mode gets saved
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//change_fan:
           fan_mode = strFull.charAt( charat );
           if( fan_mode == 'o' ) digitalWrite( furnace_fan_pin, HIGH );
           else digitalWrite( furnace_fan_pin, LOW );
#ifndef __LGT8FX8E__
           EEPROM.update( fan_mode_address, strFull.charAt( charat ) );
#else
           EEPROM.write( fan_mode_address, strFull.charAt( charat ) );
#endif
after_change_fan:
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "fan" ) )
        {
//strchr( &strFull.c_str()[ 7 ], ' ' ) + 1
//          address_str = strchr( &strFull.c_str()[ 7 ], ' ' ) + 1;//.substring( address_str.indexOf( ' ' )+ 1 );
            if( strchr( &strFull.c_str()[ 3 ], ' ' ) )
//            if( strFull.indexOf( F( " " ) ) >= 3 )
            {
//                Serial.print( only_options_after_space );
                Serial.print( F( "That space you entered also then requires a valid mode. The only valid characters allowed after that space are the options lower case a or o.  They mean auto and on and optionally may be spelled out completely" ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
            }
            Serial.print( F( "Fan is " ) );
            if( fan_mode == 'a' ) Serial.print( F( "auto" ) );
            else if( fan_mode == 'o' ) Serial.print( F( "on" ) );
            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "logging temp ch o" ) )
        {
           Serial.print( F( "Talkback for logging temp changes being turned o" ) );
//           int charat = strFull.indexOf( F( "logging temp ch o" ) )+ 17;
           int charat = strstr( strFull.c_str(), "logging temp ch o" ) - strFull.c_str() + 17;
           if( strFull.c_str()[ charat ] == 'n' ) logging_temp_changes = true;
           else logging_temp_changes = false;
#ifndef __LGT8FX8E__
           EEPROM.update( logging_temp_changes_address, ( uint8_t )logging_temp_changes );
#else
           EEPROM.write( logging_temp_changes_address, ( uint8_t )logging_temp_changes );
#endif
           if( logging_temp_changes ) Serial.print( strFull.charAt( charat ) );
           else  Serial.print( F( "ff" ) );
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "logging temp ch" ) )
        {
           Serial.print( F( "Talkback for logging temp changes is turned o" ) );
           if( logging_temp_changes ) Serial.print( F( "n" ) );
           else  Serial.print( F( "ff" ) );
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "logging o" ) )
        {
           Serial.print( F( "Talkback for logging being turned o" ) );
//           int charat = strFull.indexOf( F( "logging o" ) )+ 9;
           int charat = strstr( strFull.c_str(), "logging o" ) - strFull.c_str() + 9;           
           if( strFull.c_str()[ charat ] == 'n' ) logging = true;
           else logging = false;
#ifndef __LGT8FX8E__
           EEPROM.update( logging_address, ( uint8_t )logging );
#else
           EEPROM.write( logging_address, ( uint8_t )logging );
#endif
           if( logging ) Serial.print( strFull.charAt( charat ) );
           else  Serial.print( F( "ff" ) );
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "logging" ) )
        {
           Serial.print( F( "Talkback for logging is turned o" ) );
           if( logging ) Serial.print( F( "n" ) );
           else  Serial.print( F( "ff" ) );
//           Serial.print( strFull );
           Serial.print( F( " pers address " ) );
           Serial.print( logging_address );
           Serial.print( F( " shows " ) );
           Serial.print( ( bool )EEPROM.read( logging_address ) );
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "vi pers" ) )
        {
//          address_str = strFull.substring( 7, strFull.length() );
//          address_str = address_str.substring( address_str.indexOf( ' ' )+ 1 );
          address_str = strchr( &strFull.c_str()[ 7 ], ' ' ) + 1;//.substring( address_str.indexOf( ' ' )+ 1 );
          unsigned int address_start = atoi( address_str );
          address_str = strrchr( address_str, ' ' ) + 1;//address_str.substring( address_str.indexOf( ' ' )+ 1 );
          unsigned int address_end = atoi( address_str );//.toInt();
          if( address_start < 0 || address_start >= EEPROMlength || address_end < 0 || address_end >= EEPROMlength )
          {
            Serial.print( F( "Out of range.  Each address can only be 0 to " ) );
            Serial.print( EEPROMlength - 1 );
            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
          }
          else
          {
             Serial.print( F( "Start address: " ) );
             Serial.print( address_start );
             Serial.print( F( " End address: " ) );
             Serial.print( address_end );
             Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//             signed int drection = 1;
             if( address_start > address_end )
             {
//                drection = -1;
                // swap the two addresses:
                address_start += address_end;
                address_end = address_start - address_end; //makes address_end the original address_start
                address_start = address_start - address_end;
             }
             for ( unsigned int address = address_start; address <= address_end; address++)
             {
               Serial.print( F( "Address: " ) );
               if( address < 1000 ) Serial.print( F( " " ) );
               if( address < 100 ) Serial.print( F( " " ) );
               if( address < 10 ) Serial.print( F( " " ) );
               Serial.print( address );
               Serial.print( F( " Data: " ) );
               unsigned int data = EEPROM.read( address );
               if( data < 100 ) Serial.print( F( " " ) );
               if( data < 10 ) Serial.print( F( " " ) );
               Serial.print( data );
               Serial.print( F( "  " ) );
               Serial.print( ( char )EEPROM.read( address ) );
               Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
             }
          }
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "ch pers" ) )//NEEDS TO BE VERIFIED SINCE LAST UPDATE THAT REMOVED STRING OBJECTS
        {
//          address_str = &strFull.c_str()[ 7 ];//substring( 7, strFull.length() );
          address_str = strchr( &strFull.c_str()[ 7 ], ' ' ) + 1;//.substring( address_str.indexOf( ' ' )+ 1 );
          unsigned int address = atoi( address_str );//.toInt();
          data_str = strrchr( address_str, ' ' ) + 1;//address_str.substring( address_str.indexOf( ' ' )+ 1 );
              Serial.print( F( " entered data: <" ) );
              Serial.print( data_str );
              Serial.print( F( ">" ) );
              Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
          u8 data = atoi( data_str );//.toInt();
          if( data == 0 && !isdigit( data_str[ 0 ] ) )
          {
//            Serial.println( F( "Did not convert to integer" ) );
              if( strstr( data_str, "\"" ) == data_str )//???This is logic from first version.  Lost rack of how/if it works
              {
//                unsigned int str_ptr = 0;
                unsigned int address_start = address;
                unsigned int address_end = address + strlen( data_str )- 1;//???This is logic from first version.  Lost rack of how/if it works
                if( address_end < EEPROMlength )
                {
//                Serial.println( F( "Ready to receive alpha-numeric" ) );
//             Serial.print( F( "Start address: " ) );
//             Serial.print( address_start );
//             Serial.print( F( " End address: " ) );
//             Serial.println( address_end );
               for( unsigned int address = address_start; address <= address_end; address++ )
               {
                Serial.print( address_end-address );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                Serial.print( sizeof( data_str )- ( address_end-address )- 3 );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                 if( data_str[sizeof( data_str )- ( address_end-address )] != '\"' )
                 {
                    Serial.print( F( "Address: " ) );
                    Serial.print( address );
                    Serial.print( F( " entered data: " ) );
                    Serial.print( data_str[sizeof( data_str ) - ( address_end-address ) - 3 ] );
                    Serial.print( F( " previous data: " ) );
                    Serial.print( EEPROM.read( address ) );
#ifndef __LGT8FX8E__
              EEPROM.update( address, data );
#else
              EEPROM.write( address, data );
#endif
                    Serial.print( F( " newly stored data: " ) );
                    Serial.print( EEPROM.read( address ) );
                    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                  }
                  else
                  {
                    if( address <= address_end )
                    {
                        if( strstr( data_str, "\"" ) == data_str + strlen( data_str ) - 2 ) //???This is logic from first version.  Lost rack of how/if it works
                        {
                           Serial.print( F( "Putting a null termination at Address: " ) );
                           Serial.print( ++address );
                           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
#ifndef __LGT8FX8E__
                           EEPROM.update( address, 0 );
#else
                          EEPROM.write( address, 0 );
#endif
                        }
                    }
                  }
                }
                }
                else
                {
                Serial.print( F( "That data string is too long to go to that address so close to the end of the EEPROM.  It would not fit." ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                strFull = "";
//                return false;
                return;
                }
                strFull = "";
              }
              else
              {
                Serial.print( F( "Invalid data entered" ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                strFull = "";
//                return false;
                return;
              }
          }
          if( data > 255 || address >= EEPROMlength )
          {
            Serial.print( F( "Out of range.  Address can only be 0 to " ) );
            Serial.print( EEPROMlength - 1 );
            Serial.print( F( ", data can only be 0 to 255" ) );
            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
          }
          else
          {
              Serial.print( F( "Address: " ) );
              Serial.print( address );
              Serial.print( F( " entered data: " ) );
              Serial.print( data );
              Serial.print( F( " previous data: " ) );
              Serial.print( EEPROM.read( address ) );
#ifndef __LGT8FX8E__
              EEPROM.update( address, data );
#else
              EEPROM.write( address, data );
#endif
              Serial.print( F( " newly stored data: " ) );
              Serial.print( EEPROM.read( address ) );
              Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
          }
           strFull = "";
        }
#ifndef __LGT8FX8E__
        else if( strstr( strFull.c_str(), "vi fact" ) ) // The wemo xi does not have enough room in PROGMEM for this feature
        {
            print_factory_defaults();
           strFull = "";
        }
#endif
        else if( strstr( strFull.c_str(), "reset" ) )
        {
           restore_factory_defaults();
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "test alert" ) )
        {
           Serial.print( F( "time_stamp_this ALERT test as requested" ) );
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strstr( strFull.c_str(), "dht pin" ) )
        {
//           pin_specified_str = "";//strFull.substring( 7 );
           int i = 7;
           while( strFull.c_str()[ i ] == ' ' && strlen( strFull.c_str() ) > i ) i++;
//            pin_specified_str = strFull.substring( i );
//            Serial.print( strFull.length() );
//            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//            Serial.print( strFull.c_str()[ i ] );
//            if( isdigit( strFull.c_str()[ i + 1 ] ) ) Serial.print( strFull.c_str()[ i + 1 ] );
//            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//           if( pin_specified_str.length() > 0 ) pin_specified_str = strFull.substring( 8, strFull.length() - 1 );
//           else pin_specified_str = strFull.substring( 8, strFull.length() );
//           if( pin_specified_str.length() < 1 ) pin_specified_str = strFull.substring( 8, strFull.length() ); //In case line not ended with that CR or LF
         if( IsValidPinNumber( &strFull.c_str()[ i ] ) )
         {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
               {
                 Serial.print( F( "Pin " ) );
                 Serial.print( pin_specified );
                 Serial.print( F( " DHT read: " ) );
                 DHTresult* noInterrupt_result = ( DHTresult* )DHTreadWhenRested( pin_specified );
                 if( noInterrupt_result->ErrorCode == DEVICE_READ_SUCCESS )
                 {
                     Serial.print( ( float )( ( float )noInterrupt_result->TemperatureCelsius / 10 ), 1 );
                     Serial.print( F( " °C, " ) );
                     Serial.print( ( float )( ( float )noInterrupt_result->HumidityPercent / 10 ), 1 );
                     Serial.print( F( " %" ) );
                 }
                 else
                 {
                     Serial.print( F( "Error " ) );
                     Serial.print( noInterrupt_result->ErrorCode );
                 }
  /*
  #define TYPE_KNOWN_DHT11 1
  #define TYPE_KNOWN_DHT22 2
  #define TYPE_LIKELY_DHT11 3
  #define TYPE_LIKELY_DHT22 4
   */
                 Serial.print( F( " TYPE_" ) );
                 if( noInterrupt_result->Type == TYPE_KNOWN_DHT11 ) Serial.print( F( "KNOWN_DHT11" ) );
                 else if( noInterrupt_result->Type == TYPE_KNOWN_DHT22 ) Serial.print( F( "KNOWN_DHT22" ) );
                 else if( noInterrupt_result->Type == TYPE_LIKELY_DHT11 ) Serial.print( F( "LIKELY_DHT11" ) );
                 else if( noInterrupt_result->Type == TYPE_LIKELY_DHT22 ) Serial.print( F( "LIKELY_DHT22" ) );
                 Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
             if( strFull.c_str()[ i ] != '.' && !( strFull.c_str()[ i ] == ' ' && strFull.c_str()[ i + 1 ] == '.' ) ) break;
             }
           }
           strFull = "";
        }
/*
        else if( strFull.indexOf( F( "test hidden function" ) ) == 0 )
        {//inside this section is playground sandbox,  rebuild as needed....
           pin_specified_str = strFull.substring( 21 );
           if( pin_specified_str.length() < 1 ) pin_specified_str = strFull.substring( 21, strFull.length() ); //In case line not ended with that CR or LF
           if( IsValidPinNumber( pin_specified_str.c_str() ) )
           {
               Serial.print( F( ".." ) );
               Serial.print( F( "For pin " ) );
               Serial.print( ( u8 )pin_specified_str.toInt() );
               Serial.print( F( ": " ) );
               DHTresult* noInterrupt_result = ( DHTresult* )DHTreadWhenRested( ( u8 )pin_specified_str.toInt() );
               if( noInterrupt_result->ErrorCode == DEVICE_READ_SUCCESS )
               {
                   Serial.print( ( float )( ( float )noInterrupt_result->TemperatureCelsius / 10 ), 1 );
                   Serial.print( F( " °C, " ) );
                   Serial.print( ( float )( ( float )noInterrupt_result->HumidityPercent / 10 ), 1 );
                   Serial.print( F( " %" ) );
               }
               else
               {
                   Serial.print( F( "Error " ) );
                   Serial.print( noInterrupt_result->ErrorCode );
               }
               Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           }
           strFull = "";
        }
*/
        else
        {
          if( !strstr( strFull.c_str(), "help" ) )
          {
             Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
             Serial.print( strFull );
             Serial.print( F( ": not a valid command, remember they are cAsE sEnSiTiVe" ) );
             Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
             Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
          }
          printBasicInfo();
          strFull = "";
        }
    }
//  digitalWrite(LED_BUILTIN, LOW ); 

}
u8 last_three_temps_index = 0;
float old_getCelsius_temp = 0;
float oldtemp = 0;
u8 furnace_state = 0;
unsigned long timeOfLastSensorTimeoutError = 0;

void loop()
{
long unsigned check_furnace_effectiveness_time;
bool millis_overflowed = false;
if( fresh_powerup && logging )
{
  if( Serial )
  {
    printBasicInfo();
    fresh_powerup = false;
 }
}
else fresh_powerup = false;
//  Serial.println( F( "Starting sensor read" ) );
/*
  digitalWrite(LED_BUILTIN, HIGH ); //informs us that temp sensor is being communicated with
delay( 100 );
  digitalWrite(LED_BUILTIN, LOW ); 
delay( 100 );
  digitalWrite(LED_BUILTIN, HIGH ); //informs us that temp sensor is being communicated with
delay( 100 );
  digitalWrite(LED_BUILTIN, LOW ); 
delay( 100 );
  digitalWrite(LED_BUILTIN, HIGH ); //informs us that temp sensor is being communicated with
delay( 100 );
  digitalWrite(LED_BUILTIN, LOW ); 
delay( 100 );
  digitalWrite(LED_BUILTIN, HIGH ); //informs us that temp sensor is being communicated with
delay( 100 );
  digitalWrite(LED_BUILTIN, LOW ); 
delay( 100 );
  digitalWrite(LED_BUILTIN, HIGH ); //informs us that temp sensor is being communicated with
delay( 100 );
  digitalWrite(LED_BUILTIN, LOW ); 
delay( 100 );
  digitalWrite(LED_BUILTIN, HIGH ); //informs us that temp sensor is being communicated with
*/
//    int result = DHTLib.acquireAndWait();
//Serial.print( F( "..primary_temp_sensor_pin =  " ) );
//Serial.print( primary_temp_sensor_pin );
//Serial.print( F( ".." ) );
    DHTresult* noInterrupt_result = ( DHTresult* )( DHTreadWhenRested( primary_temp_sensor_pin ) );
    if( noInterrupt_result->ErrorCode != DEVICE_READ_SUCCESS ) noInterrupt_result = ( DHTresult* )( DHTreadWhenRested( secondary_temp_sensor_pin ) );
    if( noInterrupt_result->ErrorCode == DEVICE_READ_SUCCESS )
    {
/*
        Serial.print( F( "Humidity (%): " ) );
        Serial.print( _HumidityPercent, 1 );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        Serial.print( F( "Temperature (°C): " ) );
        Serial.print( _TemperatureCelsius, 1 );
*/
//Serial.print( noInterrupt_result.TemperatureCelsius );
//Serial.print( F( " °C, " ) );
//Serial.print( noInterrupt_result.HumidityPercent );
//Serial.print( F( " %" ) );
        timeOfLastSensorTimeoutError = 0;
        
        if( noInterrupt_result->TemperatureCelsius & 0x8000 ) _TemperatureCelsius = 0 - ( float )( ( float )( noInterrupt_result->TemperatureCelsius & 0x7FFF )/ 10 );
        else _TemperatureCelsius = ( float )( ( float )( noInterrupt_result->TemperatureCelsius & 0x7FFF )/ 10 );
        _HumidityPercent = ( float )( ( float )noInterrupt_result->HumidityPercent / 10 );
        last_three_temps[ last_three_temps_index ] = _TemperatureCelsius;
        float newtemp = ( float )( ( float )( last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ] )/ 3 );
        if( last_three_temps[ 0 ] != -100 && last_three_temps[ 1 ] != -101 && last_three_temps[ 2 ] != -102 )
        {
             if( furnace_state == 1 && furnace_started_temp_x_3 < last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ] )
             {
                  check_furnace_effectiveness_time = millis() + ( minutes_furnace_should_be_effective_after * 60000 );  //Will this become invalid if furnace temp setting gets adjusted?  TODO:  Account for that conditioin
                  if( check_furnace_effectiveness_time < minutes_furnace_should_be_effective_after * 60000 )
                  {
                        millis_overflowed = true;
                  }
                  else
                  {
                        millis_overflowed = false;
                  }
             }
        }
        if( ( newtemp != oldtemp ) && ( last_three_temps[ last_three_temps_index ] != old_getCelsius_temp ) )
        {
            if( logging && logging_temp_changes )
            {
                Serial.print( F( "time_stamp_this Temperature change to " ) );
                Serial.print( ( float )last_three_temps[ last_three_temps_index ], 1 );
                Serial.print( F( " °C " ) );
                if( noInterrupt_result == &DHTfunctionResultsArray[ primary_temp_sensor_pin - 1 ] ) Serial.print( F( "primary" ) );
                else Serial.print( F( "secondary" ) );
                Serial.print( F( " sensor" ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
            }
            old_getCelsius_temp = last_three_temps[ last_three_temps_index ];
        }
        oldtemp = newtemp;
        last_three_temps_index = ++last_three_temps_index % 3;
           if( furnace_state == 0 && ( thermostat == 'h' || thermostat == 'a' ) && last_three_temps[ 0 ] != -100 && last_three_temps[ 1 ] != -101 && last_three_temps[ 2 ] != -102 )
           {
    //            Serial.print( F( "Furnace is off?: " ) );
    //            Serial.println( furnace_state );
              if( last_three_temps[ 0 ] < lower_furnace_temp_floated && last_three_temps[ 1 ] < lower_furnace_temp_floated && last_three_temps[ 2 ] < lower_furnace_temp_floated && last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ] > lower_furnace_temp_floated )
              {
    //      Serial.println( F( "Turning furnace on" ) );
                  digitalWrite( furnace_pin, HIGH ); 
                  digitalWrite( furnace_fan_pin, HIGH );
                  furnace_state = 1;
                  timer_alert_furnace_sent = 0;
                  if( logging )
                  {
                        Serial.print( F( "time_stamp_this Furnace and furnace fan on (pins " ) );
                        Serial.print( furnace_pin );
                        Serial.print( F( " and " ) );
                        Serial.print( furnace_fan_pin );
                        Serial.print( F( ")" ) );
                        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                  }
                  check_furnace_effectiveness_time = millis() + ( minutes_furnace_should_be_effective_after * 60000 );  //Will this become invalid if furnace temp setting gets adjusted?  TODO:  Account for that conditioin
                  if( check_furnace_effectiveness_time < minutes_furnace_should_be_effective_after * 60000 )
                  {
                        millis_overflowed = true;
                  }
                  else
                  {
                        millis_overflowed = false;
                  }
                  furnace_started_temp_x_3 = last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ];
              }
           }
           else if( furnace_state == 1 && ( thermostat == 'h' || thermostat == 'a' ) )
           {
//            Serial.print( F( "Furnace is on?: " ) );
//            Serial.println( furnace_state );
               if( last_three_temps[ 0 ] > upper_furnace_temp_floated && last_three_temps[ 1 ] > upper_furnace_temp_floated && last_three_temps[ 2 ] > upper_furnace_temp_floated )
               {
                    //      Serial.println( F( "Turning furnace off" ) );
                    digitalWrite( furnace_pin, LOW );
                    digitalWrite( furnace_fan_pin, LOW );
                      furnace_state = 0;
                      timer_alert_furnace_sent = 0;
                    //               if( fan_mode == 'a' ) digitalWrite( furnace_fan_pin, LOW ); 
                    if( logging )
                    {
                        Serial.print( F( "time_stamp_this Furnace" ) );
                        if( fan_mode == 'a' ) Serial.print( F( " and furnace fan" ) );
                        Serial.print( F( " off (pin" ) );
                        if( fan_mode == 'a' ) Serial.print( F( "s" ) );
                        Serial.print( F( " " ) );
                        Serial.print( furnace_pin );
                        if( fan_mode == 'a' )
                        {
                            Serial.print( F( " and " ) );
                            Serial.print( furnace_fan_pin );
                        }
                        Serial.print( F( ")" ) );
                        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                    }
               }
               else if( !timer_alert_furnace_sent && furnace_started_temp_x_3 > last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ] )
               {
                    if( !millis_overflowed )
                        if( millis() > check_furnace_effectiveness_time || millis() < check_furnace_effectiveness_time - minutes_furnace_should_be_effective_after * 60000 )
                        {
                              goto sendAlert;
                        }
                    else if( millis() > check_furnace_effectiveness_time && millis() < check_furnace_effectiveness_time )
                    {
                         goto sendAlert;
                    }
                    goto afterSendAlert;
sendAlert:;
//                    // check_furnace_effectiveness_time is set when the furnace is ON'd (turned on)
                    //The following is output regardless of logging mode                                                         //  to millis() + ( minutes_furnace_should_be_effective_after * 60000 )
                        Serial.print( F( "time_stamp_this ALERT Furnace not heating after allowing " ) );                          //  which is a millis() value to alert at if no heating happened yet
                        Serial.print( ( u8 )( minutes_furnace_should_be_effective_after + ( ( millis() - check_furnace_effectiveness_time ) / 60000 ) ) ); //minutes_furnace_should_be_effective_after is a const of a number of minutes
                        Serial.print( F( " minutes" ) );
                        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                        timer_alert_furnace_sent = loop_cycles_to_skip_between_alert_outputs;
                        if( !timer_alert_furnace_sent )
                            timer_alert_furnace_sent = 1;
/*                    send an alert output if furnace not working: temperature dropping even though furnace got turned on.  the way to determine this is to have a flag of whether the alert output is already sent;
 *                     That flag will unconditionally get reset in the following situations: thermostat setting gets changed, upper_furnace_temp_floated or lower_furnace_temp_floated gets changed, room temperature rises while furnace is on, 
 */
afterSendAlert:;
               }
               else if( timer_alert_furnace_sent )
               {
                    timer_alert_furnace_sent--;
               }
           }
/*
       else if( furnace_state == 1 && ( thermostat == 'h' || thermostat == 'a' ) )
       {
            Serial.print( F( "Furnace is on?: " ) );
            Serial.println( furnace_state );
           if( last_three_temps[ 0 ] > upper_furnace_temp_floated && last_three_temps[ 1 ] > upper_furnace_temp_floated && last_three_temps[ 2 ] > upper_furnace_temp_floated && last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ] > lower_furnace_temp_floated )
           {
//      Serial.println( F( "Turning furnace off" ) );
               digitalWrite( furnace_pin, LOW );
               if( fan_mode == 'a' ) digitalWrite( furnace_fan_pin, LOW );
              if( logging )
              {
                Serial.print( F( "time_stamp_this Furnace" ) );
                if( fan_mode == 'a' ) Serial.print( F( " and furnace fan" ) );
                Serial.print( F( " off ( pin" ) );
                if( fan_mode == 'a' ) Serial.print( F( "s" ) );
                Serial.print( F( " " ) );
                Serial.print( furnace_pin );
                if( fan_mode == 'a' )
                {
                    Serial.print( F( " and " ) );
                Serial.print( furnace_fan_pin );
                }
                Serial.println( F( " )" ) );
              }
           }
*/       
    }
    else
    {
        timeOfLastSensorTimeoutError++;
        Serial.print( F( "time_stamp_this " ) );
        if( timeOfLastSensorTimeoutError > 100 ) Serial.print( F( "ALERT " ) );//These ALERT prefixes get added after consecutive 100 timeout fails
        Serial.print( F( "Temperature sensor TIMEOUT error" ) );
//        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//        Serial.print( F( "Time out error" ) ); 
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    }

     



//  Serial.print( F( "Humidity (%): " ) );
//  Serial.println(DHTLib.getHumidity(), 0 );

//  Serial.print( F( "Temperature (oC): " ) );
//  Serial.println(DHTLib.getCelsius(), 0 );

//  Serial.print( F( "Temperature (oF): " ) );
//  Serial.println(DHTLib.getFahrenheit(), 2 );

//  Serial.print( F( "Temperature (K ): " ) );
//  Serial.println(DHTLib.getKelvin(), 2 );

//  Serial.print( F( "Dew Point (oC): " ) );
//  Serial.println(DHTLib.getDewPoint() );

//  Serial.print( F( "Dew Point Slow (oC): " ) );
//  Serial.println( DHTLib.getDewPointSlow() );
check_for_serial_input( noInterrupt_result->ErrorCode );
  delay( 2000 );
}
