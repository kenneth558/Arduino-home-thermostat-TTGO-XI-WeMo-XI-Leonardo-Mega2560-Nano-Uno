#define VERSION "0.0.0041"//;//TODO:  add labels to pins
short unsigned _baud_rate_ = 57600;//Very much dependent upon the capability of the host computer to process talkback data, not just baud rate of its interface
/*
EEPROM addressing: use digital pin number * 2 as LSB of start address that pin description is found. The MSB of that start address is located in the following EEPROM location (digital pin number * 2 )+ 1.  

All line ends printed are with MS Windows conventions due to Arduino print and println commands.  To retain line ends compatability between *nix and MS Windows, we won't embed any new lines into print[ln] literals.  
...TODO: Interrupts should be better utilized so that loop()program flow is not paused for all those time frames ( seconds between reads and data stream bit times.  Always allow for event of defective/disconnection of devices
...TODO: Add more externally-scripted functions, like entire port pin changes, watches on pins with routines that will execute routines to any combo of pins upon pin[s] conditions,
...TODO: alert when back pressure within furnace indicates to change filter
...TODO: damper operation with multiple temp sensors
 PORTB |= ( 1<<5); // set bit 5 of PORTB
 PORTC &= ~( 1<<3 ); // clear bit 3 of PORTC
 PORTD ^= ( 1<<2 ); // toggle bit 2 of PORTD
Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );


*/
bool mswindows = false;  //Used for line-end on serial outputs.  Make true to print line ends as MS Windows needs  
#include <EEPROM.h>
#include "DHTdirectRead.h"
//All temps are shorts until displayed
const char strFactoryDefaults[] PROGMEM = "\
pin  0 \"Serial Rx communications received by Arduino board ( protected pin of several boards )\"\n\
pin  1 \"Serial Tx communications sent out by Arduino board ( protected pin of several boards )\"\n\
pin  2 \"Main room primary indoor temperature sensor DHT11/22\"\n\
pin  3 \"Furnace start/stop control, primary stage\"\n\
pin  4 \"Furnace blower fan auto/on control\"\n\
pin  5 \"Power cycle start/stop control of installer-selected system\"\n\
pin   \"Furnace humidifier on/off control ( starts only with furnace heat )\"\n\
pin   \"Side room A primary indoor temperature sensor DHT11/22\"\n\
pin   \"Side room A damper sensor return ( analog signal )\"\n\
pin   \"Side room A damper control\"\n\
pin   \"Furnace start/stop control, secondary stage\"\n\
pin   \"Furnace start/stop control, tertiary stage\"\n\
pin  6 \"Front porch light\"\n\
pin  7 \"Back porch light\"\n\
pin   \"Stand-alone humidifier start/stop control\"\n\
pin   \"Air conditioner start/stop control, primary stage\"\n\
pin   \"Air conditioner start/stop control, secondary stage\"\n\
pin 13 \"BUILTIN_LED ( protected pin of several boards )\"\n\
pin   \"Outdoor temperature sensor DHT22\"\n\
pin   \"Air conditioner start/stop control, tertiary stage\"\n\
pin   \"Side room A secondary indoor temperature sensor DHT11/22\"\n\
pin  8 \"Main room secondary indoor temperature sensor DHT11/22\"\n\
";

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
//  The following are addresses in EEPROM of values that need to be persistent across power outages.
//The first two address locations in EEPROM store a tatoo used to ensure EEPROM contents are valid for this sketch
u8 primary_temp_sensor_address = 2;
u8 furnace_address = 3;
u8 furnace_fan_pin_address = 4;
u8 power_cycle_address = 5;
u8 thermostat_address = 6;
u8 fan_mode_address = 7;
u8 secondary_temp_sensor_address = 8;

