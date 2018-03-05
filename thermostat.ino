/***********************************************************************************************************************
 *      ARDUINO HOME THERMOSTAT SKETCH  v.0.0.0045
 *      Author:  Kenneth L. Anderson
 *      Boards tested on: Uno Mega2560 WeMo XI/TTGO XI Leonardo Nano
 *      Date:  03/04/18
 * 
 * 
 * TODO:  labels to pins 
 *        Add more externally-scripted functions, like entire port pin changes, watches on pins with routines that will execute routines to any combo of pins upon pin[s] conditions,
 *        alert when back pressure within furnace indicates to change filter
 *        damper operation with multiple temp sensors
 * 
 * 
 *        https://github.com/wemos/Arduino_XI  for the IDE support for TTGO XI/WeMo XI
 * 
 * 
 * 
 *************************************************************************************************************************/
#define VERSION "0.0.0045"
#ifndef u8
    #define u8 uint8_t
#endif
#ifndef u16
    #define u16 uint16_t
#endif
#include "DHTdirectRead.h"
#include <EEPROM.h>
#ifndef __LGT8FX8E__
    short unsigned _baud_rate_ = 57600;//Very much dependent upon the capability of the host computer to process talkback data, not just baud rate of its interface
#else
    short unsigned _baud_rate_ = 19200;//The XI tends to revert to baud 19200 after programming or need that rate prior to programming so we can't risk setting baud to anything but that
    #define LED_BUILTIN 12
    #define NUM_DIGITAL_PINS 14
#endif

//All temps are shorts until displayed
//Where are other indoor sensors?
//What about utilizing PCI ISRs and how they control which pins to use?

// INTENT: Access these values by reference and pointers.  They will be hardcoded here only.  The run-time code can only find out what they are, how many there are, and their names by calls to this function
//  The following are addresses in EEPROM of values that need to be persistent across power outages.
//The first two address locations in EEPROM store a tatoo used to ensure EEPROM contents are valid for this sketch
u8 primary_temp_sensor_address = 2;
u8 furnace_pin_address = 3;
u8 furnace_fan_pin_address = 4;
u8 power_cycle_address = 5;
u8 thermostat_mode_address = 6;
u8 fan_mode_address = 7;
u8 secondary_temp_sensor_address = 8;
u8 cool_pin_address = 9;
u8 outdoor_temp_sensor1_address = 10;
u8 outdoor_temp_sensor2_address = 11;

#ifndef __LGT8FX8E__
    u16 EEPROMlength = EEPROM.length();
#else
    u16 EEPROMlength = 1024;
#endif
#ifndef SERIAL_PORT_HARDWARE
    u8 SERIAL_PORT_HARDWARE = 0;
#endif
unsigned int logging_address = EEPROMlength - sizeof( boolean );
unsigned int upper_furnace_temp_address = logging_address - sizeof( short );//EEPROMlength - 2;
unsigned int lower_furnace_temp_address = upper_furnace_temp_address - sizeof( short );//EEPROMlength - 3;
unsigned int logging_temp_changes_address = lower_furnace_temp_address - sizeof( boolean );//EEPROMlength - 4;
unsigned int upper_cool_temp_address = logging_temp_changes_address - sizeof( short );
unsigned int lower_cool_temp_address = upper_cool_temp_address - sizeof( short );

// So we don't assume every board has exactly three
//in case of boards that have more than 24 pins ( three virtual 8-bit ports )
//int number_of_ports_that_present_pins = EEPROM.read( EEPROMlength - 5 );  // may never need this

// main temperature sensor, furnace, auxiliary furnace device, system power cycle, porch light, dining room coffee outlet
//  The following values need to be stored persistent through power outages.  Sadly, the __LGT8FX8E__ will not read EEPROM until the setup() loop starts executing, so these values get set there for all boards for simplicity
u8 primary_temp_sensor_pin;// = EEPROM.read( primary_temp_sensor_address ); 
u8 secondary_temp_sensor_pin;// = EEPROM.read( secondary_temp_sensor_address ); 
u8 furnace_pin;// = EEPROM.read( furnace_pin_address );
u8 cool_pin;// = EEPROM.read( cool_pin_address );
u8 furnace_fan_pin;// = EEPROM.read( furnace_fan_pin_address );
u8 power_cycle_pin;// = EEPROM.read( power_cycle_address );
u8 outdoor_temp_sensor1_pin;
u8 outdoor_temp_sensor2_pin;
boolean logging;// = ( boolean )EEPROM.read( logging_address );
boolean logging_temp_changes;// = ( boolean )EEPROM.read( logging_temp_changes_address );
float lower_furnace_temp_floated;//filled in setup
float upper_furnace_temp_floated;
float upper_cool_temp_floated;
float lower_cool_temp_floated;
char thermostat_mode;// = ( char )EEPROM.read( thermostat_mode_address );
char fan_mode;// = ( char )EEPROM.read( fan_mode_address );//a';//Can be either auto (a) or on (o)

char strFull[ ] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, \
                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float last_three_temps[] = { -100, -101, -102 };
float furnace_started_temp_x_3 = last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ];
u8 pin_specified;
float temp_specified_floated;
boolean fresh_powerup = true;
long unsigned timer_alert_furnace_sent = 0;
const u8 factory_setting_primary_temp_sensor_pin PROGMEM = 2;
const u8 PROGMEM factory_setting_furnace_pin PROGMEM = 3;
const u8 PROGMEM factory_setting_furnace_fan_pin PROGMEM = 4;
const u8 PROGMEM factory_setting_power_cycle_pin PROGMEM = 5;
const u8 PROGMEM factory_setting_cool_pin PROGMEM = 9;
const bool PROGMEM factory_setting_logging_setting PROGMEM = true;
const bool PROGMEM factory_setting_logging_temp_changes_setting PROGMEM = true;
#ifndef __LGT8FX8E__
const char PROGMEM factory_setting_thermostat_mode PROGMEM = 'a';
#else
const char PROGMEM factory_setting_thermostat_mode PROGMEM = 'h';
#endif
const char factory_setting_fan_mode PROGMEM = 'a';
const float factory_setting_lower_furnace_temp_floated PROGMEM = 21.3;
const float factory_setting_upper_furnace_temp_floated PROGMEM = 22.4;
const float factory_setting_lower_cool_temp_floated PROGMEM = 22.4;
const float factory_setting_upper_cool_temp_floated PROGMEM = 22.7;
const u8 factory_setting_secondary_temp_sensor_pin PROGMEM = 8;
const u8 factory_setting_outdoor_temp_sensor1_pin PROGMEM = 10;
const u8 factory_setting_outdoor_temp_sensor2_pin PROGMEM = 11;
const float minutes_furnace_should_be_effective_after PROGMEM = 5.5; //Can be decimal this way
const unsigned long loop_cycles_to_skip_between_alert_outputs PROGMEM = 5 * 60 * 30;//estimating 5 loops per second, 60 seconds per minute, 30 minutes per alert

const char str_furnaceStartLowTemp[] PROGMEM = "furnace start low temp";
const char str_furnaceStopHighTemp[] PROGMEM = "furnace stop high temp";
const char str_coolStopLowTemp[] PROGMEM = "cool stop low temp";
const char str_coolStartHighTemp[] PROGMEM = "cool start high temp";
const char str_talkback[] PROGMEM = "talkback";
const char str_logging[] PROGMEM = "logging";
const char str_pin_set[] PROGMEM = "pin set";
const char str_set_pin[] PROGMEM = "set pin";
const char str_pins_read[] PROGMEM = "pins read";
const char str_read_pins[] PROGMEM = "read pins";
const char str_read_pin_dot[] PROGMEM = "read pin .";
const char str_set_pin_to[] PROGMEM = "set pin to";
const char str_pin_set_to[] PROGMEM = "pin set to";
const char str_view[] PROGMEM = "view";
const char str_sensor[] PROGMEM = "sensor";
const char str_persistent_memory[] PROGMEM = "persistent memory";
const char str_changes[] PROGMEM = "changes";
const char str_change[] PROGMEM = "change";
const char str_thermostat[] PROGMEM = "thermostat";
const char str_fan_auto[] PROGMEM = "fan auto";
const char str_fan_on[] PROGMEM = "fan on";
const char str_factory[] PROGMEM = "factory";
const char str_power_cycle[] PROGMEM = "power cycle";
const char str_cycle_power[] PROGMEM = "cycle power";
const char str_report_master_room_temp[] PROGMEM = "report master room temp";
const char str_read_pin[] PROGMEM = "read pin";
const char str_pin_read[] PROGMEM = "pin read";
const char str_set_pin_high[] PROGMEM = "set pin high";
const char str_set_pin_low[] PROGMEM = "set pin low";
const char str_set_pin_output[] PROGMEM = "set pin output";
const char str_set_pin_input_with_pullup[] PROGMEM = "set pin input with pullup";
const char str_set_pin_input[] PROGMEM = "set pin input";
const char str_ther_a[] PROGMEM = "ther a";
const char str_ther_o[] PROGMEM = "ther o";
const char str_ther_h[] PROGMEM = "ther h";
const char str_ther_c[] PROGMEM = "ther c";
const char str_auto[] PROGMEM = "auto";
const char str_off[] PROGMEM = "off";
const char str_heat[] PROGMEM = "heat";
const char str_cool[] PROGMEM = "cool";
const char str_ther_[] PROGMEM = "ther ";
const char str_ther[] PROGMEM = "ther";
const char str_fan_a[] PROGMEM = "fan a";
const char str_fan_o[] PROGMEM = "fan o";
const char str_fan_[] PROGMEM = "fan ";
const char str_fan[] PROGMEM = "fan";
const char str_logging_temp_ch_o[] PROGMEM = "logging temp ch o";
const char str_logging_temp_ch[] PROGMEM = "logging temp ch";
const char str_logging_o[] PROGMEM = "logging o";
const char str_vi_pers[] PROGMEM = "vi pers";
const char str_ch_pers[] PROGMEM = "ch pers";
const char str_vi_fact[] PROGMEM = "vi fact";
const char str_reset[] PROGMEM = "reset";
const char str_test_alert[] PROGMEM = "test alert";
const char str_sens_read[] PROGMEM = "sens read";
const char str_read_sens[] PROGMEM = "read sens";
const char str_help[] PROGMEM = "help";
char *string_to_match;
bool furnace_state = false;
bool cool_state = false;
long unsigned check_furnace_effectiveness_time = 0;
bool millis_overflowed = false;