unsigned int logging_address = EEPROM.length() - sizeof( boolean );
unsigned int upper_furnace_temp_address = logging_address - sizeof( short );//EEPROM.length() - 2;
unsigned int lower_furnace_temp_address = upper_furnace_temp_address - sizeof( short );//EEPROM.length() - 3;
unsigned int logging_temp_changes_address = lower_furnace_temp_address - sizeof( boolean );//EEPROM.length() - 4;

// So we don't assume every board has exactly three
//in case of boards that have more than 24 pins ( three virtual 8-bit ports )
//int number_of_ports_that_present_pins = EEPROM.read( EEPROM.length() - 5 );  // may never need this

// main temperature sensor, furnace, auxiliary furnace device, system power cycle, porch light, dining room coffee outlet
//  The following values need to be stored persistent through power outages
u8 primary_temp_sensor_pin = EEPROM.read( primary_temp_sensor_address ); 
u8 secondary_temp_sensor_pin = EEPROM.read( secondary_temp_sensor_address ); 
u8 furnace_pin = EEPROM.read( furnace_address );
u8 furnace_fan_pin = EEPROM.read( furnace_fan_pin_address );
u8 power_cycle_pin = EEPROM.read( power_cycle_address );
boolean logging = ( boolean )EEPROM.read( logging_address );
boolean logging_temp_changes = ( boolean )EEPROM.read( logging_temp_changes_address );
float lower_furnace_temp_floated;//filled in setup
float upper_furnace_temp_floated;
char thermostat = ( char )EEPROM.read( thermostat_address );
char fan_mode = ( char )EEPROM.read( fan_mode_address );//a';//Can be either auto (a) or on (o)

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

String str;
String strFull = "";
String pin_specified_str;
String temp_specified_str;
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