void printThermoModeWord( char setting_char, bool newline )
{
    const char *setting PROGMEM;
    if( setting_char == 'a' )
        setting = str_auto;
    else if( setting_char == 'o' )
        setting = str_off;
    else if( setting_char == 'h' )
        setting = str_heat;
    else if( setting_char == 'c' )
        setting = str_cool;
//    else return false;
//    setting = str_auto;
    Serial.print( (const __FlashStringHelper *)setting );
    if( newline ) Serial.println();
}

void printThermoModeWord( long unsigned var_with_setting, bool newline )
{
    if( thermostat_mode == 'a' )
        var_with_setting = ( long unsigned )str_auto;
    else if( thermostat_mode == 'o' )
        var_with_setting = ( long unsigned )str_off;
    else if( thermostat_mode == 'h' )
        var_with_setting = ( long unsigned )str_heat;
    else if( thermostat_mode == 'c' )
        var_with_setting = ( long unsigned )str_cool;
//    else return false;
    Serial.print( (const __FlashStringHelper *)var_with_setting );
    if( newline ) Serial.println();
}

void refusedNo_exclamation() //putting this in a function for just 2 calls saves 64 bytes due to short memory in WeMo/TTGO XI
{
     if( logging )
     {
        Serial.print( F( "Without appending a '!' pin " ) );
        Serial.print( pin_specified );
        Serial.println( F( " is reserved" ) );
     }
}

void setFurnaceEffectivenessTime()
{
    check_furnace_effectiveness_time = millis() + ( minutes_furnace_should_be_effective_after * 60000 );  //Will this become invalid if furnace temp setting gets adjusted?  TODO:  Account for that condition
    if( !check_furnace_effectiveness_time ) check_furnace_effectiveness_time = 1;
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

void IfReservedPinGettingForced( bool level )
{
    if( pin_specified == furnace_fan_pin )
    {
        if( (bool )level ) fan_mode = 'a'; //high; low = 'a'
        else fan_mode = 'o'; //high; low = 'a'
#ifndef __LGT8FX8E__
           EEPROM.update( fan_mode_address, fan_mode ); //high; low = 'a'
#else
           EEPROM.write( fan_mode_address, fan_mode ); //high; low = 'a'
#endif
    }
    else if( pin_specified == furnace_pin )
    {
        timer_alert_furnace_sent = 0;
        if( !level )
        {
            check_furnace_effectiveness_time = 0;
        }
        else
        {
            setFurnaceEffectivenessTime();
        }
        furnace_state = ( bool ) level; //high = true; low = false
    }
    else if( pin_specified == cool_pin ) cool_state = ( bool ) level; //high; = true low = false
}

bool refuseInput()
{
    if( pin_specified == power_cycle_pin || pin_specified == furnace_fan_pin || pin_specified == furnace_pin || pin_specified == cool_pin )
    {
         if( logging )
         {
                if( pin_specified == power_cycle_pin ) Serial.print( F( "Power cycling the to the host system" ) );
                if( pin_specified == furnace_fan_pin ) Serial.print( F( "Furnace blower fan" ) );
                if( pin_specified == furnace_pin ) Serial.print( F( "Furnace" ) );
                if( pin_specified == cool_pin ) Serial.print( F( "A/C" ) );
                Serial.print( F( ", pin " ) );
                Serial.print( pin_specified );
                Serial.println( F( " skipped" ) );
         }
         return true;
    }
    return false;
}

bool pin_print_and_not_sensor( bool setting )
{
    if( ( pin_specified == primary_temp_sensor_pin || pin_specified == secondary_temp_sensor_pin ) || ( pin_specified ==  outdoor_temp_sensor1_pin && DHTfunctionResultsArray[ outdoor_temp_sensor1_pin ].ErrorCode == DEVICE_READ_SUCCESS ) || ( pin_specified ==  outdoor_temp_sensor2_pin && DHTfunctionResultsArray[ outdoor_temp_sensor2_pin ].ErrorCode == DEVICE_READ_SUCCESS ) )
    {
        if( pin_specified == primary_temp_sensor_pin ) Serial.print( F( "Indoor prim" ) );
        else if( pin_specified == secondary_temp_sensor_pin ) Serial.print( F( "Indoor second" ) );
        else if( pin_specified == outdoor_temp_sensor1_pin ) Serial.print( F( "Outdoor prim" ) );
        else if( pin_specified == outdoor_temp_sensor2_pin ) Serial.print( F( "Outdoor second" ) );
        Serial.print( F( "ary temperature sensor, pin " ) );
        return( false );
    }
    if( setting ) Serial.print( F( "time_stamp_this " ) );//only do if returning true
    Serial.print( F( "Pin " ) );
    Serial.print( pin_specified );
    if( setting ) Serial.print( F( " now set to " ) );
}

void illegal_attempt_SERIAL_PORT_HARDWARE()
{
    Serial.print( F( "Communications Rx from the host, pin " ) );
    Serial.print( SERIAL_PORT_HARDWARE );
    Serial.println( F( " skipped" ) );
}

boolean IsValidPinNumber( const char* str )
{
    u8 i = 0;
    while( str[ i ] == 32 ) i++;
    if( str[ i ] == '.' && ( str[ i + 1 ] == 0 || str[ i + 1 ] == 32 || ( ( str[ i + 1 ] == '+' || str[ i + 1 ] == '-' ) && ( str[ i + 2 ] == 0 || str[ i + 2 ] == 32 ) ) || ( str[ i + 1 ] == '!' && ( str[ i + 2 ] == 0 || str[ i + 2 ] == 32 ) ) ||  ( ( str[ i + 1 ] == '+' || str[ i + 1 ] == '-' ) && str[ i + 2 ] == '!' && ( str[ i + 3 ] == 0 || str[ i + 3 ] == 32 ) ) ) )
    {
        pin_specified = 0;
        return true;
    }
    u8 j = i;
    while( isdigit( str[ j ] ) ) j++;
    if( j == i )
    {
        Serial.println( F( "Command must begin or end with a pin number as specified in help screen" ) );
        return false;
    }
    pin_specified = ( u8 )atoi( str );
    if( pin_specified < 0 || pin_specified >= NUM_DIGITAL_PINS )
    {
        Serial.print( F( "Pin number must be 0 through " ) );
        Serial.println( NUM_DIGITAL_PINS - 1 );
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
    Serial.print( F( "Sorry, pin " ) );
    Serial.print( pin );
    Serial.println( F( " is not yet set to output" ) );
}
return false;
}

boolean IsValidTemp( const char* str, bool furnace )
{
//    for( unsigned int i = 0; i < str->length(); i++ )
    for( unsigned int i = 0; i < strlen( str ); i++ )
    {
        if( !( ( isdigit( str[ i ] ) || str[ i ] == '.' ) && !( !i && str[ 0 ] == '-' ) ) || ( strchr( str, '.' ) != strrchr( str, '.' ) ) ) //allow one and only one decimal point in str
        {
            Serial.println( F( "Command must end with a temperature in Celsius" ) );
            return false;
        }
    }
    temp_specified_floated = ( float )atoi( str );
    if( strchr( str, '.' ) ){ str = strchr( str, '.' ) + 1; temp_specified_floated += ( ( float )atoi( str ) ) / 10; };
//    Serial.print( F( "Temperature entered:" ) );
//    Serial.print( temp_specified_floated );
//    Serial.print( F( ", decimal portion of Temperature entered:" ) );
//    Serial.println( str );

    signed char upper_limit = 27;  //aspplicable for furnace only
    signed char lower_limit = 10;  //aspplicable for furnace only
//    signed char lower_cool_limit = 19;  set elsewhere
    if( furnace && ( temp_specified_floated < lower_limit || temp_specified_floated > upper_limit ) )//This check should be called for the rooms where it matters
    {
        Serial.print( F( "Temperature must be Celsius " ) );
        Serial.print( lower_limit );
        Serial.print( F( " through " ) );
        Serial.println( upper_limit );
        return false;
    }
    return true;
}

void printBasicInfo()
{
    Serial.println( F( ".." ) );
    if( fresh_powerup )
    {
       Serial.println( F( "A way to save talkbacks into a file in Ubuntu and Mint Linux is: (except change \"TIME_STAMP_THIS\" to lower case, not shown so this won't get filtered in by such command)" ) );
       Serial.print( F( "    nohup stty -F igcr \$(ls /dev/ttyA* /dev/ttyU* 2>/dev/null|tail -n1) " ) );
       Serial.print( _baud_rate_ );
//The following would get time stamped inadvertently:
//       Serial.println( F( " -echo;while true;do cat \$(ls /dev/ttyA* /dev/ttyU* 2>/dev/null|tail -n1)|while IFS= read -r line;do if ! [[ -z \"\$line\" ]];then echo \"\$line\"|sed \'s/\^time_stamp_this/\'\"\$(date )\"\'/g\';fi;done;done >> /log_directory/arduino.log 2>/dev/null &" ) );
//So we have to capitalize time_stamp_this
       Serial.println( F( " -echo;while true;do cat \$(ls /dev/ttyA* /dev/ttyU* 2>/dev/null|tail -n1)|while IFS= read -r line;do if ! [[ -z \"\$line\" ]];then echo \"\$line\"|sed \'s/\^TIME_STAMP_THIS/\'\"\$(date )\"\'/g\';fi;done;done >> /log_directory/arduino.log 2>/dev/null &" ) );
       Serial.println( F( "." ) );
       Serial.println( F( "time_stamp_this New power up:" ) );
    }
    Serial.print( F( "Version: " ) );
    Serial.println( F( VERSION ) );
    Serial.print( F( "Operating mode (heat/cool/auto/off)=" ) );
    printThermoModeWord( thermostat_mode, true );
    Serial.print( F( "Fan mode=" ) );
    if( fan_mode == 'a' ) Serial.println( F( "auto" ) );
    else if( fan_mode == 'o' ) Serial.println( F( "on" ) );
    Serial.print( (const __FlashStringHelper *)str_furnaceStartLowTemp );
    Serial.print( '=' );
    Serial.println( lower_furnace_temp_floated, 1 );
    Serial.print( (const __FlashStringHelper *)str_furnaceStopHighTemp );
    Serial.print( '=' );
    Serial.println( upper_furnace_temp_floated, 1 );
    Serial.print( (const __FlashStringHelper *)str_coolStopLowTemp );
    Serial.print( '=' );
    Serial.println( lower_cool_temp_floated, 1 );
    Serial.print( (const __FlashStringHelper *)str_coolStartHighTemp );
    Serial.print( '=' );
    Serial.println( upper_cool_temp_floated, 1 );
    Serial.print( F( "Talkback for logging is turned o" ) );
    if( logging ) Serial.println( F( "n" ) );
    else  Serial.println( F( "ff" ) );
    Serial.print( F( "Talkback for logging temp changes is turned o" ) );
    if( logging_temp_changes ) Serial.println( F( "n" ) );
    else  Serial.println( F( "ff" ) );
    Serial.print( F( "primary_, secondary_temp_sensor_pins=" ) );
    Serial.print( primary_temp_sensor_pin );
    Serial.print( F( ", " ) );
    Serial.println( secondary_temp_sensor_pin );
    Serial.print( F( "outdoor_temp_sensor1_, _2_pins=" ) );
    Serial.print( outdoor_temp_sensor1_pin );
    Serial.print( F( ", " ) );
    Serial.println( outdoor_temp_sensor2_pin );
    Serial.print( F( "furnace_, furnace_fan, cool_pins=" ) );
    Serial.print( furnace_pin );
    Serial.print( F( ", " ) );
    Serial.print( furnace_fan_pin );
    Serial.print( F( ", " ) );
    Serial.println( cool_pin );
    Serial.print( F( "host/aux system power_cycle_pin=" ) );
    Serial.println( power_cycle_pin );
    Serial.print( F( "LED_BUILTIN pin=" ) );
    Serial.println( LED_BUILTIN );
    Serial.println( F( "." ) );
    Serial.println( F( "Pin numbers may otherwise be period (all pins) with +/-/! for setting and forcing reserved pins" ) );
    Serial.println( F( "Example: pin set to output .-! (results in all pins [.] being set to output with low logic level [-], even reserved pins [!])" ) );
    Serial.println( F( "Valid commands (cAsE sEnSiTiVe, minimal sanity checking, one per line) are:" ) );
    Serial.println( F( "." ) );
    Serial.println( F( "help (re-display this information)" ) );
    Serial.println( F( "ther[mostat][ a[uto]/ o[ff]/ h[eat]/ c[ool]] (to read or set thermostat mode)" ) );//, auto requires outdoor sensor[s] and reverts to heat if sensor[s] fail)" ) );
    Serial.println( F( "fan[ a[uto]/ o[n]] (to read or set fan)" ) );
    Serial.println( F( "furnace start low temp <°C> (to turn furnace on at this or lower temperature, always persistent)" ) );//not worth converting for some unkown odd reason
    Serial.println( F( "furnace stop high temp <°C> (to turn furnace off at this or higher temperature, always persistent)" ) );
    Serial.println( F( "cool stop low temp <°C> (to turn A/C off at this or lower temperature, always persistent)" ) );
    Serial.println( F( "cool start high temp <°C> (to turn A/C on at this or higher temperature, always persistent)" ) );

    Serial.println( F( "talkback[ on/off] (or logging on/off)" ) );//(for the host system to log when each output pin gets set high or low, always persistent)" ) );
    Serial.println( F( "talkback temp change[s[ on/off]] (or logging temp changes[ on/off])(requires normal talkback on)" ) );// - for the host system to log whenever the main room temperature changes, always persistent)" ) );
    Serial.println( (const __FlashStringHelper *)str_report_master_room_temp );
    Serial.println( F( "power cycle (or cycle power)" ) );

    Serial.println( F( "read pin <pin number> (or ...pin read...)(obtain the name if any, setting and voltage)" ) );
    Serial.println( F( "read pins (or pins read )(obtain the names, settings and voltages of ALL pins)" ) );
    Serial.println( F( "read sens[or] <pin number> (retrieves sensor reading, a period with due care in place of pin number for all pins)" ) );
    Serial.println( F( "set pin [to] output <pin number> (or ...pin set)" ) );
    Serial.println( F( "set pin [to] input <pin number> [pers] (or ...pin set...) optional persistence FUTURE" ) );
    Serial.println( F( "set pin [to] input with pullup <pin number> [pers] (or ...pin set...) optional persistence FUTURE" ) );
    Serial.println( F( "set pin [to] low <pin number> [pers] (or ...pin set...)(only allowed to pins assigned as output) optional persistence FUTURE" ) );
    Serial.println( F( "set pin [to] high <pin number> [pers] (or ...pin set...)(only allowed to pins assigned as output) optional persistence FUTURE" ) );

    Serial.println( F( "ch[ange] pers[istent memory] <address> <value> (changes EEPROM, see source code for addresses of data)" ) );
    Serial.println( F( "ch[ange] pers[istent memory] <StartingAddress> \"<character string>[\"[ 0]] (store character string in EEPROM as long as desired, optional null-terminated. Reminder: echo -e and escape the quote[s])" ) );
    Serial.println( F( "vi[ew] pers[istent memory] <StartingAddress>[ <EndingAddress>] (views EEPROM)" ) );
    Serial.println( F( "vi[ew] fact[ory defaults] (so you can see what would happen before you reset to them)" ) );//Wemo XI does not have enough memory for this
    Serial.println( F( "reset (factory defaults: pure, simple and absolute)" ) );
    Serial.println( F( "test alert (sends an alert message to host for testing purposes)" ) );
    Serial.println( F( ".." ) );
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
    outdoor_temp_sensor1_pin = factory_setting_outdoor_temp_sensor1_pin;
    outdoor_temp_sensor2_pin = factory_setting_outdoor_temp_sensor2_pin;
    thermostat_mode = factory_setting_thermostat_mode;
    timer_alert_furnace_sent = 0;
    fan_mode = factory_setting_fan_mode;
    
    lower_furnace_temp_floated = factory_setting_lower_furnace_temp_floated;
    upper_furnace_temp_floated = factory_setting_upper_furnace_temp_floated;
    upper_cool_temp_floated = factory_setting_upper_cool_temp_floated;    
    lower_cool_temp_floated = factory_setting_lower_cool_temp_floated;    
    if( fan_mode == 'o' ) digitalWrite( furnace_fan_pin, HIGH );
    else digitalWrite( furnace_fan_pin, LOW );
    if( thermostat_mode == 'o' || thermostat_mode == 'c' )
    {
        digitalWrite( furnace_pin, LOW );
        furnace_state = false;
    }
    if( thermostat_mode == 'h' || thermostat_mode == 'o' ) 
    {
        digitalWrite( cool_pin, LOW );
        cool_state = false;
    }
    Serial.print( F( "Storing thermostat-related.  Primary temperature sensor goes on pin " ) );
    Serial.print( factory_setting_primary_temp_sensor_pin );
    Serial.print( F( ", secondary sensor on pin " ) );
    Serial.print( factory_setting_secondary_temp_sensor_pin );
    Serial.print( F( ", furnace controlled by pin " ) );
    Serial.print( factory_setting_furnace_pin );
    Serial.print( F( ", furnace fan controlled by pin " ) );
    Serial.print( factory_setting_furnace_fan_pin );
    Serial.print( F( ", cool controlled by pin " ) );
    Serial.print( factory_setting_cool_pin );
    Serial.print( F( ", outdoor sensor 1 on pin " ) );
    Serial.print( factory_setting_outdoor_temp_sensor1_pin );
    Serial.print( F( ", outdoor sensor 2 on pin " ) );
    Serial.print( factory_setting_outdoor_temp_sensor2_pin );
    Serial.print( F( ", power cycle of automation system or whatnot controlled by pin " ) );
    Serial.print( factory_setting_power_cycle_pin );
    Serial.print( F( ", logging o" ) );
    if( factory_setting_logging_setting ) Serial.print( F( "n" ) );
    else Serial.print( F( "ff" ) );
    Serial.print( F( ", logging temp changes o" ) );
    if( factory_setting_logging_temp_changes_setting ) Serial.print( F( "n, " ) );
    else Serial.print( F( "ff, " ) );
    Serial.print( (const __FlashStringHelper *)str_furnaceStartLowTemp );
    Serial.print( '=' );
    Serial.print( factory_setting_lower_furnace_temp_floated, 1 );
    Serial.print( F( ", furnace stop high temp=" ) );
    Serial.print( factory_setting_upper_furnace_temp_floated, 1 );
    Serial.print( F( ", cool stop low temp=" ) );
    Serial.print( factory_setting_lower_cool_temp_floated, 1 );
    Serial.print( F( ", cool start high temp=" ) );
    Serial.print( factory_setting_upper_cool_temp_floated, 1 );
    Serial.print( F( ", furnace mode=" ) );
    printThermoModeWord( factory_setting_thermostat_mode, true );
    Serial.print( F( ", fan mode=" ) );
    Serial.print( factory_setting_fan_mode );
    
/*
    if( factory_setting_thermostat_mode == 'a' )
        Serial.println( F( "auto" ) );
    else if( factory_setting_thermostat_mode == 'o' )
        Serial.println( F( "off" ) );
    else if( factory_setting_thermostat_mode == 'h' )
        Serial.println( F( "heat" ) );
    else if( factory_setting_thermostat_mode == 'c' )
        Serial.println( F( "cool" ) );
*/
#ifndef __LGT8FX8E__
    EEPROM.update( primary_temp_sensor_address, factory_setting_primary_temp_sensor_pin );//2 );
#else
    EEPROM.write( primary_temp_sensor_address, factory_setting_primary_temp_sensor_pin );//2 );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( furnace_pin_address, factory_setting_furnace_pin );//3 );
#else
    EEPROM.write( furnace_pin_address, factory_setting_furnace_pin );//3 );
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
    EEPROM.update( thermostat_mode_address, factory_setting_thermostat_mode );//'a' );
#else
    EEPROM.write( thermostat_mode_address, factory_setting_thermostat_mode );//'a' );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( fan_mode_address, factory_setting_fan_mode );//'a' );
#else
    EEPROM.write( fan_mode_address, factory_setting_fan_mode );//'a' );
#endif
//These two must be put's because their data types are longer than one.  Data types of length one can be either updates or puts.
    short factory_setting_lower_furnace_temp_shorted_times_ten = ( short )( factory_setting_lower_furnace_temp_floated * 10 );
    short factory_setting_upper_furnace_temp_shorted_times_ten = ( short )( factory_setting_upper_furnace_temp_floated * 10 );
    short factory_setting_lower_cool_temp_shorted_times_ten = ( short )( factory_setting_lower_cool_temp_floated * 10 );
    short factory_setting_upper_cool_temp_shorted_times_ten = ( short )( factory_setting_upper_cool_temp_floated * 10 );
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
    EEPROM.put( lower_cool_temp_address, factory_setting_lower_cool_temp_shorted_times_ten );//21 );
#else
    EEPROM.write( lower_cool_temp_address, ( u8 )factory_setting_lower_cool_temp_shorted_times_ten );
    EEPROM.write( lower_cool_temp_address + 1, ( u8 )( factory_setting_lower_cool_temp_shorted_times_ten >> 8 ) );
#endif

#ifndef __LGT8FX8E__
    EEPROM.update( secondary_temp_sensor_address, factory_setting_secondary_temp_sensor_pin );//2 );
#else
    EEPROM.write( secondary_temp_sensor_address, factory_setting_secondary_temp_sensor_pin );//2 );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( outdoor_temp_sensor1_address, factory_setting_outdoor_temp_sensor1_pin );//2 );
#else
    EEPROM.write( outdoor_temp_sensor1_address, factory_setting_outdoor_temp_sensor1_pin );//2 );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( outdoor_temp_sensor2_address, factory_setting_outdoor_temp_sensor2_pin );//2 );
#else
    EEPROM.write( outdoor_temp_sensor2_address, factory_setting_outdoor_temp_sensor2_pin );//2 );
#endif
#ifndef __LGT8FX8E__
    EEPROM.put( 0, ( NUM_DIGITAL_PINS + 1 ) * 3 );//Tattoo the board
#else
    EEPROM.write( 0, ( u8 )( ( NUM_DIGITAL_PINS + 1 ) * 3 ) );
    EEPROM.write( 1, ( u8 )( ( ( NUM_DIGITAL_PINS + 1 ) * 3 ) >> 8 ) );
#endif
    Serial.println( F( "Done. Allowing you time to unplug the Arduino if desired." ) );
    delay( 10000 );
      printBasicInfo();
}

void print_factory_defaults()
{
    Serial.print( F( "Version: " ) );
    Serial.print( F( VERSION ) );
    Serial.println( F( " Factory defaults:" ) );
    Serial.print( F( "Operating mode (heat/cool/auto/off)=" ) );
    printThermoModeWord( factory_setting_thermostat_mode, true );
/*
    if( factory_setting_thermostat_mode == 'a' )
        Serial.println( F( "auto" ) );
    else if( factory_setting_thermostat_mode == 'o' )
        Serial.println( F( "off" ) );
    else if( factory_setting_thermostat_mode == 'h' )
        Serial.println( F( "heat" ) );
    else if( factory_setting_thermostat_mode == 'c' )
        Serial.println( F( "cool" ) );
*/
    Serial.print( F( "Fan mode=" ) );
    if( factory_setting_fan_mode == 'a' ) Serial.println( F( "auto" ) );
    else if( factory_setting_fan_mode == 'o' ) Serial.println( F( "on" ) );
    Serial.print( ( const __FlashStringHelper * )str_furnaceStartLowTemp );
    Serial.print( '=' );
    Serial.println( factory_setting_lower_furnace_temp_floated, 1 );
    Serial.print( ( const __FlashStringHelper * )str_furnaceStopHighTemp );
    Serial.print( '=' );
    Serial.println( factory_setting_upper_furnace_temp_floated, 1 );
    Serial.print( ( const __FlashStringHelper * )str_coolStopLowTemp );
    Serial.print( '=' );
    Serial.println( factory_setting_lower_cool_temp_floated, 1 );
    Serial.print( ( const __FlashStringHelper * )str_coolStartHighTemp );
    Serial.print( '=' );
    Serial.println( factory_setting_upper_cool_temp_floated, 1 );
    Serial.print( F( "Talkback for logging o" ) );
    if( factory_setting_logging_setting ) Serial.println( F( "n" ) );
    else   Serial.println( F( "ff" ) );
    Serial.print( F( "Talkback for logging temp changes is turned o" ) );
    if( factory_setting_logging_temp_changes_setting ) Serial.println( F( "n" ) );
    else  Serial.println( F( "ff" ) );
    Serial.println();
    Serial.print( F( "primary_temp_sensor_pin=" ) );
    Serial.println( factory_setting_primary_temp_sensor_pin );
    Serial.print( F( "secondary_temp_sensor_pin=" ) );
    Serial.println( factory_setting_secondary_temp_sensor_pin );
    Serial.print( F( "outdoor_temp_sensor1_pin=" ) );
    Serial.println( factory_setting_outdoor_temp_sensor1_pin );
    Serial.print( F( "outdoor_temp_sensor_pin=" ) );
    Serial.println( factory_setting_outdoor_temp_sensor2_pin );
    Serial.print( F( "furnace_pin=" ) );
    Serial.println( factory_setting_furnace_pin );
    Serial.print( F( "furnace_fan_pin=" ) );
    Serial.println( factory_setting_furnace_fan_pin );
    Serial.print( F( "cool_pin=" ) );
    Serial.println( factory_setting_cool_pin );
    Serial.print( F( "automation system power_cycle_pin=" ) );
    Serial.println( factory_setting_power_cycle_pin );
    Serial.print( F( "LED_BUILTIN pin=" ) );
    Serial.println( LED_BUILTIN );
}

void setup()
{
    Serial.begin( _baud_rate_ );
    Serial.setTimeout( 10 );
    //read EEPROM addresses 0 (LSB)and 1 (MSB).  This sketch expects MSB combined with LSB always contain ( NUM_DIGITAL_PINS + 1 ) * 3 for confidence in the remiander of the EEPROM's contents.
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
        Serial.println(); 
        Serial.println( F( "Detected first time run so initializing to factory default pin names, power-up states and thermostat assignments.  Please wait..." ) );    
       restore_factory_defaults();
    }
    primary_temp_sensor_pin = EEPROM.read( primary_temp_sensor_address ); 
    secondary_temp_sensor_pin = EEPROM.read( secondary_temp_sensor_address );
    outdoor_temp_sensor1_pin = EEPROM.read( outdoor_temp_sensor1_address );
    outdoor_temp_sensor2_pin = EEPROM.read( outdoor_temp_sensor2_address );
    furnace_pin = EEPROM.read( furnace_pin_address );
    cool_pin = EEPROM.read( cool_pin_address );
    furnace_fan_pin = EEPROM.read( furnace_fan_pin_address );
    power_cycle_pin = EEPROM.read( power_cycle_address );
    logging = ( boolean )EEPROM.read( logging_address );
    logging_temp_changes = ( boolean )EEPROM.read( logging_temp_changes_address );
    thermostat_mode = ( char )EEPROM.read( thermostat_mode_address );
    fan_mode = ( char )EEPROM.read( fan_mode_address );//a';//Can be either auto (a) or on (o)
    pinMode( power_cycle_pin, OUTPUT );
    digitalWrite( power_cycle_pin, LOW );
    pinMode( furnace_pin, OUTPUT );
    digitalWrite( furnace_pin, LOW );
    pinMode( cool_pin, OUTPUT );
    digitalWrite( cool_pin, LOW );
    pinMode( furnace_fan_pin, OUTPUT );
    if( fan_mode == 'o' ) digitalWrite( furnace_fan_pin, HIGH );
    else digitalWrite( furnace_fan_pin, LOW );
    short lower_furnace_temp_shorted_times_ten = 0;
    short upper_furnace_temp_shorted_times_ten = 0;
    short upper_cool_temp_shorted_times_ten = 0;
    short lower_cool_temp_shorted_times_ten = 0;
#ifndef __LGT8FX8E__
    EEPROM.get( lower_furnace_temp_address, lower_furnace_temp_shorted_times_ten );
#else
    lower_furnace_temp_shorted_times_ten = EEPROM.read( lower_furnace_temp_address );
    lower_furnace_temp_shorted_times_ten += ( u16 )( EEPROM.read( lower_furnace_temp_address + 1 ) << 8 );
#endif
    lower_furnace_temp_floated = ( float )( ( float )( lower_furnace_temp_shorted_times_ten ) / 10 );
#ifndef __LGT8FX8E__
    EEPROM.get( upper_furnace_temp_address, upper_furnace_temp_shorted_times_ten );
#else
    upper_furnace_temp_shorted_times_ten = EEPROM.read( upper_furnace_temp_address );
    upper_furnace_temp_shorted_times_ten += ( u16 )( EEPROM.read( upper_furnace_temp_address + 1 ) << 8 );
#endif
    upper_furnace_temp_floated = ( float )( ( float )( upper_furnace_temp_shorted_times_ten ) / 10 );
      delay( 3000 ); // The sensor needs time to initialize, if you have some code before this that make a delay, you can eliminate this delay
      //( pin_specified*3 )and ( pin_specified*3 )+1 contains the EEPROM address where the pin's assigned name is stored.  Pin 0 will always have its name stored at EEPROM address (NUM_DIGITAL_PINS+1 )*3, so address (NUM_DIGITAL_PINS+1 )*3 will always be stored in EEPROM addresses 0 and 1; 0 = ( pin_number*3 )and 1 = (( pin_number*3 )+1 ) )]
      // That will be the way we determine if the EEPROM is configured already or not
      // ( pin_specified*3 )+2 is EEPROM address where the pin's desired inital state is stored
#ifndef __LGT8FX8E__
    EEPROM.get( upper_cool_temp_address, upper_cool_temp_shorted_times_ten );
#else
    upper_cool_temp_shorted_times_ten = EEPROM.read( upper_cool_temp_address );
    upper_cool_temp_shorted_times_ten += ( u16 )( EEPROM.read( upper_cool_temp_address + 1 ) << 8 );
#endif
    upper_cool_temp_floated = ( float )( ( float )( upper_cool_temp_shorted_times_ten ) / 10 );
#ifndef __LGT8FX8E__
    EEPROM.get( lower_cool_temp_address, lower_cool_temp_shorted_times_ten );
#else
    lower_cool_temp_shorted_times_ten = EEPROM.read( lower_cool_temp_address );
    lower_cool_temp_shorted_times_ten += ( u16 )( EEPROM.read( lower_cool_temp_address + 1 ) << 8 );
#endif
    lower_cool_temp_floated = ( float )( ( float )( lower_cool_temp_shorted_times_ten ) / 10 );
    logging = ( boolean )EEPROM.read( logging_address );
}