boolean IsValidPinNumber( String str )
{
  bool first_run=true;
    for( unsigned int i = 0; i < str.length(); i++ )
    {
        if( ( !( isDigit( str.charAt( i ) ) || str.charAt( i ) == '.' ) && first_run ) || ( !first_run && str.charAt( i ) != 10 && str.charAt( i ) != 13 ) )
        {
            Serial.print( F( "Command must begin or end with a pin number as specified in help screen" ) );
            Serial.print( str );
            Serial.print( F( ">" ) );
            Serial.print( ( u8 )str.charAt( i ) );
            Serial.print( str.charAt( i ) );
//            Serial.print( F( "," ) );
//            Serial.print( i );
            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
            return false;
        }
        first_run=false;
    }
    pin_specified=str.toInt();
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

boolean IsValidTemp( String str )
{
    for( unsigned int i = 0; i < str.length(); i++ )
    {
        if( !( isDigit( str.charAt( i ) )|| str.charAt( i ) == '.' || ( !i && str.charAt( 0 ) == '-' ) ) )
        {
            Serial.print( F( "Command must begin with a temperature in Celsius" ) );
            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
            return false;
        }
    }
    temp_specified_floated = ( float )str.toInt();
    if( str.indexOf('.') ){ temp_specified_floated += ( ( float )str.substring( str.indexOf('.') + 1 ).toInt() ) / 10; };
    signed char upper_limit = 37;
    signed char lower_limit = -37;
    if(  temp_specified_floated < lower_limit || temp_specified_floated > upper_limit )//This check should be called for the rooms where it matters
    {
        Serial.print( F( "Temperature must be Celsius " ) );
        Serial.print( lower_limit );
        Serial.print( F( " through " ) );
        Serial.print( upper_limit );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        return false;
    }
//    return true;
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
    Serial.print( F( "Valid commands (case sensitive, minimal sanity checking, one per line) are:" ) );
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
    Serial.print( F( "talkback[ on/off] (or logging on/off)(for the host system to log when each output pin gets set high or low, always persistent)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "talkback temp change[s[ on/off]] (or logging temp changes on/off)(requires normal talkback on - for the host system to log whenever the main room temperature changes, always persistent)" ) );
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
    Serial.print( F( "vi[ew] fact[ory defaults] (so you can see what would happen before you reset to them)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "reset (factory defaults: pure, simple and absolute)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "test alert (sends an alert message to host for testing purposes)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "dht pin <pin number> (retrieves DHT device reading, a period with due care in place of pin number for all pins)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "test hidden function <pin number> (for experimental code debugging)" ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
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
    EEPROM.update( primary_temp_sensor_address, factory_setting_primary_temp_sensor_pin );//2 );
    EEPROM.update( furnace_address, factory_setting_furnace_pin );//3 );
    EEPROM.update( furnace_fan_pin_address, factory_setting_furnace_fan_pin );//4 );
    EEPROM.update( power_cycle_address, factory_setting_power_cycle_pin );//5);
    EEPROM.update( logging_address, factory_setting_logging_setting );//( uint8_t )true );
    EEPROM.update( logging_temp_changes_address, factory_setting_logging_temp_changes_setting );//( uint8_t )true );
    EEPROM.update( thermostat_address, factory_setting_thermostat_mode );//'a' );
    EEPROM.update( fan_mode_address, factory_setting_fan_mode );//'a' );
//These two must be put's because their data types are longer than one.  Data types of length one can be either updates or puts.
    short factory_setting_lower_furnace_temp_shorted_times_ten = ( short )( factory_setting_lower_furnace_temp_floated * 10 );
    short factory_setting_upper_furnace_temp_shorted_times_ten = ( short )( factory_setting_upper_furnace_temp_floated * 10 );
    EEPROM.put( lower_furnace_temp_address, factory_setting_lower_furnace_temp_shorted_times_ten );//21 );
    EEPROM.put( upper_furnace_temp_address, factory_setting_upper_furnace_temp_shorted_times_ten );//21 );
    EEPROM.update( secondary_temp_sensor_address, factory_setting_secondary_temp_sensor_pin );//2 );
    EEPROM.put( 0, ( NUM_DIGITAL_PINS + 1 ) * 3 );//Tattoo the board
//    EEPROM.update( 1, ( u8 )( ( ( NUM_DIGITAL_PINS + 1 ) * 3 ) >> 8 ) );//Tattoo the board
//    EEPROM.update( EEPROM.length() - 4, 114 );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Factory initialization complete.  Now allowing you 10-13 seconds to unplug the Arduino in case this environment is for programming only." ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    //EEPROM.update( EEPROM.length() - 5, 3 );//not used for now, forgot what the 3 means.  Mode or ID of some sort?
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
//  Serial.println( F( "idDHTLib Example program" ) );
//  Serial.print( F( "LIB version: " ) );
//  Serial.println( IDDHTLIB_VERSION );
//  Serial.println( F( "---------------" ) );
//  pinMode( LED_BUILTIN, OUTPUT );
//  digitalWrite( LED_BUILTIN, LOW );    
pinMode( power_cycle_pin, OUTPUT );
digitalWrite( power_cycle_pin, LOW );
pinMode( furnace_pin, OUTPUT );
digitalWrite( furnace_pin, LOW );
pinMode( furnace_fan_pin, OUTPUT );
if( fan_mode == 'o' ) digitalWrite( furnace_fan_pin, HIGH );
else digitalWrite( furnace_fan_pin, LOW );
//read EEPROM addresses 0 (LSB)and 1 (MSB).  MSB combined with LSB should always contain ( NUM_DIGITAL_PINS + 1 ) * 3.
u16 tattoo = 0;
EEPROM.get( 0, tattoo );
//tattoo |= EEPROM.read( 0 ); //Location 0 should contain 
//MAKE THE FOLLOWING INTO A STANDALONE ROUTINE INSTEAD OF EXPLICIT HERE because they can also be needed if end user wants to reset to factory defaults
if( tattoo != ( NUM_DIGITAL_PINS + 1 ) * 3 ) // Check for tattoo
{
    while ( !Serial ); // wait for serial port to connect. Needed for Leonardo's native USB
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
    Serial.print( F( "Detected first time run so initializing to factory default pin names, power-up states and thermostat assignments.  Please wait..." ) );
    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );

   restore_factory_defaults();
}
short lower_furnace_temp_shorted_times_ten = 0;
EEPROM.get( lower_furnace_temp_address, lower_furnace_temp_shorted_times_ten );
lower_furnace_temp_floated = ( float )( ( float )( lower_furnace_temp_shorted_times_ten ) / 10 );
short upper_furnace_temp_shorted_times_ten = 0;
EEPROM.get( upper_furnace_temp_address, upper_furnace_temp_shorted_times_ten );
upper_furnace_temp_floated = ( float )( ( float )( upper_furnace_temp_shorted_times_ten ) / 10 );
  delay( 3000 ); // The sensor needs time to initialize, if you have some code before this that make a delay, you can eliminate this delay
  //( pin_specified*3 )and ( pin_specified*3 )+1 contains the EEPROM address where the pin's assigned name is stored.  Pin 0 will always have its name stored at EEPROM address (NUM_DIGITAL_PINS+1 )*3, so address (NUM_DIGITAL_PINS+1 )*3 will always be stored in EEPROM addresses 0 and 1; 0 = ( pin_number*3 )and 1 = (( pin_number*3 )+1 ) )]
  // That will be the way we determine if the EEPROM is configured already or not
  // ( pin_specified*3 )+2 is EEPROM address where the pin's desired inital state is stored
}
// This wrapper is in charge of calling 
// mus be defined like this for the lib work
/*
void dhtLib_wrapper(){
  DHTLib.dht11Callback(); // Change dht11Callback()for a dht22Callback()if you have a DHT22 sensor
}
*/
void check_for_serial_input( char result )
{
    if( Serial.available() > 0 )
    {
//  digitalWrite(LED_BUILTIN, HIGH );
        str = Serial.readStringUntil( '\n' );
        strFull = strFull + str;
        strFull.replace( F( "talkback" ), F( "logging" ) );
        strFull.replace( F( "pin set" ), F( "set pin" ) );
        strFull.replace( F( "set pin to" ), F( "set pin" ) );
        strFull.replace( F( "view" ), F( "vi" ) );
        strFull.replace( F( "persistent memory" ), F( "pers" ) );
        strFull.replace( F( "changes" ), F( "ch" ) );
        strFull.replace( F( "change" ), F( "ch" ) );
        strFull.replace( F( "thermostat" ), F( "ther" ) );
        strFull.replace( F( "fan auto" ), F( "fan a" ) );
        strFull.replace( F( "fan on" ), F( "fan o" ) );
        strFull.replace( F( "factory" ), F( "fact" ) );
        if( strFull.indexOf( F( "power cycle" ) ) >= 0 || strFull.indexOf( F( "cycle power" ) ) >= 0 )
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
        else if( strFull.indexOf( F( "set lower furnace temp" ) ) >= 0 )
        {
           temp_specified_str = strFull.substring( 0, strFull.indexOf( ' ' ) );
           if( IsValidTemp( temp_specified_str ) )
           {
              if( temp_specified_floated <= upper_furnace_temp_floated )
              {
                lower_furnace_temp_floated = temp_specified_floated;
                timer_alert_furnace_sent = 0;
                short temp_specified_shorted_times_ten = ( short )( temp_specified_floated * 10 );
                EEPROM.put( lower_furnace_temp_address, temp_specified_shorted_times_ten );
              }
              else 
              {
                Serial.print( F( "Upper furnace temp too low for this value. Raise that setting before retrying this value" ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
              }
            }
            strFull = "";
        }
        else if( strFull.indexOf( F( "set upper furnace temp" ) ) >= 0 )
        {
           temp_specified_str = strFull.substring( 0, strFull.indexOf( ' ' ) );
           if( IsValidTemp( temp_specified_str ) )
           {
              if( temp_specified_floated >= lower_furnace_temp_floated )
              {
                upper_furnace_temp_floated = temp_specified_floated;
                timer_alert_furnace_sent = 0;
                short temp_specified_shorted_times_ten = ( short )( temp_specified_floated * 10 );
                EEPROM.put( upper_furnace_temp_address, temp_specified_shorted_times_ten );
              }
              else 
              {
                Serial.print( F( "Lower furnace temp too high for this value. Lower that setting before retrying this value" ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
              }
           }
           strFull = "";
        }
        else if( strFull.indexOf( F( "report master room temp" ) ) >=0 )
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
        else if( strFull.indexOf( F( "read pins" ) ) >= 0 || strFull.indexOf( F( "pins read" ) ) >= 0 )
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
        else if( strFull.indexOf( F( "read pin" ) ) >= 0 || strFull.indexOf( F( "pin read" ) ) >= 0 )
        {
           pin_specified_str = strFull.substring( 0, strFull.indexOf( ' ' ) );
           if( IsValidPinNumber( pin_specified_str ) )
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
                 if( data < EEPROM.length() - 1 )
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
        else if( strFull.indexOf( F( "set pin high" ) ) >= 0 )
        {
           pin_specified_str = strFull.substring( 0, strFull.indexOf( ' ' ) );
           if( IsValidPinNumber( pin_specified_str ) )
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
        else if( strFull.indexOf( F( "set pin low" ) ) >= 0 )
        {
           pin_specified_str = strFull.substring( 0, strFull.indexOf( ' ' ) );
           if( IsValidPinNumber( pin_specified_str ) )
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
        else if( strFull.indexOf( F( "set pin output" ) ) >= 0 )
        {
           pin_specified_str = strFull.substring( 0, strFull.indexOf( ' ' ) );
           if( IsValidPinNumber( pin_specified_str ) && pin_specified != SERIAL_PORT_HARDWARE )
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
        else if( strFull.indexOf( F( "set pin input with pullup" ) ) >= 0 )
        {
           pin_specified_str = strFull.substring( 0, strFull.indexOf( ' ' ) );
           if( IsValidPinNumber( pin_specified_str ) && pin_specified != SERIAL_PORT_HARDWARE )
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
        else if( strFull.indexOf( F( "set pin input" ) ) >= 0 )
        {
           pin_specified_str = strFull.substring( 0, strFull.indexOf( ' ' ) );
           if( IsValidPinNumber( pin_specified_str ) && pin_specified != SERIAL_PORT_HARDWARE )
           {
              if( pin_specified == power_cycle_pin || pin_specified == furnace_fan_pin || pin_specified == furnace_pin )
              {
                Serial.print( F( "Sorry, unable to allow that pin to be made an input. You've dedicated output only for " ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
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
        else if( strFull.indexOf( F( "ther a" ) ) == 0 || strFull.indexOf( F( "ther o" ) ) == 0 || strFull.indexOf( F( "ther h" ) ) == 0 || strFull.indexOf( F( "ther c" ) ) == 0 )
        {
           Serial.print( F( "Thermostat being set to " ) );
           int charat = strFull.indexOf( F( "ther " ) ) + 5;
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
           EEPROM.update( thermostat_address, strFull.charAt( charat ) );
after_change_thermostat:
           strFull = "";
        }
        else if( strFull.indexOf( F( "ther" ) ) == 0 )
        {
            if( strFull.indexOf( F( " " ) ) >= 4 )
            {
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
        else if( strFull.length() < 7 && ( strFull.indexOf( F( "fan a" ) ) == 0 || strFull.indexOf( F( "fan o" ) ) == 0 ) )
        {
           Serial.print( F( "Fan being set to " ) );
           int charat = strFull.indexOf( F( "fan " ) ) + 4;
           if( strFull.charAt( charat ) == 'a' ) Serial.print( F( "auto" ) );
           else if( strFull.charAt( charat ) == 'o' ) Serial.print( F( "on" ) );
           else { Serial.print( F( "<fault>. No change made" ) ); goto after_change_fan; } //Making extra sure that no invalid mode gets saved
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//change_fan:
           fan_mode = strFull.charAt( charat );
           if( fan_mode == 'o' ) digitalWrite( furnace_fan_pin, HIGH );
           else digitalWrite( furnace_fan_pin, LOW );
           EEPROM.update( fan_mode_address, strFull.charAt( charat ) );
after_change_fan:
           strFull = "";
        }
        else if( strFull.indexOf( F( "fan" ) ) == 0 )
        {
            if( strFull.indexOf( F( " " ) ) >= 3 )
            {
                Serial.print( F( "That space you entered also then requires a valid mode. The only valid characters allowed after that space are the options lower case a or o.  They mean auto and on and optionally may be spelled out completely" ) );
                Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
            }
            Serial.print( F( "Fan is " ) );
            if( fan_mode == 'a' ) Serial.print( F( "auto" ) );
            else if( fan_mode == 'o' ) Serial.print( F( "on" ) );
            Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strFull.indexOf( F( "logging temp ch o" ) ) >= 0 )
        {
           Serial.print( F( "Talkback for logging temp changes being turned o" ) );
           int charat = strFull.indexOf( F( "logging temp ch o" ) )+ 17;
           if( strFull.charAt( charat ) == 'n' ) logging_temp_changes = true;
           else logging_temp_changes = false;
           EEPROM.update( logging_temp_changes_address, ( uint8_t )logging_temp_changes );
           if( logging_temp_changes ) Serial.print( strFull.charAt( charat ) );
           else  Serial.print( F( "ff" ) );
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strFull.indexOf( F( "logging temp ch" ) ) >= 0 )
        {
           Serial.print( F( "Talkback for logging temp changes is turned o" ) );
           if( logging_temp_changes ) Serial.print( F( "n" ) );
           else  Serial.print( F( "ff" ) );
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strFull.indexOf( F( "logging o" ) ) >= 0 )
        {
           Serial.print( F( "Talkback for logging being turned o" ) );
           int charat = strFull.indexOf( F( "logging o" ) )+ 9;
           if( strFull.charAt( charat ) == 'n' ) logging = true;
           else logging = false;
           EEPROM.update( logging_address, ( uint8_t )logging );
           if( logging ) Serial.print( strFull.charAt( charat ) );
           else  Serial.print( F( "ff" ) );
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strFull.indexOf( F( "logging" ) ) >= 0 )
        {
           Serial.print( F( "Talkback for logging is turned o" ) );
           if( logging ) Serial.print( F( "n" ) );
           else  Serial.print( F( "ff" ) );
           Serial.print( strFull );
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strFull.indexOf( F( "vi pers" ) ) == 0 )
        {
          String address_str = strFull.substring( 7, strFull.length() );
          address_str = address_str.substring( address_str.indexOf( ' ' )+ 1 );
          unsigned int address_start = address_str.toInt();
          address_str = address_str.substring( address_str.indexOf( ' ' )+ 1 );
          unsigned int address_end = address_str.toInt();
          if( address_start < 0 || address_start >= EEPROM.length() || address_end < 0 || address_end >= EEPROM.length() )
          {
            Serial.print( F( "Out of range.  Each address can only be 0 to " ) );
            Serial.print( EEPROM.length() - 1 );
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
        else if( strFull.indexOf( F( "ch pers" ) ) == 0 )
        {
          String address_str = strFull.substring( 7, strFull.length() );
          address_str = address_str.substring( address_str.indexOf( ' ' )+ 1 );
          unsigned int address = address_str.toInt();
          String data_str = address_str.substring( address_str.indexOf( ' ' )+ 1 );
              Serial.print( F( " entered data: <" ) );
              Serial.print( data_str );
              Serial.print( F( ">" ) );
              Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
          uint8_t data = data_str.toInt();
          if( data == 0 && data_str != "0" )
          {
//            Serial.println( F( "Did not convert to integer" ) );
              if( data_str.startsWith( "\"" ) )
              {
//                unsigned int str_ptr = 0;
                unsigned int address_start = address;
                unsigned int address_end = address + data_str.length()- 1;
                if( address_end < EEPROM.length() )
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
                    EEPROM.update( address, data );
                    Serial.print( F( " newly stored data: " ) );
                    Serial.print( EEPROM.read( address ) );
                    Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                  }
                  else
                  {
                    if( address <= address_end )
                    {
                        if( data_str.endsWith( "\" 0" ) )
                        {
                           Serial.print( F( "Putting a null termination at Address: " ) );
                           Serial.print( ++address );
                           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                           EEPROM.update( address, 0 );
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
          if( data > 255 || address >= EEPROM.length() )
          {
            Serial.print( F( "Out of range.  Address can only be 0 to " ) );
            Serial.print( EEPROM.length() - 1 );
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
              EEPROM.update( address, data );
              Serial.print( F( " newly stored data: " ) );
              Serial.print( EEPROM.read( address ) );
              Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
          }
           strFull = "";
        }
        else if( strFull.indexOf( F( "vi fact" ) ) >= 0 )
        {
            print_factory_defaults();
           strFull = "";
        }
        else if( strFull.indexOf( F( "reset" ) ) == 0 )
        {
           restore_factory_defaults();
           strFull = "";
        }
        else if( strFull.indexOf( F( "test alert" ) ) == 0 )
        {
           Serial.print( F( "time_stamp_this ALERT test as requested" ) );
           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
           strFull = "";
        }
        else if( strFull.indexOf( F( "dht pin" ) ) == 0 )
        {
           pin_specified_str = strFull.substring( 8 );
           for( u8 next_pin = ( u8 )pin_specified_str.toInt(); next_pin < NUM_DIGITAL_PINS; next_pin++ )
           {
             if( IsValidPinNumber( pin_specified_str ) )
             {
                 Serial.print( F( "Pin " ) );
                 Serial.print( next_pin );
                 Serial.print( F( " DHT read: " ) );
                 DHTresult* noInterrupt_result = ( DHTresult* )DHTreadWhenRested( next_pin );
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
                 if( noInterrupt_result->Type == TYPE_KNOWN_DHT11 ) Serial.print( F( " TYPE_KNOWN_DHT11" ) );
                 else if( noInterrupt_result->Type == TYPE_KNOWN_DHT22 ) Serial.print( F( " TYPE_KNOWN_DHT22" ) );
                 else if( noInterrupt_result->Type == TYPE_LIKELY_DHT11 ) Serial.print( F( " TYPE_LIKELY_DHT11" ) );
                 else if( noInterrupt_result->Type == TYPE_LIKELY_DHT22 ) Serial.print( F( " TYPE_LIKELY_DHT22" ) );
                 Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
             }
             if( pin_specified_str.charAt( 0 ) != '.') next_pin = NUM_DIGITAL_PINS;
           }
           strFull = "";
        }
        else if( strFull.indexOf( F( "test hidden function" ) ) == 0 )
        {//inside this section is playground sandbox,  rebuild as needed....
           pin_specified_str = strFull.substring( 21 );
           if( pin_specified_str.length() < 1 ) pin_specified_str = strFull.substring( 8, strFull.length() ); //In case line not ended with that CR or LF
           if( IsValidPinNumber( pin_specified_str ) )
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
        else
        {
          if( strFull.indexOf( F( "help" ) )< 0 )
          {
             Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
             Serial.print( strFull );
             Serial.print( F( ": not a valid command" ) );
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
        if( timeOfLastSensorTimeoutError > 100 ) Serial.print( F( "ALERT " ) );
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