void check_for_serial_input( char result )
{
        char nextChar;
        nextChar = 0;
        while( Serial.available() )
        {
//            pinMode( LED_BUILTIN, OUTPUT );                // These lines for blinking the LED are here if you want the LED to blink when data is rec'd
//            digitalWrite( LED_BUILTIN, HIGH );             // These lines for blinking the LED are here if you want the LED to blink when data is rec'd
            nextChar = (char)Serial.read();
            if( nextChar != 0xD && nextChar != 0xA )
            {
                strFull[ strlen( strFull ) + 1 ] = 0;
                strFull[ strlen( strFull ) ] = nextChar;
            }
            else
            {
                if( Serial.available() ) Serial.read();
                nextChar = 0;
            }
        }
//        digitalWrite( LED_BUILTIN, LOW );                  // These lines for blinking the LED are here if you want the LED to blink when data is rec'd
        if( strFull[ 0 ] == 0 || nextChar != 0  ) return;       //The way this and while loop is set up allows reception of lines with no endings but at a timing cost of one loop()
        char *hit;
        hit = strstr_P( strFull, string_to_match );
        Serial.println( string_to_match );
        if( hit )
        {
            strcpy( hit, str_logging ); 
            memmove( hit + 7, hit + 8, strlen( hit + 7 ) );
        }
        hit = strstr_P( strFull, str_pin_set );
        if( hit )
        {
            strncpy( hit, str_set_pin, 7 ); 
        }  
        hit = strstr_P( strFull, str_pins_read );
        if( !hit ) hit = strstr_P( strFull, str_read_pins );
        if( hit )
        {
            strncpy( hit, str_read_pin_dot, 11 ); 
        }  
        hit = strstr_P( strFull, str_set_pin_to );
        if( !hit ) hit = strstr_P( strFull, str_pin_set_to );
        if( hit )
        {
            memmove( hit + 7, hit + 10, strlen( hit + 9 ) );
        }
        hit = strstr_P( strFull, str_view );
        if( hit )
        {
            memmove( hit + 2, hit + 4, strlen( hit + 3 ) );
        }
        hit = strstr_P( strFull, str_sensor );
        if( hit )
        {
            memmove( hit + 4, hit + 6, strlen( hit + 5 ) );
        }
        hit = strstr_P( strFull, str_persistent_memory );
        if( hit )
        {
            memmove( hit + 4, hit + 17, strlen( hit + 16 ) );
        }
        hit = strstr_P( strFull, str_changes );
        if( hit )
        {
            memmove( hit + 2, hit + 7, strlen( hit + 6 ) );
        }
        hit = strstr_P( strFull, str_change );
        if( hit )
        {
            memmove( hit + 2, hit + 6, strlen( hit + 5 ) );
        }
        hit = strstr_P( strFull, str_thermostat );
        if( hit )
        {
            memmove( hit + 4, hit + 10, strlen( hit + 9 ) );
        }
        hit = strstr_P( strFull, str_fan_auto );
        if( hit )
        {
            memmove( hit + 5, hit + 8, strlen( hit + 7 ) );
        }
        hit = strstr_P( strFull, str_fan_on );
        if( hit )
        {
            memmove( hit + 5, hit + 6, strlen( hit + 5 ) );
        }
        hit = strstr_P( strFull, str_factory );
        if( hit )
        {
            memmove( hit + 4, hit + 7, strlen( hit + 6 ) );
        }
        char *address_str;
        char *data_str;
        char *number_specified_str;
        number_specified_str = strrchr( strFull, ' ' ) + 1;
        if( strstr_P( strFull, str_power_cycle ) || strstr_P( strFull, str_cycle_power ) ) 
        {
          digitalWrite( power_cycle_pin, HIGH );
          if( logging )
          {
               Serial.print( F( "time_stamp_this powered off, power control pin " ) );
               Serial.print( power_cycle_pin );
               Serial.print( '=' );
               Serial.print( digitalRead( power_cycle_pin ) );
          }
          delay( 1500 );
          digitalWrite( power_cycle_pin, LOW );            
          if( logging )
          {
              Serial.print( F( ", powered back on, power control pin " ) );
              Serial.print( power_cycle_pin );
              Serial.print( '=' );
              Serial.println( digitalRead( power_cycle_pin ) );
          }
          strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_furnaceStartLowTemp ) )//change to furnace low start temp with optional numeric: furnace high stop temp, cool low stop temp, cool high start temp
        {
           if( IsValidTemp( number_specified_str, true ) )
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
                Serial.print( (const __FlashStringHelper *)str_furnaceStopHighTemp );
                Serial.println( F( " too low for this value. Raise that before trying this value" ) );
              }
            }
            if( logging )
            {
                Serial.print( (const __FlashStringHelper *)str_furnaceStartLowTemp );
                Serial.print( F( " now " ) );
                Serial.println( lower_furnace_temp_floated, 1 );
            }
            strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_furnaceStopHighTemp ) )
        {
           if( IsValidTemp( number_specified_str, true ) )
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
                Serial.print( (const __FlashStringHelper *)str_furnaceStartLowTemp );
                Serial.println( F( " too high for this value. Lower that before trying this value" ) );
              }
           }
            if( logging )
            {
                Serial.print( (const __FlashStringHelper *)str_furnaceStopHighTemp );
                Serial.print( F( " now " ) );
                Serial.println( upper_furnace_temp_floated, 1 );
            }
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_coolStopLowTemp ) )
        {
           if( IsValidTemp( number_specified_str, false ) )
           {
              if( temp_specified_floated <= upper_cool_temp_floated )
              {
                lower_cool_temp_floated = temp_specified_floated;
                short temp_specified_shorted_times_ten = ( short )( temp_specified_floated * 10 );
#ifndef __LGT8FX8E__
                EEPROM.put( lower_cool_temp_address, temp_specified_shorted_times_ten );
#else
                EEPROM.write( lower_cool_temp_address, ( u8 )temp_specified_shorted_times_ten );
                EEPROM.write( lower_cool_temp_address + 1, ( u8 )( temp_specified_shorted_times_ten >> 8 ) );
#endif
              }
              else 
              {
                Serial.print( (const __FlashStringHelper *)str_coolStartHighTemp );
                Serial.println( F( " too low for this value. Raise that before trying this value" ) );
              }
           }
            if( logging )
            {
                Serial.print( (const __FlashStringHelper *)str_coolStopLowTemp );
                Serial.print( F( " now " ) );
                Serial.println( lower_cool_temp_floated, 1 );
            }
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_coolStartHighTemp ) )
        {
           if( IsValidTemp( number_specified_str, false ) )
           {
              if( temp_specified_floated >= lower_cool_temp_floated )
              {
                upper_cool_temp_floated = temp_specified_floated;
                short temp_specified_shorted_times_ten = ( short )( temp_specified_floated * 10 );
#ifndef __LGT8FX8E__
                EEPROM.put( upper_cool_temp_address, temp_specified_shorted_times_ten );
#else
                EEPROM.write( upper_cool_temp_address, ( u8 )temp_specified_shorted_times_ten );
                EEPROM.write( upper_cool_temp_address + 1, ( u8 )( temp_specified_shorted_times_ten >> 8 ) );
#endif
              }
              else 
              {
                Serial.print( (const __FlashStringHelper *)str_coolStopLowTemp );
                Serial.println( F( " too high for this value. Lower that before trying this value" ) );
              }
           }
            if( logging )
            {
                Serial.print( (const __FlashStringHelper *)str_coolStartHighTemp );
                Serial.print( F( " now " ) );
                Serial.println( upper_cool_temp_floated, 1 );
            }
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_report_master_room_temp ) )
        {
           if( result == DEVICE_READ_SUCCESS )
           {
               Serial.print( F( "Humidity (%): " ) );
               Serial.println( _HumidityPercent, 1 );
               Serial.print( F( "Temperature (°C): " ) );
               Serial.println( _TemperatureCelsius, 1 );
               Serial.print( F( "Tempperature heat is set to start: " ) );
               Serial.println( lower_furnace_temp_floated, 1 );
               Serial.print( F( "Temperature heat is set to stop: " ) );
               Serial.println( upper_furnace_temp_floated, 1 );
               Serial.print( F( "Temperature cool is set to stop: " ) );
               Serial.println( lower_cool_temp_floated, 1 );
               Serial.print( F( "Temperature cool is set to start: " ) );
               Serial.println( upper_cool_temp_floated, 1 );
               Serial.print( F( "Furnace: " ) );
               Serial.println( digitalRead( furnace_pin ) );
               Serial.print( F( "Furnace fan: " ) );
               Serial.println( digitalRead( furnace_fan_pin ) );
               Serial.print( F( "Cool: " ) );
               Serial.println( digitalRead( cool_pin ) );
           }
           else
           {
                Serial.println( F( "Sensor didn't read" ) );
           }
            strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_read_pin ) || strstr_P( strFull, str_pin_read ) )
        {
           if( IsValidPinNumber( number_specified_str ) )
           {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
               {
                    if( !pin_print_and_not_sensor( false ) )
                    {
                        Serial.print( pin_specified );
                    }
                     if( isanoutput( pin_specified, false ) ) Serial.print( F( ": output & logic " ) );
                     else Serial.print( F( ": input & logic " ) );
                     Serial.println( digitalRead( pin_specified ) );
                    if( *number_specified_str != '.' && !( *number_specified_str == ' ' && *( number_specified_str + 1 ) == '.' ) ) break;
               }
           }
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_set_pin_high ) )
        {
           if( IsValidPinNumber( number_specified_str ) )
           {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
               {
                  if( isanoutput( pin_specified, true ) )
                  {
                        if( ( pin_specified == power_cycle_pin || pin_specified == furnace_fan_pin || pin_specified == furnace_pin || pin_specified == cool_pin ) && number_specified_str[ strlen( number_specified_str ) - 1 ] != '!' )
                            refusedNo_exclamation();
                        else 
                        {
                             digitalWrite( pin_specified, HIGH );
                             IfReservedPinGettingForced( HIGH );
                             if( logging )
                             {
                                if( pin_print_and_not_sensor( true ) ) // && !( pin_specified == power_cycle_pin || pin_specified == furnace_fan_pin || pin_specified == furnace_pin || pin_specified == cool_pin ) )
                                {
                                    Serial.print( F( "logic 1" ) );
                                    u8 pinState = digitalRead( pin_specified );
                                    if( pinState == LOW ) Serial.print( F( ". Pin appears shorted to logic 0 level !" ) );
                                 }
                                else 
                                {
                                    Serial.print( pin_specified );
                                    Serial.print( F( " skipped" ) ); //not to time stamp
                                }
                                Serial.println(); 
                             }
                        }
                  }
                    if( *number_specified_str != '.' && !( *number_specified_str == ' ' && *( number_specified_str + 1 ) == '.' ) ) break;
               }
           }
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_set_pin_low ) )
        {
           if( IsValidPinNumber( number_specified_str ) )
           {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
               {
                  if( isanoutput( pin_specified, true ) )
                  {
                        if( ( pin_specified == power_cycle_pin || pin_specified == furnace_fan_pin || pin_specified == furnace_pin || pin_specified == cool_pin ) && number_specified_str[ strlen( number_specified_str ) - 1 ] != '!' )
                            refusedNo_exclamation();
                        else 
                        {
                            digitalWrite( pin_specified, LOW );
                            IfReservedPinGettingForced( LOW );
                             if( logging )
                             {
                                if( pin_print_and_not_sensor( true ) ) // && !( pin_specified == power_cycle_pin || pin_specified == furnace_fan_pin || pin_specified == furnace_pin || pin_specified == cool_pin ) )
                                {
                                    Serial.print( F( "logic 0" ) );
                                    u8 pinState = digitalRead( pin_specified );
                                    if( pinState == HIGH ) Serial.print( F( ". Pin appears shorted to logic 1 level!" ) );
                                }
                                else 
                                {
                                    Serial.print( pin_specified );
                                    Serial.print( F( " skipped" ) );
                                }
                                Serial.println(); 
                             }
                        }
                  }
                    if( *number_specified_str != '.' && !( *number_specified_str == ' ' && *( number_specified_str + 1 ) == '.' ) ) break;
               }
           }
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_set_pin_output ) )
        {
           if( IsValidPinNumber( number_specified_str ) )
           {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
               {
                    if( pin_specified != SERIAL_PORT_HARDWARE )
                    {
                        if( ( pin_specified == power_cycle_pin || pin_specified == furnace_fan_pin || pin_specified == furnace_pin || pin_specified == cool_pin ) && number_specified_str[ strlen( number_specified_str ) - 1 ] != '!' )
                        {
                            if( logging )
                            {
                                refusedNo_exclamation();
                                goto doneWithPinOutput;
                            }
                        }
                        else
                        {
                              pinMode( pin_specified, OUTPUT );
                              if( *( number_specified_str + 1 ) == '+' || ( *( number_specified_str + 1 ) == '!'  && *( number_specified_str + 2 ) == '+' ) )
                              {
                                    digitalWrite( pin_specified, HIGH );
                                    IfReservedPinGettingForced( HIGH );
                              }
                              else if( *( number_specified_str + 1 ) == '-' || ( *( number_specified_str + 1 ) == '!'  && *( number_specified_str + 2 ) == '-' ) )
                              {
                                digitalWrite( pin_specified, LOW );
                                IfReservedPinGettingForced( LOW );
                              }
                        }
                        if( logging )
                        {
                            if( pin_print_and_not_sensor( true ) )
                            {
                                 Serial.print( F( "output & logic " ) );
                                 Serial.println( digitalRead( pin_specified ) );
                            }
                            else 
                            {
                                Serial.print( pin_specified );
                                Serial.println( F( " skipped" ) );
                            }
                        }
                   }
                   else illegal_attempt_SERIAL_PORT_HARDWARE(); 
doneWithPinOutput:;
                    if( *number_specified_str != '.' && !( *number_specified_str == ' ' && *( number_specified_str + 1 ) == '.' ) ) break;
               }
           }
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_set_pin_input_with_pullup ) )
        {
           if( IsValidPinNumber( number_specified_str ) )
           {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
               {
                    if( pin_specified != SERIAL_PORT_HARDWARE )
                    {
                          if( !refuseInput() )
                          {
                             pinMode( pin_specified, INPUT_PULLUP );
                             if( logging )
                             {
                                if( pin_print_and_not_sensor( true ) ) Serial.print( F( "input with pullup" ) );
                                else 
                                {
                                    Serial.print( pin_specified );
                                    Serial.print( F( " skipped" ) );
                                }
                                Serial.println(); 
                             }
                          }
                    }
                    else illegal_attempt_SERIAL_PORT_HARDWARE();
                    if( *number_specified_str != '.' && !( *number_specified_str == ' ' && *( number_specified_str + 1 ) == '.' ) ) break;
               }
           }
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_set_pin_input ) )
        {
           if( IsValidPinNumber( number_specified_str ) )
           {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
               {
                    if( pin_specified != SERIAL_PORT_HARDWARE )
                    {
                      if( !refuseInput() )
                      {
                         pinMode( pin_specified, OUTPUT );
                         digitalWrite( pin_specified, LOW );
                         pinMode( pin_specified, INPUT );
                        u8 pinState = digitalRead( pin_specified );
                         if( logging )
                         {
                                if( pin_print_and_not_sensor( true ) )
                                {
                                    Serial.print( F( "input" ) );
                                    if( pinState == HIGH ) Serial.print( F( " apparently with pullup because pin shows logic 1 level !" ) );
                                }
                                else
                                {
                                    Serial.print( pin_specified );
                                    Serial.print( F( " skipped" ) );
                                }
                                Serial.println(); 
                         }
                      }
                   }
                   else illegal_attempt_SERIAL_PORT_HARDWARE();
                    if( *number_specified_str != '.' && !( *number_specified_str == ' ' && *( number_specified_str + 1 ) == '.' ) ) break;
               }
           }
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_ther ) )
        {
            if( strlen( strFull ) == 4 ) goto showThermostatSetting;
            if( strFull[ 4 ] != ' ' ) goto notValidCommand;
           if( strFull[ 5 ] == 'a' )
           {
                if( !( DHTfunctionResultsArray[ outdoor_temp_sensor1_pin - 1 ].ErrorCode == DEVICE_READ_SUCCESS ) && !( DHTfunctionResultsArray[ outdoor_temp_sensor2_pin - 1 ].ErrorCode == DEVICE_READ_SUCCESS ) ) 
                {
                    Serial.println( F( "NO CHANGE due to outdoor sensors not reporting." ) );
                    if( outdoor_temp_sensor1_pin || outdoor_temp_sensor2_pin )
                    {
                        Serial.print( F( "To start one send \"sens read " ) );
                        if( outdoor_temp_sensor1_pin )
                        {
                            Serial.print ( outdoor_temp_sensor1_pin );
                            if(outdoor_temp_sensor2_pin )
                            Serial.print( F( "\" or \"sens read " ) );
                            Serial.print ( outdoor_temp_sensor2_pin );
                        }
                        else
                            Serial.print ( outdoor_temp_sensor2_pin );
                        Serial.println( F( "\"" ) );
                        Serial.println( DHTfunctionResultsArray[ outdoor_temp_sensor1_pin - 1 ].ErrorCode );
                        Serial.println( DHTfunctionResultsArray[ outdoor_temp_sensor2_pin - 1 ].ErrorCode );
                    }
                    goto showThermostatSetting;
                }
           }
           if( !( strFull[ 5 ] == 'a' ) && !( strFull[ 5 ] == 'h' ) && !( strFull[ 5 ] == 'c' ) && !( strFull[ 5 ] == 'o' ) ) goto badTherMode;
                Serial.print( F( "Thermostat being set to " ) );
          printThermoModeWord( strFull[ 5 ], true );
          if( thermostat_mode == strFull[ 5 ] );
           if( thermostat_mode == 'o' ) 
           {
                digitalWrite( furnace_pin, LOW );
                furnace_state = false;
                timer_alert_furnace_sent = 0;
                check_furnace_effectiveness_time = 0;
                digitalWrite( cool_pin, LOW );
                cool_state = false;
           }
            timer_alert_furnace_sent = 0;           
#ifndef __LGT8FX8E__
           EEPROM.update( thermostat_mode_address, strFull[ 5 ] );
#else
           EEPROM.write( thermostat_mode_address, strFull[ 5 ] );
#endif
            goto after_change_thermostat;
badTherMode:;
            Serial.println( F( "That space you entered also then requires a valid mode. The only valid characters allowed after that space are the options lower case a, o, h, or c.  They mean auto, off, heat, and cool and may be fully spelled out" ) );
showThermostatSetting:;
            Serial.print( F( "Thermostat is " ) );
            printThermoModeWord( thermostat_mode, true );
after_change_thermostat:
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_fan_a ) || strstr_P( strFull, str_fan_o ) )
        {
           fan_mode = strFull[ ( u8 )( strlen_P( str_fan_ ) ) ];
           Serial.print( F( "Fan being set to " ) );
           if( fan_mode == 'o' )
            {
                Serial.println( F( "on" ) );
                digitalWrite( furnace_fan_pin, HIGH );
            }
           else
           {
                Serial.println( F( "auto" ) );
                digitalWrite( furnace_fan_pin, LOW );
           }
#ifndef __LGT8FX8E__
           EEPROM.update( fan_mode_address, fan_mode );
#else
           EEPROM.write( fan_mode_address, fan_mode );
#endif
after_change_fan:
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_fan ) )
        {
            if( strchr( &strFull[ 3 ], ' ' ) )
            {
#ifndef __LGT8FX8E__
                Serial.println( F( "That space you entered also then requires a valid mode. The only valid characters allowed after that space are the options lower case a or o.  They mean auto and on and optionally may be spelled out completely" ) );
#else
                Serial.println( F( "The only valid characters allowed after that space are the options lower case a or o (auto/on or may be spelled out)" ) );
#endif
            }
            Serial.print( F( "Fan is " ) );
            if( fan_mode == 'a' ) Serial.println( F( "auto" ) );
            else if( fan_mode == 'o' ) Serial.println( F( "on" ) );
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_logging_temp_ch_o ) )
        {
           Serial.print( F( "Talkback for logging temp changes being turned o" ) );
           u8 charat = ( u8 )( strrchr( strFull, ' ' ) + 1 - strFull );
           if( strFull[ charat ] == 'n' ) logging_temp_changes = true;
           else logging_temp_changes = false;
#ifndef __LGT8FX8E__
           EEPROM.update( logging_temp_changes_address, ( uint8_t )logging_temp_changes );
#else
           EEPROM.write( logging_temp_changes_address, ( uint8_t )logging_temp_changes );
#endif
           if( logging_temp_changes ) Serial.println( strFull[ charat ] );
           else  Serial.println( F( "ff" ) );
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_logging_temp_ch ) )
        {
           Serial.print( F( "Talkback for logging temp changes is turned o" ) );
           if( logging_temp_changes ) Serial.println( F( "n" ) );
           else  Serial.println( F( "ff" ) );
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_logging_o ) )
        {
           Serial.print( F( "Talkback for logging being turned o" ) );
           u8 charat = ( u8 )( strrchr( strFull, ' ' ) + 1 - strFull );
           if( strFull[ charat ] == 'n' ) logging = true;
           else logging = false;
#ifndef __LGT8FX8E__
           EEPROM.update( logging_address, ( uint8_t )logging );
#else
           EEPROM.write( logging_address, ( uint8_t )logging );
#endif
           if( logging ) Serial.println( strFull[ charat ] );
           else  Serial.println( F( "ff" ) );
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_logging ) )
        {
           Serial.print( F( "Talkback for logging is turned o" ) );
           if( logging ) Serial.print( F( "n" ) );
           else  Serial.print( F( "ff" ) );
           Serial.print( F( " pers address " ) );
           Serial.print( logging_address );
           Serial.print( F( " shows " ) );
           Serial.println( ( bool )EEPROM.read( logging_address ) );
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_vi_pers ) )
        {
          address_str = strchr( &strFull[ 7 ], ' ' ) + 1;//.substring( address_str.indexOf( ' ' )+ 1 );
          unsigned int address_start = atoi( address_str );
          address_str = strrchr( address_str, ' ' ) + 1;//address_str.substring( address_str.indexOf( ' ' )+ 1 );
          unsigned int address_end = atoi( address_str );//.toInt();
          if( address_start < 0 || address_start >= EEPROMlength || address_end < 0 || address_end >= EEPROMlength )
          {
            Serial.print( F( "Out of range.  Each address can only be 0 to " ) );
            Serial.println( EEPROMlength - 1 );
          }
          else
          {
             Serial.print( F( "Start address: " ) );
             Serial.print( address_start );
             Serial.print( F( " End address: " ) );
             Serial.println( address_end );
             if( address_start > address_end )
             {
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
               Serial.println( ( char )EEPROM.read( address ) );
             }
          }
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_ch_pers ) )
        {
          address_str = strchr( &strFull[ 7 ], ' ' ) + 1;//.substring( address_str.indexOf( ' ' )+ 1 );
          unsigned int address = atoi( address_str );//.toInt();
          data_str = strrchr( address_str, ' ' ) + 1;//address_str.substring( address_str.indexOf( ' ' )+ 1 );
          u8 data = atoi( data_str );//.toInt();
          if( data == 0 && !isdigit( data_str[ 0 ] ) )
          {
              if( strchr( data_str, '"' ) == data_str )//NOT DEBUGGED YET
              {
                unsigned int address_start = address;
                unsigned int address_end = address + strlen( data_str )- 1;//???This is logic from first version.  Lost track of how/if it works
                if( address_end < EEPROMlength )
                {
               for( unsigned int address = address_start; address <= address_end; address++ )
               {
                Serial.println( address_end - address );
                Serial.println( sizeof( data_str ) - ( address_end - address )- 3 );
                 if( data_str[sizeof( data_str ) - ( address_end - address )] != '\"' )
                 {
                    Serial.print( F( "Address: " ) );
                    Serial.print( address );
                    Serial.print( F( " entered data: " ) );
                    Serial.print( data_str[sizeof( data_str ) - ( address_end - address ) - 3 ] );
                    Serial.print( F( " previous data: " ) );
                    Serial.print( EEPROM.read( address ) );
#ifndef __LGT8FX8E__
              EEPROM.update( address, data );
#else
              EEPROM.write( address, data );
#endif
                    Serial.print( F( " newly stored data: " ) );
                    Serial.println( EEPROM.read( address ) );
                  }
                  else
                  {
                    if( address <= address_end )
                    {
                        if( strchr( data_str, '"' ) == data_str + strlen( data_str ) - 2 ) //???This is logic from first version.  Lost rack of how/if it works
                        {
                           Serial.print( F( "Putting a null termination at Address: " ) );
                           Serial.println( ++address );
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
                Serial.println( F( "Data string too long to fit there in EEPROM." ) );
                strFull[ 0 ] = 0;
                return;
                }
                strFull[ 0 ] = 0;
              }
              else
              {
                Serial.println( F( "Invalid data entered" ) );
                strFull[ 0 ] = 0;
                return;
              }
          }
          if( data > 255 || address >= EEPROMlength )
          {
            Serial.print( F( "Address can only be 0 to " ) );
            Serial.print( EEPROMlength - 1 );
            Serial.println( F( ", data can only be 0 to 255" ) );
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
              Serial.println( EEPROM.read( address ) );
          }
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_vi_fact ) ) // The Leonardo does not have enough room in PROGMEM for this feature
        {
#ifdef __AVR_ATmega32U4__
            Serial.println( F( "Not supported for this board due to limited memory space of ATmega32U4-based and ATmega168-based devices" ) );//Don't know how to detect atmega168 boards
#else
            print_factory_defaults();
#endif
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_reset ) )
        {
           restore_factory_defaults();
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_test_alert ) )
        {
           Serial.println( F( "time_stamp_this ALERT test as requested" ) );
           strFull[ 0 ] = 0;
        }
        else if( strstr_P( strFull, str_sens_read ) || strstr_P( strFull, str_read_sens ) )
        {
           int i = 9;
           while( strFull[ i ] == ' ' && strlen( strFull ) > i ) i++;
         if( IsValidPinNumber( &strFull[ i ] ) )
         {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
               {
                 Serial.print( F( "Pin " ) );
                 Serial.print( pin_specified );
                 Serial.print( F( " sensor read: " ) );
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
                 if( noInterrupt_result->Type > 0 && noInterrupt_result->Type <= TYPE_LIKELY_DHT22 ) Serial.print( F( " TYPE_" ) );
                 if( noInterrupt_result->Type == TYPE_KNOWN_DHT11 ) Serial.print( F( "KNOWN_DHT11" ) );
                 else if( noInterrupt_result->Type == TYPE_KNOWN_DHT22 ) Serial.print( F( "KNOWN_DHT22" ) );
                 else if( noInterrupt_result->Type == TYPE_LIKELY_DHT11 ) Serial.print( F( "LIKELY_DHT11" ) );
                 else if( noInterrupt_result->Type == TYPE_LIKELY_DHT22 ) Serial.print( F( "LIKELY_DHT22" ) );
                 Serial.println(); 
                 if( strFull[ i ] != '.' && !( strFull[ i ] == ' ' && strFull[ i + 1 ] == '.' ) ) break;
             }
           }
           strFull[ 0 ] = 0;
        }
        else
        {
          if( !strstr_P( strFull, str_help ) )
          {
notValidCommand:;
                Serial.println();
                 Serial.print( strFull );
                 Serial.println( F( ": not a valid command, note they are cAsE sEnSiTiVe" ) );
                Serial.println(); 
          }
          printBasicInfo();
          strFull[ 0 ] = 0;
        }
}

u8 last_three_temps_index = 0;
float old_getCelsius_temp = 0;
float oldtemp = 0;
unsigned long timeOfLastSensorTimeoutError = 0;
float temp_minimus;

void furnace_on_loop()
{
   if( !furnace_state )
   {
          if( last_three_temps[ 0 ] < lower_furnace_temp_floated && last_three_temps[ 1 ] < lower_furnace_temp_floated && last_three_temps[ 2 ] < lower_furnace_temp_floated && last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ] > lower_furnace_temp_floated )
          {
                digitalWrite( furnace_pin, HIGH ); 
//                digitalWrite( cool_pin, LOW );
              furnace_state = true;
//              cool_state = false;
              timer_alert_furnace_sent = 0;
              if( logging )
              {
                Serial.println( F( "time_stamp_this Furnace on" ) );
              }
              setFurnaceEffectivenessTime();
              temp_minimus = min( last_three_temps[ 0 ], last_three_temps[ 1 ] );//Be sure to set this when furnace is manually operated by pin manipulation or any other means also
              temp_minimus = min( temp_minimus, last_three_temps[ 2 ] );
          }
   }
   else
   {
           if( last_three_temps[ 0 ] > upper_furnace_temp_floated && last_three_temps[ 1 ] > upper_furnace_temp_floated && last_three_temps[ 2 ] > upper_furnace_temp_floated )
           {
                digitalWrite( furnace_pin, LOW );
                  furnace_state = false;
                  timer_alert_furnace_sent = 0;
                  check_furnace_effectiveness_time = 0;
                if( logging )
                {
                    Serial.println( F( "time_stamp_this Furnace off" ) );
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
                Serial.print( F( "time_stamp_this ALERT Furnace not heating enough after allowing " ) );                          //  which is a millis() value to alert at if no heating happened yet
                Serial.print( ( u8 )( minutes_furnace_should_be_effective_after + ( ( millis() - check_furnace_effectiveness_time ) / 60000 ) ) ); //minutes_furnace_should_be_effective_after is a const of a number of minutes
                Serial.println( F( " minutes" ) );
                timer_alert_furnace_sent = loop_cycles_to_skip_between_alert_outputs;
                if( !timer_alert_furnace_sent )
                    timer_alert_furnace_sent = 1;
/*                    send an alert output if furnace not working: temperature dropping even though furnace got turned on.  the way to determine this is to have a flag of whether the alert output is already sent;
*                     That flag will unconditionally get reset in the following situations: thermostat_mode setting gets changed, upper_furnace_temp_floated or lower_furnace_temp_floated gets changed, room temperature rises while furnace is on, 
*/
afterSendAlert:;
       }
       else if( timer_alert_furnace_sent && ( oldtemp - temp_minimus > 2 ) )//enough temperature rise (2C) from temperature minimus which means furnace is assumed working so we say no alert sent
       {
            timer_alert_furnace_sent = 0;
       }
       else if( timer_alert_furnace_sent )
       {
            timer_alert_furnace_sent--;
       }
   }
}

void cool_on_loop()
{
   if( !cool_state )
   {
          if( last_three_temps[ 0 ] > upper_cool_temp_floated && last_three_temps[ 1 ] > upper_cool_temp_floated && last_three_temps[ 2 ] > upper_cool_temp_floated )
          {
//                digitalWrite( furnace_pin, LOW ); 
                digitalWrite( cool_pin, HIGH );
//              furnace_state = false;
              cool_state = true;
//              timer_alert_furnace_sent = 0;
              if( logging )
              {
                Serial.println( F( "time_stamp_this A/C on" ) );
              }
          }
   }
   else
   {
           if( last_three_temps[ 0 ] < lower_cool_temp_floated && last_three_temps[ 1 ] < lower_cool_temp_floated && last_three_temps[ 2 ] < lower_cool_temp_floated )
           {
//                digitalWrite( furnace_pin, LOW );
                digitalWrite( cool_pin, LOW );
                  cool_state = false;
                if( logging )
                {
                    Serial.println( F( "time_stamp_this A/C off" ) );
                }
           }
   }
}

void loop()
{
if( fresh_powerup && logging )
{
    if( Serial )
    {
        printBasicInfo();
        fresh_powerup = false;
    }
}
else fresh_powerup = false;
    DHTresult* noInterrupt_result = ( DHTresult* )( DHTreadWhenRested( primary_temp_sensor_pin ) ); 
    if( noInterrupt_result->ErrorCode != DEVICE_READ_SUCCESS ) noInterrupt_result = ( DHTresult* )( DHTreadWhenRested( secondary_temp_sensor_pin ) );
    if( noInterrupt_result->ErrorCode == DEVICE_READ_SUCCESS )
    {
        timeOfLastSensorTimeoutError = 0;
        if( noInterrupt_result->TemperatureCelsius & 0x8000 ) _TemperatureCelsius = 0 - ( float )( ( float )( noInterrupt_result->TemperatureCelsius & 0x7FFF )/ 10 );
        else _TemperatureCelsius = ( float )( ( float )( noInterrupt_result->TemperatureCelsius & 0x7FFF )/ 10 );
        _HumidityPercent = ( float )( ( float )noInterrupt_result->HumidityPercent / 10 );
        last_three_temps[ last_three_temps_index ] = _TemperatureCelsius;
        float newtemp = ( float )( ( float )( last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ] )/ 3 );
        if( ( newtemp != oldtemp ) && ( last_three_temps[ last_three_temps_index ] != old_getCelsius_temp ) )
        {
            if( logging && logging_temp_changes )
            {
                Serial.print( F( "time_stamp_this Temperature change to " ) );
                Serial.print( ( float )last_three_temps[ last_three_temps_index ], 1 );
                Serial.print( F( " °C " ) );// Leave in celsius for memory savings
                if( noInterrupt_result == &DHTfunctionResultsArray[ primary_temp_sensor_pin - 1 ] ) Serial.print( F( "prim" ) );
                else Serial.print( F( "second" ) );
                Serial.println( F( "ary sensor" ) );
            }
            old_getCelsius_temp = last_three_temps[ last_three_temps_index ];
            if( furnace_state && !timer_alert_furnace_sent && newtemp < oldtemp ) //store temp at every small drop when furnace is calling && !timer_alert_furnace_sent
            {
                  temp_minimus = min( temp_minimus, last_three_temps[ 0 ] ); //Be sure to set this when furnace is manually operated by pin manipulation or any other means also
                  temp_minimus = min( temp_minimus, last_three_temps[ 1 ] );
                  temp_minimus = min( temp_minimus, last_three_temps[ 2 ] );
            }
        }
        oldtemp = newtemp;
        last_three_temps_index = ++last_three_temps_index % 3;

        if( last_three_temps[ 0 ] == -100 || last_three_temps[ 1 ] == -101 || last_three_temps[ 2 ] == -102 )
            ;  //don't do any thermostat function until temps start coming in.  Debatable b/c logically we could do shutoff/reset duties in here instead, but setup() loop did it for us
        else if( thermostat_mode == 'o' )//thermostat_mode == off
        {
            digitalWrite( furnace_pin, LOW ); 
            digitalWrite( cool_pin, LOW );
            furnace_state = false;
            timer_alert_furnace_sent = 0;
            cool_state = false;
            check_furnace_effectiveness_time = 0;
        }
        else if( thermostat_mode == 'a' )
        {
            noInterrupt_result = ( DHTresult* )( DHTreadWhenRested( outdoor_temp_sensor1_pin ) ); 
            if( noInterrupt_result->ErrorCode != DEVICE_READ_SUCCESS ) noInterrupt_result = ( DHTresult* )( DHTreadWhenRested( outdoor_temp_sensor2_pin ) );
            if( noInterrupt_result->ErrorCode == DEVICE_READ_SUCCESS )
            {
                if( noInterrupt_result->TemperatureCelsius & 0x8000 ) O_TemperatureCelsius = 0 - ( float )( ( float )( noInterrupt_result->TemperatureCelsius & 0x7FFF )/ 10 );
                else O_TemperatureCelsius = ( float )( ( float )( noInterrupt_result->TemperatureCelsius & 0x7FFF )/ 10 );
                if( O_TemperatureCelsius <= _TemperatureCelsius ) furnace_on_loop();//get outdoor temp, use second sensor if first fails, get indoor temp same way, if indoor < outdoor cool_on_loop();
                else cool_on_loop();
            }
        }
        else if( thermostat_mode == 'h' ) furnace_on_loop(); //This furnace loop is all that the WeMo/TTGO XI can do as thermostat
        else if( thermostat_mode == 'c' ) cool_on_loop();
    }
    else
    {
        timeOfLastSensorTimeoutError++;
        Serial.print( F( "time_stamp_this " ) );
        if( timeOfLastSensorTimeoutError > 100 ) Serial.print( F( "ALERT " ) );//These ALERT prefixes get added after consecutive 100 timeout fails
        Serial.println( F( "Temperature sensor TIMEOUT error" ) );
    }
    for( u8 i = 0; i < 4; i++ )
    {
        check_for_serial_input( noInterrupt_result->ErrorCode );
        delay( 500 );
    }
}
