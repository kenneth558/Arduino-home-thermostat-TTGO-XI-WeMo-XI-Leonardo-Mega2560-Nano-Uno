/*******************(C)  COPYRIGHT 2018 KENNETH L ANDERSON *********************
*      ARDUINO SINGLE REGULATED SPACE THERMOSTAT 
* File Name          : thermostat.ino
* Author             : KENNETH L ANDERSON
* Version            : v.1.0
* Date               : 26-March-2018
* Description        : Implements residential thermostat functionality on Arduino-compatible hardware platforms having 32K minimum Flash + 64 bytes minimum EEPROM
* Boards tested on   : Uno Mega2560 WeMo XI/TTGO XI Leonardo Nano
* Known limitations  : Only a host serially-connected computer can effect run-time change and/or viewing of thermostat settings ("Here, have yourself a thermostat that is locked from changing if not attached to a host computer")
*                    : No scheduling ability to work around that limitation is integrated
*                    : No time-of-day ability is even vaguely allowed for except if implemented on a host computer
********************************************************************************
* THE PRESENT PRODUCT WHICH IS ONLY FOR USE BY SOFTWARE- AND HARDWARE-COMPETENT USERS
* IS INTENDED TO OFFER VARIOUS CAPABILITIES.  BECAUSE ABSOLUTELY NO CONSIDERATION 
* IS EXPECTED IN RETURN, NO CONTRACT, WHETHER OFFERING SUITABILITY, RELIABILITY, 
* FUNCTIONALITY, SAFETY, OR OTHER, EXISTS.  BY USING THIS PRODUCT YOU EXPRESSLY AND
* TOTALLY ACCEPT FULL LIABILITY FOR ANY DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES 
* WITH RESPECT TO ANY CLAIMS ARISING FROM SOFTWARE AND/OR HARDWARE DERIVATIONS OF PRODUCT.
* I MAKE ABSOLUTELY NO CLAIM OF MY OWN COMPETENCE, LICENSURE, CERTIFICATION, EXPERIENCE,
* OR PROFESSIONAL STANDING OTHER THAN BEING AN ELECTRONICS TECHNICIAN BY FORMAL 
* DEGREE AND UNITED STATES FCC RADIOTELEPHONE OPERATORS LICENSE, AND 
* A SOFTWARE DEVELOPER BY LIMITED PRACTICAL EXPERIENCE
********************************************************************************
*
*     I RECOMMEND WHEN USING A DIGITAL SENSOR ON A PIN THAT YOU ADD 128 TO THE PIN NUMBER WHEN STORING IT IN EEPROM SO IF THE DIGITAL SENSOR FAILS THE SKETCH WILL NOT REVERT TO READ AN INVALID ANALOG VALUE FROM THAT PIN!
*     
* 
* TODO:  Scheduling and GPS-acquired time using NMEA streaming to/from GPS module
*        labels to pins 
*        Add more externally-scripted functions, like entire port pin changes, watches on pins with routines that will execute routines to any combo of pins upon pin[s] conditions,
*        alert when back pressure within furnace indicates to change filter
*        damper operation with multiple temp sensors
* 
*        https://github.com/wemos/Arduino_XI  for the IDE support for TTGO XI/WeMo XI
* 
*      Standard furnace nomenclature can be somewhat disorienting: blower is the term for fan when the furnace unit is being referenced by part for a furnace workman,
*                                                                  fan is the term for same part but for the thermostat operator person
* 
********************************************************************************************************************************************************************************************************/
#define VERSION "1.0"

//On the first run of this sketch, if you received an error message about the following line...
//#define RESTORE_FACTORY_DEFAULTS //As the error message said, uncomment this line, compile & load for first run EEPROM setup in WeMo XI/TTGO XI and any other board that needs it, then comment back out and recompile and load b/c sketch would be too long otherwise

//If you've connected the analog temperature sensor[s] the opposite direction, thus seeing reported temperature go the opposite direction the real temperature goes, you may re-compile with the following line commented out rather than re-wire:
#define ALL_ANALOG_SENSOR_CIRCUITS_ARE_THERMISTOR_TO_EXCITATION_VOLTS_AND_RESISTOR_TO_GROUND

#ifndef u8
    #define u8 uint8_t
#endif
#ifndef u16
    #define u16 uint16_t
#endif
#include "analog_pin_adjust.h"
#include <EEPROM.h> // Any board that errors compiling this line is unsuitable to be a thermostat because it cannot store settings persistently

double analogReadLeast( u8 pin )
{
    double raw = analogRead( pin );
    for( u8 times = 0; times < 4 ; times++ )
    {//go for more and keep lowest value
        double raw2 = analogRead( pin );
        if( raw2 < raw ) raw = raw2;
    }
    return raw;
}

#include "DHTdirectRead.h"
#define _baud_rate_ 57600 //Very much dependent upon the capability of the host computer to process talkback data, not just baud rate of its interface
#if not defined ( __LGT8FX8E__ ) && not defined ( ARDUINO_AVR_YUN ) && not defined ( ARDUINO_AVR_LEONARDO ) && not defined ( ARDUINO_AVR_LEONARDO_ETH ) && not defined ( ARDUINO_AVR_MICRO ) && not defined ( ARDUINO_AVR_ESPLORA ) && not defined ( ARDUINO_AVR_LILYPAD_USB ) && not defined ( ARDUINO_AVR_YUNMINI ) && not defined ( ARDUINO_AVR_INDUSTRIAL101 ) && not defined ( ARDUINO_AVR_LININO_ONE )
    #ifndef RESTORE_FACTORY_DEFAULTS
        #define RESTORE_FACTORY_DEFAULTS
    #endif
#else
    #if defined ( __LGT8FX8E__ )
        #define _baud_rate_ 19200 //During sketch development it was found that the XI tends to revert to baud 19200 after programming or need that rate prior to programming so we can't risk setting baud to anything but that until trusted in the future
        #define LED_BUILTIN 12
//Commonly available TTGO XI/WeMo XI EEPROM library has only .read() and .write() methods.
        #define EEPROMlength 1024
        #define NUM_DIGITAL_PINS 14  //here if necessary or FYI.  TTGO XI has no D13 labeled but its prob sck, https://www.avrfreaks.net/comment/2247501#comment-2247501 says why D14 and up don't work digital: "lgt8fx8x_init() disables digital with DIDR0 register.   Subsequent pinMode() should enable/disable digital as required.   i.e. needs a patch in wiring_digital.c"
    #endif
#endif
#ifndef EEPROMlength
    #define EEPROMlength EEPROM.length()
#endif

#ifndef SERIAL_PORT_HARDWARE
    #define SERIAL_PORT_HARDWARE 0
#endif


//All temps are shorts until displayed
//Where are other indoor sensors?  TODO WITH DAMPERS
//What about utilizing PCI ISRs and how they control which pins to use?  POSSIBLY TODO

// INTENT: Access these values by reference and pointers.  They will be hardcoded here only.  The run-time code can only find out what they are, how many there are, and their names by calls to this function
//  The following are addresses in EEPROM of values that need to be persistent across power outages.
//The first two address locations in EEPROM store a tatoo used to ensure EEPROM contents are valid for this sketch
u8 primary_temp_sensor_pin_address = 2;
u8 heat_pin_address = 3;
u8 furnace_blower_pin_address = 4;
u8 power_cycle_pin_address = 5;
u8 thermostat_mode_address = 6;
u8 fan_mode_address = 7;
u8 secondary_temp_sensor_pin_address = 8;
u8 cool_pin_address = 9;
u8 outdoor_temp_sensor1_pin_address = 10;
u8 outdoor_temp_sensor2_pin_address = 11;
#ifdef PIN_A0 
    u8 calibration_offset_array_start_address_first_byte = 12;//will occupy two bytes
#endif

#define NO_BOUNDS_CHECK 0
#define HEAT_TEMP_BOUNDS_CHECK 1
#define COOL_TEMP_BOUNDS_CHECK 2
#define BOTH_TEMPS_BOUNDS_CHECK 3

#define ADJUST_START_AND_STOP_LIMITS_TOGETHER 4
#define SINGLE_LIMIT_ONLY 0

#define ADJUST 0
#define SETFULL 16

#define HEAT_UPPER_BOUNDS 27
#define HEAT_LOWER_BOUNDS 10
#define COOL_UPPER_BOUNDS 32
#define COOL_LOWER_BOUNDS 15
#define NO_TEMP_ENTERED 2

#define SHORTEN 0
#define REPLACE 1

#define NOT_RUNNING false
#define ALREADY_RUNNING true

#define KY013 0
#define RAW 1

#define TO_END_OF_STRING 1
#define NOT_TO_END_OF_STRING 0

unsigned int logging_address = EEPROMlength - sizeof( boolean );
unsigned int upper_heat_temp_address = logging_address - sizeof( short );//EEPROMlength - 2;
unsigned int lower_heat_temp_address = upper_heat_temp_address - sizeof( short );//EEPROMlength - 3;
unsigned int logging_temp_changes_address = lower_heat_temp_address - sizeof( boolean );//EEPROMlength - 4;
unsigned int upper_cool_temp_address = logging_temp_changes_address - sizeof( short );
unsigned int lower_cool_temp_address = upper_cool_temp_address - sizeof( short );

// So we don't assume every board has exactly three
//in case of boards that have more than 24 pins ( three virtual 8-bit ports )
//int number_of_ports_that_present_pins = EEPROM.read( EEPROMlength - 5 );  // NOT UTILIZED THIS VERSION

// main temperature sensor, heat, auxiliary heat device, system power cycle, porch light, dining room coffee outlet
//  The following values need to be stored persistent through power outages.  Sadly, the __LGT8FX8E__ will not read EEPROM until the setup() loop starts executing, so these values get set there for all boards for simplicity
u8 primary_temp_sensor_pin;// = EEPROM.read( primary_temp_sensor_pin_address ); 
u8 secondary_temp_sensor_pin;// = EEPROM.read( secondary_temp_sensor_pin_address ); 
u8 heat_pin;// = EEPROM.read( heat_pin_address );
u8 cool_pin;// = EEPROM.read( cool_pin_address );
u8 furnace_blower_pin;// = EEPROM.read( furnace_blower_pin_address );
u8 power_cycle_pin;// = EEPROM.read( power_cycle_pin_address );
u8 outdoor_temp_sensor1_pin;
u8 outdoor_temp_sensor2_pin;
boolean logging;// = ( boolean )EEPROM.read( logging_address );
boolean logging_temp_changes;// = ( boolean )EEPROM.read( logging_temp_changes_address );
float lower_heat_temp_floated;//filled in setup
float upper_heat_temp_floated;
float upper_cool_temp_floated;
float lower_cool_temp_floated;
char thermostat_mode;// = ( char )EEPROM.read( thermostat_mode_address );
char fan_mode;// = ( char )EEPROM.read( fan_mode_address );//a';//Can be either auto (a) or on (o)

char strFull[ ] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, \
                   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float last_three_temps[] = { -100, -101, -102 };
float heat_started_temp_x_3 = last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ];
u8 pin_specified;
float temp_specified_floated;
boolean fresh_powerup = true;
long unsigned timer_alert_furnace_sent = 0;
const u8 factory_setting_primary_temp_sensor_pin PROGMEM = 2 + 128;//the 128 restricts the pin to digital mode only so if device fails in the future unexpectedly sketch would not revert the pin to analog mode if the pin is capable of analog mode
const u8 PROGMEM factory_setting_heat_pin PROGMEM = 3;
const u8 PROGMEM factory_setting_furnace_blower_pin PROGMEM = 4;
const u8 PROGMEM factory_setting_power_cycle_pin PROGMEM = 5;
const u8 PROGMEM factory_setting_cool_pin PROGMEM = 9;
const bool PROGMEM factory_setting_logging_setting PROGMEM = true;
const bool PROGMEM factory_setting_logging_temp_changes_setting PROGMEM = true;
const char PROGMEM factory_setting_thermostat_mode PROGMEM = 'h';
const char factory_setting_fan_mode PROGMEM = 'a';
const float factory_setting_lower_heat_temp_floated PROGMEM = 22.4;
const float factory_setting_upper_heat_temp_floated PROGMEM = 23;
const float factory_setting_lower_cool_temp_floated PROGMEM = 21.5;
const float factory_setting_upper_cool_temp_floated PROGMEM = 22.1;
const u8 factory_setting_secondary_temp_sensor_pin PROGMEM = 8 + 128;//the 128 restricts it to only digital sensor
const u8 factory_setting_outdoor_temp_sensor1_pin PROGMEM = 10 + 128;//the 128 restricts it to only digital sensor
const u8 factory_setting_outdoor_temp_sensor2_pin PROGMEM = 11 + 128;//the 128 restricts it to only digital sensor
const float minutes_furnace_should_be_effective_after PROGMEM = 5.5; //Can be decimal this way
const unsigned long loop_cycles_to_skip_between_alert_outputs PROGMEM = 4 * 60 * 30;//estimating 4 loops per second, 60 seconds per minute, 30 minutes per alert

const char str_talkback[] PROGMEM = "talkback";
const char str_pin_set[] PROGMEM = "pin set";
const char str_pins_read[] PROGMEM = "pins read";
const char str_read_pins[] PROGMEM = "read pins";
const char str_set_pin_to[] PROGMEM = "set pin to";
//const char str_pin_set_to[] PROGMEM = "pin set to";
const char str_view[] PROGMEM = "view";
const char str_sensor[] PROGMEM = "sensor";
const char str_persistent_memory[] PROGMEM = "persistent memory";
const char str_changes[] PROGMEM = "changes";
const char str_change[] PROGMEM = "change";
const char str_thermostat[] PROGMEM = "thermostat";
const char str_fan_auto[] PROGMEM = "fan auto";
const char str_fan_on[] PROGMEM = "fan on";
const char str_factory[] PROGMEM = "factory";
const char str_cycle_power[] PROGMEM = "cycle power";
const char str_pin_read[] PROGMEM = "pin read";

const char str_auto[] PROGMEM = "auto";
const char str_off[] PROGMEM = "off";

const char str_heat[] PROGMEM = "heat";
const char str_cool[] PROGMEM = "cool";

const char str_heat_temps[] PROGMEM = "heat temps";
const char str_cool_temps[] PROGMEM = "cool temps";
const char str_all_temps[] PROGMEM = "all temps";
const char str_power_cycle[] PROGMEM = "power cycle";
const char str_read_pin[] PROGMEM = "read pin";
const char str_read_pin_dot[] PROGMEM = "read pin .";
const char str_set_pin[] PROGMEM = "set pin";
const char str_logging[] PROGMEM = "logging";
const char str_report_master_room_temp[] PROGMEM = "report master room temp";
const char str_set_pin_high[] PROGMEM = "set pin high";
const char str_set_pin_low[] PROGMEM = "set pin low";
const char str_set_pin_output[] PROGMEM = "set pin output";
const char str_set_pin_input_with_pullup[] PROGMEM = "set pin input with pullup";
const char str_set_pin_input[] PROGMEM = "set pin input";
const char str_heatStartLowTemp[] PROGMEM = "heat start low temp";
const char str_heatStopHighTemp[] PROGMEM = "heat stop high temp";
const char str_coolStopLowTemp[] PROGMEM = "cool stop low temp";
const char str_coolStartHighTemp[] PROGMEM = "cool start high temp";
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
bool heat_state = false;
bool cool_state = false;
long unsigned check_furnace_effectiveness_time;

#ifdef __LGT8FX8E__
void EEPROMupdate ( unsigned long address, u8 val )
{
    if( EEPROM.read( address ) != val ) EEPROM.write( address, val );
}
#endif

void printAnalogLevel( u8 pin_specified_local )
{
#ifdef ADC_BITS
        analogReadResolution( 12 );
#endif
        Serial.print( F( " level: " ) );
        Serial.print( analogReadLeast( pin_specified_local ), 0 );
#ifdef ADC_BITS
        analogReadResolution( 10 );
#endif
}

void assign_pins( bool already_running )
{
    digitalWrite( heat_pin, LOW );
    digitalWrite( cool_pin, LOW );
    digitalWrite( furnace_blower_pin, LOW );
//lines above cover pins getting changed below.  It is actually a feature of this sketch
    fan_mode = ( char )EEPROM.read( fan_mode_address );//a';//Can be either auto (a) or on (o)
    primary_temp_sensor_pin = EEPROM.read( primary_temp_sensor_pin_address ); 
    secondary_temp_sensor_pin = EEPROM.read( secondary_temp_sensor_pin_address );
    outdoor_temp_sensor1_pin = EEPROM.read( outdoor_temp_sensor1_pin_address );
    outdoor_temp_sensor2_pin = EEPROM.read( outdoor_temp_sensor2_pin_address );
    heat_pin = EEPROM.read( heat_pin_address );
    cool_pin = EEPROM.read( cool_pin_address );
    furnace_blower_pin = EEPROM.read( furnace_blower_pin_address );
    power_cycle_pin = EEPROM.read( power_cycle_pin_address );
    pinMode( power_cycle_pin, OUTPUT );
    digitalWrite( power_cycle_pin, LOW );
    pinMode( heat_pin, OUTPUT );
    digitalWrite( heat_pin, heat_state );
    pinMode( cool_pin, OUTPUT );
    digitalWrite( cool_pin, cool_state );
    pinMode( furnace_blower_pin, OUTPUT );
    if( fan_mode == 'o' ) digitalWrite( furnace_blower_pin, HIGH );
    else digitalWrite( furnace_blower_pin, LOW );
}

void printThermoModeWord( char setting_char, bool newline )
{
    const char *setting PROGMEM;
    if( setting_char == 'o' )
        setting = str_off;
    else if( setting_char == 'h' )
        setting = str_heat;
    else if( setting_char == 'c' )
        setting = str_cool;
#if not defined ( __LGT8FX8E__ ) && not defined ( ARDUINO_AVR_YUN ) && not defined ( ARDUINO_AVR_LEONARDO ) && not defined ( ARDUINO_AVR_LEONARDO_ETH ) && not defined ( ARDUINO_AVR_MICRO ) && not defined ( ARDUINO_AVR_ESPLORA ) && not defined ( ARDUINO_AVR_LILYPAD_USB ) && not defined ( ARDUINO_AVR_YUNMINI ) && not defined ( ARDUINO_AVR_INDUSTRIAL101 ) && not defined ( ARDUINO_AVR_LININO_ONE )
    else if( setting_char == 'a' )
        setting = str_auto;
#endif
    else 
    {
        Serial.println();
        return;
    }
    Serial.print( ( const __FlashStringHelper * )setting );
    if( newline ) Serial.println();
}

void printThermoModeWord( long unsigned var_with_setting, bool newline )
{
    if( thermostat_mode == 'o' )
        var_with_setting = ( long unsigned )str_off;
    else if( thermostat_mode == 'h' )
        var_with_setting = ( long unsigned )str_heat;
    else if( thermostat_mode == 'c' )
        var_with_setting = ( long unsigned )str_cool;
#if not defined ( __LGT8FX8E__ ) && not defined ( ARDUINO_AVR_YUN ) && not defined ( ARDUINO_AVR_LEONARDO ) && not defined ( ARDUINO_AVR_LEONARDO_ETH ) && not defined ( ARDUINO_AVR_MICRO ) && not defined ( ARDUINO_AVR_ESPLORA ) && not defined ( ARDUINO_AVR_LILYPAD_USB ) && not defined ( ARDUINO_AVR_YUNMINI ) && not defined ( ARDUINO_AVR_INDUSTRIAL101 ) && not defined ( ARDUINO_AVR_LININO_ONE )
    else if( thermostat_mode == 'a' )
        var_with_setting = ( long unsigned )str_auto;
#endif
    else 
    {
        Serial.println();
        return;
    }
    Serial.print( ( const __FlashStringHelper * )var_with_setting );
    if( newline ) Serial.println();
}

#ifdef RESTORE_FACTORY_DEFAULTS
    #include "restore_factory_defaults.h"
#endif

void refusedNo_exclamation() //putting this in a function for just 2 calls saves 64 bytes due to short memory in WeMo/TTGO XI
{
     if( logging )
     {
        Serial.print( F( "Append a '!' or pin " ) );
        Serial.print( pin_specified );
        Serial.println( F( " is reserved" ) );
     }
}

void setFurnaceEffectivenessTime()
{
    check_furnace_effectiveness_time = millis();//Will this become invalid if heat temp setting gets adjusted?  TODO:  Account for that condition
    heat_started_temp_x_3 = last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ];
}

void IfReservedPinGettingForced( bool level )
{
    if( pin_specified == furnace_blower_pin )
    {
        if( (bool )level ) fan_mode = 'a'; //high; low = 'a'
        else fan_mode = 'o'; //high; low = 'a'
#ifndef __LGT8FX8E__
           EEPROM.update( fan_mode_address, fan_mode ); //high; low = 'a'
#else
            EEPROMupdate( fan_mode_address, fan_mode );
#endif
    }
    else if( pin_specified == heat_pin )
    {
        timer_alert_furnace_sent = 0;
        if( level )
        {
            setFurnaceEffectivenessTime();
        }
        heat_state = ( bool ) level; //high = true; low = false
    }
    else if( pin_specified == cool_pin ) cool_state = ( bool ) level; //high; = true low = false
}

bool refuseInput()
{
    if( pin_specified == power_cycle_pin || pin_specified == furnace_blower_pin || pin_specified == heat_pin || pin_specified == cool_pin )
    {
         if( logging )
         {
                if( pin_specified == power_cycle_pin ) Serial.print( F( "Host pwr" ) );
                if( pin_specified == furnace_blower_pin ) Serial.print( F( "Furnace fan" ) );
                if( pin_specified == heat_pin ) Serial.print( F( "Furnace" ) );
                if( pin_specified == cool_pin ) Serial.print( F( "A/C" ) );
                Serial.print( F( " pin " ) );
                Serial.print( pin_specified );
                Serial.println( F( " skipped" ) );
         }
         return true;
    }
    return false;
}

bool pin_print_and_not_sensor( bool setting, u8 pin_specified )
{
    if( ( pin_specified == ( primary_temp_sensor_pin & 0x7F ) || pin_specified == ( secondary_temp_sensor_pin & 0x7F ) ) || ( pin_specified == ( outdoor_temp_sensor1_pin & 0x7F ) && DHTfunctionResultsArray[ outdoor_temp_sensor1_pin ].ErrorCode == DEVICE_READ_SUCCESS ) || ( pin_specified == ( outdoor_temp_sensor2_pin & 0x7F ) && DHTfunctionResultsArray[ outdoor_temp_sensor2_pin ].ErrorCode == DEVICE_READ_SUCCESS ) )
    {
        if( pin_specified == ( primary_temp_sensor_pin & 0x7F ) ) Serial.print( F( "Indoor prim" ) );
        else if( pin_specified == ( secondary_temp_sensor_pin & 0x7F ) ) Serial.print( F( "Indoor second" ) );
        else if( pin_specified == ( outdoor_temp_sensor1_pin & 0x7F ) ) Serial.print( F( "Outdoor prim" ) );
        else if( pin_specified == ( outdoor_temp_sensor2_pin & 0x7F ) ) Serial.print( F( "Outdoor second" ) );
        Serial.print( F( "ary temp sensor pin " ) );
        return( false );
    }
    if( setting ) Serial.print( F( "time_stamp_this " ) );//only do if returning true
    Serial.print( F( "Pin " ) );
    Serial.print( pin_specified );
    if( setting ) Serial.print( F( " now set to " ) );
}

void illegal_attempt_SERIAL_PORT_HARDWARE()
{
    Serial.print( F( "Rx from host pin " ) );
    Serial.print( SERIAL_PORT_HARDWARE );
    Serial.println( F( " skipped" ) );
}

boolean IsValidPinNumber( const char* str, u8 type_analog_allowed )
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
    pin_specified = ( u8 )atoi( str );
    if( j == i || ( !type_analog_allowed && ( ( pin_specified & 0x7F ) >= NUM_DIGITAL_PINS ) ) || ( ( pin_specified & 0x7F ) >= NUM_DIGITAL_PINS && !memchr( analog_pin_list, pin_specified, PIN_Amax ) ) )
    {
        Serial.println( F( " Pin # error-see help screen " ) );
        Serial.println( str );
//        Serial.println( NUM_DIGITAL_PINS );
//        for( u8 i = 0; i < sizeof( analog_pin_list ); i++ )
//        Serial.println( analog_pin_list[ i ] );
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
        Serial.print( F( "Pin " ) );
        Serial.print( pin );
        Serial.println( F( " not set to output" ) );
    }
    return false;
}

u8 IsValidTemp( const char* str, u8 boundsCheckAndAdjustTogether )
{
    for( unsigned int i = 0; i < strlen( str ); i++ )
    {
        if( !i && str[ 0 ] == '-' );
        else if( !( isdigit( str[ i ] ) || str[ i ] == '.' ) || ( strchr( str, '.' ) != strrchr( str, '.' ) ) ) //allow one and only one decimal point in str
        {
            return NO_TEMP_ENTERED;
        }
    }
    temp_specified_floated = ( float )atoi( str );
    if( strchr( str, '.' ) ){ str = strchr( str, '.' ) + 1; temp_specified_floated += ( ( float )atoi( str ) ) / 10; };

    if( ( ( boundsCheckAndAdjustTogether & HEAT_TEMP_BOUNDS_CHECK ) && !( boundsCheckAndAdjustTogether & ADJUST_START_AND_STOP_LIMITS_TOGETHER ) && ( temp_specified_floated < HEAT_LOWER_BOUNDS || temp_specified_floated > HEAT_UPPER_BOUNDS ) ) || \
        ( ( boundsCheckAndAdjustTogether & COOL_TEMP_BOUNDS_CHECK ) && !( boundsCheckAndAdjustTogether & ADJUST_START_AND_STOP_LIMITS_TOGETHER ) && ( temp_specified_floated < COOL_LOWER_BOUNDS || temp_specified_floated > COOL_UPPER_BOUNDS ) ) || \
        ( ( boundsCheckAndAdjustTogether & HEAT_TEMP_BOUNDS_CHECK ) && ( boundsCheckAndAdjustTogether & ADJUST_START_AND_STOP_LIMITS_TOGETHER ) && ( lower_heat_temp_floated + temp_specified_floated < HEAT_LOWER_BOUNDS || upper_heat_temp_floated + temp_specified_floated > HEAT_UPPER_BOUNDS ) ) || \
        ( ( boundsCheckAndAdjustTogether & COOL_TEMP_BOUNDS_CHECK ) && ( boundsCheckAndAdjustTogether & ADJUST_START_AND_STOP_LIMITS_TOGETHER ) && ( lower_cool_temp_floated + temp_specified_floated < COOL_LOWER_BOUNDS || upper_cool_temp_floated + temp_specified_floated > COOL_UPPER_BOUNDS ) ) )//This check should be called for the rooms where it matters
    {
        Serial.println( F( "Would be usafe" ) );
        return ( u8 )false;
    }
    return ( u8 )true;
}

void printBasicInfo()
{
    Serial.println( F( ".." ) );
    if( fresh_powerup )
    {
       Serial.println( F( "A way to save talkbacks into a file in Linux is: (except change \"TIME_STAMP_THIS\" to lower case, not shown so this won't get filtered in by such command)" ) );
       Serial.print( F( "    nohup stty igncr -F \$(ls /dev/ttyA* /dev/ttyU* 2>/dev/null|tail -n1) " ) );
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
#if not defined ( __LGT8FX8E__ ) && not defined ( ARDUINO_AVR_YUN ) && not defined ( ARDUINO_AVR_LEONARDO ) && not defined ( ARDUINO_AVR_LEONARDO_ETH ) && not defined ( ARDUINO_AVR_MICRO ) && not defined ( ARDUINO_AVR_ESPLORA ) && not defined ( ARDUINO_AVR_LILYPAD_USB ) && not defined ( ARDUINO_AVR_YUNMINI ) && not defined ( ARDUINO_AVR_INDUSTRIAL101 ) && not defined ( ARDUINO_AVR_LININO_ONE )
    Serial.print( F( "Operating mode (heat/cool/auto/off)=" ) );
#else
    Serial.print( F( "Operating mode (heat/cool/off)=" ) );
#endif
    printThermoModeWord( thermostat_mode, true );
    Serial.print( F( "Fan mode=" ) );
    if( fan_mode == 'a' ) Serial.println( F( "auto" ) );
    else if( fan_mode == 'o' ) Serial.println( F( "on" ) );
    Serial.print( ( const __FlashStringHelper * )str_heatStartLowTemp );
    Serial.print( '=' );
    Serial.println( lower_heat_temp_floated, 1 );
    Serial.print( ( const __FlashStringHelper * )str_heatStopHighTemp );
    Serial.print( '=' );
    Serial.println( upper_heat_temp_floated, 1 );
    Serial.print( ( const __FlashStringHelper * )str_coolStopLowTemp );
    Serial.print( '=' );
    Serial.println( lower_cool_temp_floated, 1 );
    Serial.print( ( const __FlashStringHelper * )str_coolStartHighTemp );
    Serial.print( '=' );
    Serial.println( upper_cool_temp_floated, 1 );
    Serial.print( F( "Logging talkback is turned o" ) );
    if( logging ) Serial.println( F( "n" ) );
    else  Serial.println( F( "ff" ) );
    Serial.print( F( "Talkback of temp changes is turned o" ) );
    if( logging_temp_changes ) Serial.println( F( "n" ) );
    else  Serial.println( F( "ff" ) );
    Serial.print( F( "primary, secondary indoor temp sensor pins=" ) );
    Serial.print( primary_temp_sensor_pin & 0x7F );
    Serial.print( F( "@" ) );
    Serial.print( primary_temp_sensor_pin_address );
    Serial.print( F( ", " ) );
    Serial.print( secondary_temp_sensor_pin & 0x7F );
    Serial.print( F( "@" ) );
    Serial.println( secondary_temp_sensor_pin_address );
    Serial.print( F( "outdoor temp sensor1, 2 pins=" ) );
    Serial.print( outdoor_temp_sensor1_pin & 0x7F );
    Serial.print( F( "@" ) );
    Serial.print( outdoor_temp_sensor1_pin_address );
    Serial.print( F( ", " ) );
    Serial.print( outdoor_temp_sensor2_pin & 0x7F );
    Serial.print( F( "@" ) );
    Serial.println( outdoor_temp_sensor2_pin_address );
    Serial.print( F( "heat, fan, cool pins=" ) );
    Serial.print( heat_pin );
    Serial.print( F( "@" ) );
    Serial.print( heat_pin_address );
    Serial.print( F( ", " ) );
    Serial.print( furnace_blower_pin );
    Serial.print( F( "@" ) );
    Serial.print( furnace_blower_pin_address );
    Serial.print( F( ", " ) );
    Serial.print( cool_pin );
    Serial.print( F( "@" ) );
    Serial.println( cool_pin_address );
    Serial.print( F( "system power pin=" ) );
    Serial.print( power_cycle_pin );
    Serial.print( F( "@" ) );
    Serial.println( power_cycle_pin_address );
    Serial.println( F( "  (pin@EEPROM address)" ) );
    Serial.print( F( "LED_BUILTIN pin=" ) );
    Serial.println( LED_BUILTIN );
    Serial.print( F( "Digital pins=0 to " ) );
    Serial.println( NUM_DIGITAL_PINS - 1 );
    Serial.print( F( "Analog pins=" ) );
    for( u8 i = 0; i < sizeof( analog_pin_list ); i++ )
    {
        Serial.print( analog_pin_list[ i ] );
        if( i < sizeof( analog_pin_list ) - 1 )
        {
            Serial.print( F( "," ) );
        }
        else
        {
            Serial.println();
        }
    }
    Serial.println( F( "." ) );
    Serial.println( F( "Pin numbers may otherwise be period (all pins) with +/-/! for setting and forcing reserved pins" ) );
    Serial.println( F( "Example: pin set to output .-! (results in all pins [.] being set to output with low logic level [-], even reserved pins [!])" ) );
    Serial.println( F( "Valid commands, cAsE sEnSiTiVe:" ) );
    Serial.println( F( "." ) );
    Serial.println( F( "help (show this screen)" ) );
#if not defined ( __LGT8FX8E__ ) && not defined ( ARDUINO_AVR_YUN ) && not defined ( ARDUINO_AVR_LEONARDO ) && not defined ( ARDUINO_AVR_LEONARDO_ETH ) && not defined ( ARDUINO_AVR_MICRO ) && not defined ( ARDUINO_AVR_ESPLORA ) && not defined ( ARDUINO_AVR_LILYPAD_USB ) && not defined ( ARDUINO_AVR_YUNMINI ) && not defined ( ARDUINO_AVR_INDUSTRIAL101 ) && not defined ( ARDUINO_AVR_LININO_ONE )
    Serial.println( F( "ther[mostat][ a[uto]/ o[ff]/ h[eat]/ c[ool]] (read or set thermostat mode)" ) );//, auto requires outdoor sensor[s] and reverts to heat if sensor[s] fail)" ) );
#else
    Serial.println( F( "ther[mostat][ o[ff]/ h[eat]/ c[ool]] (read or set thermostat mode)" ) );//, auto requires outdoor sensor[s] and reverts to heat if sensor[s] fail)" ) );
#endif
    Serial.println( F( "fan[ a[uto]/ o[n]] (read or set fan)" ) );
    Serial.println( F( "heat start low temp[ <°C>] (heat turns on at this or lower temperature)" ) );//not worth converting for some unkown odd reason
    Serial.println( F( "heat stop high temp[ <°C>] (heat turns off at this or higher temperature)" ) );
    Serial.println( F( "cool stop low temp[ <°C>] (A/C turns off at this or lower temperature)" ) );
    Serial.println( F( "cool start high temp[ <°C>] (A/C turns on at this or higher temperature)" ) );

//    Serial.print( ( const __FlashStringHelper * )str_heat_temps );//uses more bytes this way
    Serial.println( F( "heat temps[ <°C>] (adjust heat settings up, use - for down)" ) );
//    Serial.print( ( const __FlashStringHelper * )str_cool_temps );//uses more bytes this way
    Serial.println( F( "cool temps[ <°C>] (adjust cool settings up, use - for down)" ) );
//    Serial.print( ( const __FlashStringHelper * )str_all_temps );//uses more bytes this way
    Serial.println( F( "all temps[ <°C>] (adjust all temperature settings up, use - for down)" ) );

    Serial.println( F( "talkback[ on/off] (or logging[ on/off], off may confuse user)" ) );//(for the host system to log when each output pin gets set high or low, always persistent)" ) );
    Serial.println( F( "talkback temp change[s[ on/off]] (or logging temp changes[ on/off])(requires normal talkback on)" ) );// - for the host system to log whenever the main room temperature changes, always persistent)" ) );
    Serial.println( ( const __FlashStringHelper * )str_report_master_room_temp );
    Serial.println( F( "power cycle (or cycle power)" ) );

    Serial.println( F( "read pin <pin number or .> (or ...pin read...)(obtain the name if any, setting and voltage)" ) );
    Serial.println( F( "read pins (or pins read )(obtain the names, settings and voltages of ALL pins)" ) );
    Serial.println( F( "read sens[or] <pin number or .> (retrieves sensor reading, a period with due care in place of pin number for all pins)" ) );
    Serial.println( F( "set pin [to] output <pin number or .> (or ...pin set)" ) );
    Serial.println( F( "set pin [to] input <pin number or .> [pers] (or ...pin set...)" ) );
    Serial.println( F( "set pin [to] input with pullup <pin number or .> [pers] (or ...pin set...)" ) );
    Serial.println( F( "set pin [to] low <pin number or .> [pers] (or ...pin set...)(only allowed to pins assigned as output)" ) );
    Serial.println( F( "set pin [to] high <pin number or .> [pers] (or ...pin set...)(only allowed to pins assigned as output)" ) );

    Serial.println( F( "ch[ange] pers[istent memory] <address> <value> (changes EEPROM, see source code for addresses of data)" ) );
    Serial.println( F( "ch[ange] pers[istent memory] <StartingAddress> \"<character string>[\"[ 0]] (store character string in EEPROM as long as desired, optional null-terminated. Reminder: echo -e and escape the quote[s])" ) );
    Serial.println( F( "vi[ew] pers[istent memory] <StartingAddress>[ <EndingAddress>] (views EEPROM)" ) );
#ifdef RESTORE_FACTORY_DEFAULTS
    Serial.println( F( "vi[ew] fact[ory defaults] (see what would happen before you reset to them)" ) );
    Serial.println( F( "reset (factory defaults: pure, simple and absolute)" ) );
#else
    #ifndef __AVR_ATmega32U4__
        Serial.println( F( "vi[ew] fact[ory defaults] (sketch re-compile would be required this board. See source code comments)" ) );
    #endif
#endif
    Serial.println( F( "assign pins (from EEPROM)" ) );
    Serial.println( F( "test alert (sends an alert message for testing)" ) );
    Serial.println( F( ".." ) );
}

void print_factory_defaults()
{
    Serial.print( F( "Version: " ) );
    Serial.print( F( VERSION ) );
    Serial.println( F( " Factory defaults:" ) );
#if not defined ( __LGT8FX8E__ ) && not defined ( ARDUINO_AVR_YUN ) && not defined ( ARDUINO_AVR_LEONARDO ) && not defined ( ARDUINO_AVR_LEONARDO_ETH ) && not defined ( ARDUINO_AVR_MICRO ) && not defined ( ARDUINO_AVR_ESPLORA ) && not defined ( ARDUINO_AVR_LILYPAD_USB ) && not defined ( ARDUINO_AVR_YUNMINI ) && not defined ( ARDUINO_AVR_INDUSTRIAL101 ) && not defined ( ARDUINO_AVR_LININO_ONE )
    Serial.print( F( "Operating mode (heat/cool/auto/off)=" ) );
#else
    Serial.print( F( "Operating mode (heat/cool/off)=" ) );
#endif
    printThermoModeWord( factory_setting_thermostat_mode, true );
    Serial.print( F( "Fan mode=" ) );
    if( factory_setting_fan_mode == 'a' ) Serial.println( F( "auto" ) );
    else if( factory_setting_fan_mode == 'o' ) Serial.println( F( "on" ) );
    Serial.print( ( const __FlashStringHelper * )str_heatStartLowTemp );
    Serial.print( '=' );
    Serial.println( factory_setting_lower_heat_temp_floated, 1 );
    Serial.print( ( const __FlashStringHelper * )str_heatStopHighTemp );
    Serial.print( '=' );
    Serial.println( factory_setting_upper_heat_temp_floated, 1 );
    Serial.print( ( const __FlashStringHelper * )str_coolStopLowTemp );
    Serial.print( '=' );
    Serial.println( factory_setting_lower_cool_temp_floated, 1 );
    Serial.print( ( const __FlashStringHelper * )str_coolStartHighTemp );
    Serial.print( '=' );
    Serial.println( factory_setting_upper_cool_temp_floated, 1 );
    Serial.print( F( "Logging talkback o" ) );
    if( factory_setting_logging_setting ) Serial.println( F( "n" ) );
    else   Serial.println( F( "ff" ) );
    Serial.print( F( "Logging temp changes o" ) );
    if( factory_setting_logging_temp_changes_setting ) Serial.println( F( "n" ) );
    else  Serial.println( F( "ff" ) );
    Serial.println();
    Serial.print( F( "primary_temp_sensor_pin=" ) );
    Serial.println( factory_setting_primary_temp_sensor_pin & 0x7F );
    Serial.print( F( "secondary_temp_sensor_pin=" ) );
    Serial.println( factory_setting_secondary_temp_sensor_pin & 0x7F );
    Serial.print( F( "outdoor_temp_sensor1_pin=" ) );
    Serial.println( factory_setting_outdoor_temp_sensor1_pin & 0x7F );
    Serial.print( F( "outdoor_temp_sensor_pin=" ) );
    Serial.println( factory_setting_outdoor_temp_sensor2_pin & 0x7F );
    Serial.print( F( "heat_pin=" ) );
    Serial.println( factory_setting_heat_pin );
    Serial.print( F( "furnace_blower_pin=" ) );
    Serial.println( factory_setting_furnace_blower_pin );
    Serial.print( F( "cool_pin=" ) );
    Serial.println( factory_setting_cool_pin );
    Serial.print( F( "automation system power_cycle_pin=" ) );
    Serial.println( factory_setting_power_cycle_pin );
//    Serial.print( F( "LED_BUILTIN pin=" ) );
//    Serial.println( LED_BUILTIN );
}

void setup()
{
//  analogReadResolution(10); //This is default and what this sketch algorithm assumes.  12 bits resolution aren't known to be necessary for a simple home thermostat, nor can the increased compiled sketch size from this command otherwise be justified
    Serial.begin( _baud_rate_ );
    Serial.setTimeout( 10 );
    //read EEPROM addresses 0 (LSB)and 1 (MSB).  This sketch expects MSB combined with LSB always contain ( NUM_DIGITAL_PINS + 1 ) * 3 for confidence in the remiander of the EEPROM's contents.
    u16 tattoo = 0;
#if defined ( RESTORE_FACTORY_DEFAULTS ) && ( defined ( __LGT8FX8E__ ) || defined ( ARDUINO_AVR_YUN ) || defined ( ARDUINO_AVR_LEONARDO ) || defined ( ARDUINO_AVR_LEONARDO_ETH ) || defined ( ARDUINO_AVR_MICRO ) || defined ( ARDUINO_AVR_ESPLORA ) || defined ( ARDUINO_AVR_LILYPAD_USB ) || defined ( ARDUINO_AVR_YUNMINI ) || defined ( ARDUINO_AVR_INDUSTRIAL101 ) || defined ( ARDUINO_AVR_LININO_ONE ) )
#else
    #ifndef __LGT8FX8E__
        EEPROM.get( 0, tattoo );
    #else
        tattoo = EEPROM.read( 0 );
        tattoo += ( u16 )( EEPROM.read( 1 ) << 8 );
    #endif
#endif
#ifdef __LGT8FX8E__
    delay( 10000 );//needed for TTGO XI Serial to initialize
    if( ( tattoo != ( NUM_DIGITAL_PINS + 1 ) * 3 ) && ( tattoo != 45 ) ) // Check for tattoo, allow for the obsolete one also
#else
    if( tattoo != ( NUM_DIGITAL_PINS + 1 ) * 3 ) // Check for tattoo
#endif
    {
        while ( !Serial ); // wait for serial port to connect. Needed for Leonardo's native USB
        Serial.println(); 
#ifdef RESTORE_FACTORY_DEFAULTS
            Serial.println( F( "Detected first time run so initializing to factory default pin names, power-up states and thermostat assignments, please wait..." ) );    
           restore_factory_defaults();
#else
        while( true )
        {
            Serial.println( F( "For this board to be a thermostat, EEPROM needs to get set up. Because there is not" ) );
            Serial.println( F( "enough room in some boards for the requisite code to be included by default, the" ) );
            Serial.println( F( "EEPROM configuring code will need to be executed by the following process:" ) );
            Serial.println( F( "1.  Modify the soure code of the main .ino file so that the line defining" ) );
            Serial.println( F( "    RESTORE_FACTORY_DEFAULTS is uncommented back into active code" ) );
            Serial.println( F( "2.  Don't save file that way, just reload board with modified sketch," ) );
            Serial.println( F( "    let board execute that modification of the sketch" ) );
            Serial.println( F( "    once to set up the EEPROM" ) );
            Serial.println( F( "3.  Revert sketch, reload" ) );  //Some boards just aren't large enough for the verbosity of next lines
//            Serial.println( F( "    just as the source code was originally - and upload the sketch into the board." ) );
//            Serial.println( F( "    After restarting the board it will operate as a thermostat." ) );
            Serial.println();
            delay( 20000 );
        }
#endif
    }
    else
    {
        assign_pins( NOT_RUNNING );
    }
    logging = ( boolean )EEPROM.read( logging_address );
    logging_temp_changes = ( boolean )EEPROM.read( logging_temp_changes_address );
    thermostat_mode = ( char )EEPROM.read( thermostat_mode_address );
    short lower_heat_temp_shorted_times_ten = 0;
    short upper_heat_temp_shorted_times_ten = 0;
    short upper_cool_temp_shorted_times_ten = 0;
    short lower_cool_temp_shorted_times_ten = 0;
#ifndef __LGT8FX8E__
    EEPROM.get( lower_heat_temp_address, lower_heat_temp_shorted_times_ten );
#else
    lower_heat_temp_shorted_times_ten = EEPROM.read( lower_heat_temp_address );
    lower_heat_temp_shorted_times_ten += ( u16 )( EEPROM.read( lower_heat_temp_address + 1 ) << 8 );
#endif
    lower_heat_temp_floated = ( float )( ( float )( lower_heat_temp_shorted_times_ten ) / 10 );
#ifndef __LGT8FX8E__
    EEPROM.get( upper_heat_temp_address, upper_heat_temp_shorted_times_ten );
#else
    upper_heat_temp_shorted_times_ten = EEPROM.read( upper_heat_temp_address );
    upper_heat_temp_shorted_times_ten += ( u16 )( EEPROM.read( upper_heat_temp_address + 1 ) << 8 );
#endif
    upper_heat_temp_floated = ( float )( ( float )( upper_heat_temp_shorted_times_ten ) / 10 );
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
#ifdef PIN_A0
    calibration_offset = EEPROM.read( calibration_offset_array_start_address_first_byte );
    calibration_offset += ( ( u16 )EEPROM.read( calibration_offset_array_start_address_first_byte + 1 ) ) << 8;
#endif
}

void fixInputted( u8 functionDesired, const char *strToFind, const char *strToReplaceWith, u8 lengthGoal )
{
    char *hit;
    hit = strstr_P( strFull, strToFind );
    if( hit )
    {
        if( functionDesired == REPLACE )
        {
            if( strlen_P( strToReplaceWith ) < strlen_P( strToFind ) )
            {
                functionDesired = SHORTEN;
                lengthGoal = strlen_P( strToReplaceWith );
                strcpy_P( hit, strToReplaceWith ); 
            }
            else
            {
                strncpy_P( hit, strToReplaceWith, strlen_P( strToReplaceWith ) + lengthGoal ); 
            }
        }
        if( functionDesired == SHORTEN )
        {
            memmove( hit + lengthGoal, hit + strlen_P( strToFind ), strlen( hit + strlen_P( strToFind ) ) + 1 );
        }
    }    
}

void printTooHighLow( u8 highOrLow )
{
    Serial.print( F( " too " ) );
    if( highOrLow == LOW )
        Serial.print( F( "low for this value. Raise" ) );
    else
        Serial.print( F( "high for this value. Lower" ) );
   Serial.println( F( " that before trying this value" ) );
}

void LimitSet( float *settingOfInterest, unsigned int settingOfInterestAddress, u8 AdjustOrSetfull )
{
    if( AdjustOrSetfull == SETFULL )
        *settingOfInterest = temp_specified_floated;
    else
        *settingOfInterest += temp_specified_floated;
    
    timer_alert_furnace_sent = 0;
    short temp_specified_shorted_times_ten = ( short )( *settingOfInterest * 10 );
#ifndef __LGT8FX8E__
    EEPROM.put( settingOfInterestAddress, temp_specified_shorted_times_ten );
#else
    EEPROMupdate( settingOfInterestAddress, ( u8 )temp_specified_shorted_times_ten );
    EEPROMupdate( settingOfInterestAddress + 1, ( u8 )( temp_specified_shorted_times_ten >> 8 ) );
#endif
}

void showHeatSettings( void )
{
    Serial.print( ( const __FlashStringHelper * )str_heatStartLowTemp );
    Serial.print( F( " now " ) );
    Serial.println( lower_heat_temp_floated, 1 );
    Serial.print( ( const __FlashStringHelper * )str_heatStopHighTemp );
    Serial.print( F( " now " ) );
    Serial.println( upper_heat_temp_floated, 1 );
}

void showCoolSettings( void )
{
    Serial.print( ( const __FlashStringHelper * )str_coolStopLowTemp );
    Serial.print( F( " now " ) );
    Serial.println( lower_cool_temp_floated, 1 );
    Serial.print( ( const __FlashStringHelper * )str_coolStartHighTemp );
    Serial.print( F( " now " ) );
    Serial.println( upper_cool_temp_floated, 1 );
}

void print_the_pin_and_sensor_reading( u8 pin_specified, u8 KY013orRaw )
{
    Serial.print( F( "Pin " ) );
    Serial.print( pin_specified );
    if( KY013orRaw == KY013 )
    {
        Serial.print( F( " sensor read: " ) );
        DHTresult* noInterrupt_result = ( DHTresult* )FetchTemp( pin_specified, LIVE );
        if( noInterrupt_result->ErrorCode == DEVICE_READ_SUCCESS || noInterrupt_result->Type == TYPE_ANALOG )
        {
            Serial.print( ( float )( ( float )( noInterrupt_result->TemperatureCelsius )/ 10 ), 1 );
            Serial.print( F( " °C, " ) );
            Serial.print( ( float )( ( float )noInterrupt_result->HumidityPercent / 10 ), 1 );
            Serial.print( F( " %" ) );
        }
        else
        {
            Serial.print( F( "Error " ) );
            Serial.print( noInterrupt_result->ErrorCode );
//            Serial.print( F( ", Type " ) );
//            Serial.print( noInterrupt_result->Type );//If no type is printed below it it > TYPE_ANALOG
        }
        if( noInterrupt_result->Type <= TYPE_ANALOG ) Serial.print( F( " TYPE " ) );
        if( noInterrupt_result->Type == TYPE_KNOWN_DHT11 ) Serial.print( F( "DHT11" ) );
        else if( noInterrupt_result->Type == TYPE_KNOWN_DHT22 ) Serial.print( F( "DHT22" ) );
        else if( noInterrupt_result->Type == TYPE_LIKELY_DHT11 ) Serial.print( F( "DHT11?" ) );
        else if( noInterrupt_result->Type == TYPE_LIKELY_DHT22 ) Serial.print( F( "DHT22?" ) );
        else if( noInterrupt_result->Type == TYPE_ANALOG )
        {
            Serial.print( F( "assumed ANALOG" ) );
        }
    }
    else
    {
        printAnalogLevel( pin_specified );
    }
    Serial.println(); 
}

void check_for_serial_input()
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
        fixInputted( REPLACE, str_talkback, str_logging, NOT_TO_END_OF_STRING );
        fixInputted( REPLACE, str_pin_set, str_set_pin, NOT_TO_END_OF_STRING );
        fixInputted( REPLACE, str_cycle_power, str_power_cycle, NOT_TO_END_OF_STRING );
        fixInputted( REPLACE, str_pins_read, str_read_pin_dot, TO_END_OF_STRING );
        fixInputted( REPLACE, str_read_pins, str_read_pin_dot, TO_END_OF_STRING );
        fixInputted( REPLACE, str_set_pin_to, str_set_pin, NOT_TO_END_OF_STRING );
        fixInputted( SHORTEN, str_view, 0, 2 );
        fixInputted( SHORTEN, str_sensor, 0, 4 );
        fixInputted( SHORTEN, str_persistent_memory, 0, 4 );
        fixInputted( SHORTEN, str_changes, 0, 2 );
        fixInputted( SHORTEN, str_change, 0, 2 );
        fixInputted( SHORTEN, str_thermostat, 0, 4 );
        fixInputted( SHORTEN, str_fan_auto, 0, 5 );
        fixInputted( SHORTEN, str_fan_on, 0, 5 );
        fixInputted( SHORTEN, str_factory, 0, 4 );

        char *address_str;
        char *data_str;
        char *number_specified_str;
        number_specified_str = strrchr( strFull, ' ' ) + 1;
         Serial.println( strFull );
        if( strstr_P( strFull, str_power_cycle ) ) 
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
        }
        else if( strstr_P( strFull, str_heat_temps ) )
        {
           if( IsValidTemp( number_specified_str, NO_BOUNDS_CHECK ) != NO_TEMP_ENTERED )
           {
                if( ( bool )IsValidTemp( number_specified_str, HEAT_TEMP_BOUNDS_CHECK + ADJUST_START_AND_STOP_LIMITS_TOGETHER ) )
               {
                    if( temp_specified_floated < 0 )
                    {
                        LimitSet( &lower_heat_temp_floated, lower_heat_temp_address, ADJUST );
                    }
                    LimitSet( &upper_heat_temp_floated, upper_heat_temp_address, ADJUST );
                    if( temp_specified_floated >= 0 )
                    {
                        LimitSet( &lower_heat_temp_floated, lower_heat_temp_address, ADJUST );
                    }
               }
           }
           if( logging )
            {
                showHeatSettings();
            }
        }
        else if( strstr_P( strFull, str_cool_temps ) )
        {
           if( IsValidTemp( number_specified_str, NO_BOUNDS_CHECK ) != NO_TEMP_ENTERED )
           {
               if( ( bool )IsValidTemp( number_specified_str, COOL_TEMP_BOUNDS_CHECK + ADJUST_START_AND_STOP_LIMITS_TOGETHER ) )
               {
                    if( temp_specified_floated < 0 )
                        LimitSet( &lower_cool_temp_floated, lower_cool_temp_address, ADJUST );
                    LimitSet( &upper_cool_temp_floated, upper_cool_temp_address, ADJUST );
                    if( temp_specified_floated >= 0 )
                        LimitSet( &lower_cool_temp_floated, lower_cool_temp_address, ADJUST );
               }
           }
           if( logging )
            {
                showCoolSettings();
           }
        }
        else if( strstr_P( strFull, str_all_temps ) )
        {
           if( IsValidTemp( number_specified_str, NO_BOUNDS_CHECK ) != NO_TEMP_ENTERED )
           {
               if( ( bool )IsValidTemp( number_specified_str, BOTH_TEMPS_BOUNDS_CHECK + ADJUST_START_AND_STOP_LIMITS_TOGETHER ) )
               {
                    if( temp_specified_floated < 0 )
                    {
                        LimitSet( &lower_heat_temp_floated, lower_heat_temp_address, ADJUST );
                        LimitSet( &lower_cool_temp_floated, lower_cool_temp_address, ADJUST );
                    }
                    LimitSet( &upper_heat_temp_floated, upper_heat_temp_address, ADJUST );
                    LimitSet( &upper_cool_temp_floated, upper_cool_temp_address, ADJUST );
                    if( temp_specified_floated >= 0 )
                    {
                        LimitSet( &lower_heat_temp_floated, lower_heat_temp_address, ADJUST );
                        LimitSet( &lower_cool_temp_floated, lower_cool_temp_address, ADJUST );
                    }
               }
           }
           if( logging )
            {
                showHeatSettings();
                showCoolSettings();
           }
        }
        else if( strstr_P( strFull, str_heatStartLowTemp ) )//change to heat low start temp with optional numeric: heat high stop temp, cool low stop temp, cool high start temp
        {
           if( IsValidTemp( number_specified_str, NO_BOUNDS_CHECK ) != NO_TEMP_ENTERED )
           {
               if( ( bool )IsValidTemp( number_specified_str, HEAT_TEMP_BOUNDS_CHECK + SINGLE_LIMIT_ONLY ) )
               {
                  if( temp_specified_floated <= upper_heat_temp_floated )
                  {
                    LimitSet( &lower_heat_temp_floated, lower_heat_temp_address, SETFULL );
                  }
                  else 
                  {
                    Serial.print( ( const __FlashStringHelper * )str_heatStopHighTemp );
                    printTooHighLow( LOW );
                  }
                }
           }
            if( logging )
            {
                Serial.print( ( const __FlashStringHelper * )str_heatStartLowTemp );
                Serial.print( F( " now " ) );
                Serial.println( lower_heat_temp_floated, 1 );
            }
        }
        else if( strstr_P( strFull, str_heatStopHighTemp ) )
        {
           if( ( bool )IsValidTemp( number_specified_str, HEAT_TEMP_BOUNDS_CHECK + SINGLE_LIMIT_ONLY ) )
           {
              if( temp_specified_floated >= lower_heat_temp_floated )
              {
                LimitSet( &upper_heat_temp_floated, upper_heat_temp_address, SETFULL );
              }
              else 
              {
                Serial.print( ( const __FlashStringHelper * )str_heatStartLowTemp );
                printTooHighLow( HIGH );
              }
           }
            if( logging )
            {
                Serial.print( ( const __FlashStringHelper * )str_heatStopHighTemp );
                Serial.print( F( " now " ) );
                Serial.println( upper_heat_temp_floated, 1 );
            }
           
        }
        else if( strstr_P( strFull, str_coolStopLowTemp ) )
        {
           if( ( bool )IsValidTemp( number_specified_str, COOL_TEMP_BOUNDS_CHECK + SINGLE_LIMIT_ONLY ) )
           {
              if( temp_specified_floated <= upper_cool_temp_floated )
              {
                LimitSet( &lower_cool_temp_floated, lower_cool_temp_address, SETFULL );
              }
              else 
              {
                Serial.print( ( const __FlashStringHelper * )str_coolStartHighTemp );
                printTooHighLow( LOW );
              }
           }
            if( logging )
            {
                Serial.print( ( const __FlashStringHelper * )str_coolStopLowTemp );
                Serial.print( F( " now " ) );
                Serial.println( lower_cool_temp_floated, 1 );
            }
           
        }
        else if( strstr_P( strFull, str_coolStartHighTemp ) )
        {
           if( ( bool )IsValidTemp( number_specified_str, COOL_TEMP_BOUNDS_CHECK + SINGLE_LIMIT_ONLY ) )
           {
              if( temp_specified_floated >= lower_cool_temp_floated )
              {
                LimitSet( &upper_cool_temp_floated, upper_cool_temp_address, SETFULL );
              }
              else 
              {
                Serial.print( ( const __FlashStringHelper * )str_coolStopLowTemp );
                printTooHighLow( HIGH );
              }
           }
            if( logging )
            {
                Serial.print( ( const __FlashStringHelper * )str_coolStartHighTemp );
                Serial.print( F( " now " ) );
                Serial.println( upper_cool_temp_floated, 1 );
            }
           
        }
        else if( strstr_P( strFull, str_report_master_room_temp ) )
        {
               Serial.print( F( "Humidity (%): " ) );
               Serial.println( _HumidityPercent, 1 );
               Serial.print( F( "Temperature (°C): " ) );
               Serial.println( _TemperatureCelsius, 1 );
                showHeatSettings();
                showCoolSettings();
                Serial.print( F( "Furnace: " ) );
               Serial.println( digitalRead( heat_pin ) );
               Serial.print( F( "Furnace fan: " ) );
               Serial.println( digitalRead( furnace_blower_pin ) );
               Serial.print( F( "Cool: " ) );
               Serial.println( digitalRead( cool_pin ) );
        }
        else if( strstr_P( strFull, str_read_pin ) || strstr_P( strFull, str_pin_read ) )
        {
           if( IsValidPinNumber( number_specified_str, 1 ) )
           {
               for( u8 pin_specified_local = pin_specified; ; pin_specified_local++ )
               {
                    if( !pin_print_and_not_sensor( false, pin_specified_local ) )
                    {
                        Serial.print( pin_specified_local );
                    }
                    if( pin_specified_local < NUM_DIGITAL_PINS && isanoutput( pin_specified_local, false ) )
                    {
                        Serial.print( F( ": out" ) );
                    }
                    else 
                    {
                        Serial.print( F( ": in" ) );
                    }
                    Serial.print( F( "put, logic " ) );
                    Serial.print( digitalRead( pin_specified_local ) );
#ifdef PIN_A0
                    if( ( unsigned long )memchr( analog_pin_list, pin_specified_local, PIN_Amax ) - ( unsigned long )&analog_pin_list[ 0 ] < sizeof( analog_pin_list ) )
                    {
                        printAnalogLevel( pin_specified_local );
                    }
#endif
                    Serial.println();
#ifdef PIN_A0
                    if( ( *number_specified_str != '.' && !( *number_specified_str == ' ' && *( number_specified_str + 1 ) == '.' ) ) || ( memchr( analog_pin_list, pin_specified_local, PIN_Amax ) == &analog_pin_list[ sizeof( analog_pin_list ) - 1 ] ) ) break;
                    if( pin_specified_local == NUM_DIGITAL_PINS - 1 ) Serial.println( F( "Remaining analog inputs:" ) );
#else
                    if( ( *number_specified_str != '.' && !( *number_specified_str == ' ' && *( number_specified_str + 1 ) == '.' ) ) || ( pin_specified_local == NUM_DIGITAL_PINS - 1 ) ) break;
#endif
               }
           }
        }
        else if( strstr_P( strFull, str_set_pin_high ) )
        {
           if( IsValidPinNumber( number_specified_str, 0 ) )
           {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
               {
                  if( isanoutput( pin_specified, true ) )
                  {
                        if( ( pin_specified == power_cycle_pin || pin_specified == furnace_blower_pin || pin_specified == heat_pin || pin_specified == cool_pin ) && number_specified_str[ strlen( number_specified_str ) - 1 ] != '!' )
                            refusedNo_exclamation();
                        else 
                        {
                             digitalWrite( pin_specified, HIGH );
                             IfReservedPinGettingForced( HIGH );
                             if( logging )
                             {
                                if( pin_print_and_not_sensor( true, pin_specified ) ) // && !( pin_specified == power_cycle_pin || pin_specified == furnace_blower_pin || pin_specified == heat_pin || pin_specified == cool_pin ) )
                                {
                                    Serial.print( F( "logic 1" ) );
                                    u8 pinState = digitalRead( pin_specified );
                                    if( pinState == LOW ) Serial.print( F( ". Is pin shorted to logic 0?" ) );
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
        }
        else if( strstr_P( strFull, str_set_pin_low ) )
        {
           if( IsValidPinNumber( number_specified_str, 0 ) )
           {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
               {
                  if( isanoutput( pin_specified, true ) )
                  {
                        if( ( pin_specified == power_cycle_pin || pin_specified == furnace_blower_pin || pin_specified == heat_pin || pin_specified == cool_pin ) && number_specified_str[ strlen( number_specified_str ) - 1 ] != '!' )
                            refusedNo_exclamation();
                        else 
                        {
                            digitalWrite( pin_specified, LOW );
                            IfReservedPinGettingForced( LOW );
                             if( logging )
                             {
                                if( pin_print_and_not_sensor( true, pin_specified ) ) // && !( pin_specified == power_cycle_pin || pin_specified == furnace_blower_pin || pin_specified == heat_pin || pin_specified == cool_pin ) )
                                {
                                    Serial.print( F( "logic 0" ) );
                                    u8 pinState = digitalRead( pin_specified );
                                    if( pinState == HIGH ) Serial.print( F( ". Is pin shorted to logic 1?" ) );
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
        }
        else if( strstr_P( strFull, str_set_pin_output ) )
        {
           if( IsValidPinNumber( number_specified_str, 0 ) )
           {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )
               {
                    if( pin_specified != SERIAL_PORT_HARDWARE )
                    {
                        if( ( pin_specified == power_cycle_pin || pin_specified == furnace_blower_pin || pin_specified == heat_pin || pin_specified == cool_pin ) && number_specified_str[ strlen( number_specified_str ) - 1 ] != '!' )
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
                            if( pin_print_and_not_sensor( true, pin_specified ) )
                            {
                                 Serial.print( F( "output, logic " ) );
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
        }
        else if( strstr_P( strFull, str_set_pin_input_with_pullup ) )
        {
           if( IsValidPinNumber( number_specified_str, 0 ) )
           {
               for( ; pin_specified < NUM_DIGITAL_PINS; pin_specified++ )//sketch footprint too large on some boards to set analog only input pins with pullups, so we'll save space by not allowing that abnormal option at all.
               {
                    if( pin_specified != SERIAL_PORT_HARDWARE )
                    {
                          if( !refuseInput() )
                          {
                             pinMode( pin_specified, INPUT_PULLUP );
                             if( logging )
                             {
                                if( pin_print_and_not_sensor( true, pin_specified ) ) Serial.print( F( "input & pullup" ) );
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
        }
        else if( strstr_P( strFull, str_set_pin_input ) )
        {
           if( IsValidPinNumber( number_specified_str, 0 ) )
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
                                if( pin_print_and_not_sensor( true, pin_specified ) )
                                {
                                    Serial.print( F( "input" ) );
                                    if( pinState == HIGH ) Serial.print( F( " apparent pullup: pin shows logic 1 level!" ) );
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
        }
        else if( strstr_P( strFull, str_ther ) )
        {
            if( strlen( strFull ) == 4 ) goto showThermostatSetting;
            if( strFull[ 4 ] != ' ' ) goto notValidCommand;
#if not defined ( __LGT8FX8E__ ) && not defined ( ARDUINO_AVR_YUN ) && not defined ( ARDUINO_AVR_LEONARDO ) && not defined ( ARDUINO_AVR_LEONARDO_ETH ) && not defined ( ARDUINO_AVR_MICRO ) && not defined ( ARDUINO_AVR_ESPLORA ) && not defined ( ARDUINO_AVR_LILYPAD_USB ) && not defined ( ARDUINO_AVR_YUNMINI ) && not defined ( ARDUINO_AVR_INDUSTRIAL101 ) && not defined ( ARDUINO_AVR_LININO_ONE )
           if( strFull[ 5 ] == 'a' )
           {
                if( !( DHTfunctionResultsArray[ outdoor_temp_sensor1_pin - 1 ].ErrorCode == DEVICE_READ_SUCCESS || DHTfunctionResultsArray[ outdoor_temp_sensor1_pin - 1 ].Type == TYPE_ANALOG || DHTfunctionResultsArray[ outdoor_temp_sensor2_pin - 1 ].ErrorCode == DEVICE_READ_SUCCESS || DHTfunctionResultsArray[ outdoor_temp_sensor2_pin - 1 ].Type == TYPE_ANALOG ) )
                {
                    DHTresult* noInterrupt_result = ( DHTresult* )( FetchTemp( outdoor_temp_sensor1_pin, RECENT ) );
                    if( !( noInterrupt_result->ErrorCode == DEVICE_READ_SUCCESS || noInterrupt_result->Type == TYPE_ANALOG ) ) noInterrupt_result = ( DHTresult* )( FetchTemp( outdoor_temp_sensor2_pin, RECENT ) );
                    if( !( noInterrupt_result->ErrorCode == DEVICE_READ_SUCCESS || noInterrupt_result->Type == TYPE_ANALOG ) )
                    {
                        Serial.println( F( "NO CHANGE as outdoor sensors not reporting." ) );
/*  In order to save flash space this is commented out
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
*/
                        goto showThermostatSetting;
                    }
                }
           }
           if( !( strFull[ 5 ] == 'a' ) && !( strFull[ 5 ] == 'h' ) && !( strFull[ 5 ] == 'c' ) && !( strFull[ 5 ] == 'o' ) )
           {
                Serial.println( F( "That space you entered also then requires a valid mode. The only valid characters allowed after that space are the options lower case a, o, h, or c. They mean auto, off, heat, and cool and may be fully spelled out" ) );
#else
           if( !( strFull[ 5 ] == 'h' ) && !( strFull[ 5 ] == 'c' ) && !( strFull[ 5 ] == 'o' ) )
           {
                Serial.println( F( "Valid mode missing after the space: h, c, o (heat/cool/off or may be spelled out)" ) ); //Trying to save space with these boards
#endif
            goto showThermostatSetting;
           }
           thermostat_mode = strFull[ 5 ];
            Serial.print( F( "Thermostat being set to " ) );
            printThermoModeWord( thermostat_mode, true );
           if( thermostat_mode == 'o' ) 
           {
                digitalWrite( heat_pin, LOW );
                heat_state = false;
                timer_alert_furnace_sent = 0;
                digitalWrite( cool_pin, LOW );
                cool_state = false;
           }
            timer_alert_furnace_sent = 0;           
#ifndef __LGT8FX8E__
           EEPROM.update( thermostat_mode_address, thermostat_mode );
#else
           EEPROMupdate( thermostat_mode_address, thermostat_mode );
#endif
//            goto after_change_thermostat;
            if( false )
            {
showThermostatSetting:;
                Serial.print( F( "Thermostat is " ) );
                printThermoModeWord( thermostat_mode, true );
            }
//after_change_thermostat:;
           
        }
        else if( strstr_P( strFull, str_fan_a ) || strstr_P( strFull, str_fan_o ) )
        {
           fan_mode = strFull[ ( u8 )( strlen_P( str_fan_ ) ) ];
           Serial.print( F( "Fan being set to " ) );
           if( fan_mode == 'o' )
            {
                Serial.println( F( "on" ) );
                digitalWrite( furnace_blower_pin, HIGH );
            }
           else
           {
                Serial.println( F( "auto" ) );
                digitalWrite( furnace_blower_pin, LOW );
           }
#ifndef __LGT8FX8E__
           EEPROM.update( fan_mode_address, fan_mode );
#else
           EEPROMupdate( fan_mode_address, fan_mode );
#endif           
        }
        else if( strstr_P( strFull, str_fan ) )
        {
            if( strchr( &strFull[ 3 ], ' ' ) )
            {
#if not defined ( __LGT8FX8E__ ) && not defined ( ARDUINO_AVR_YUN ) && not defined ( ARDUINO_AVR_LEONARDO ) && not defined ( ARDUINO_AVR_LEONARDO_ETH ) && not defined ( ARDUINO_AVR_MICRO ) && not defined ( ARDUINO_AVR_ESPLORA ) && not defined ( ARDUINO_AVR_LILYPAD_USB ) && not defined ( ARDUINO_AVR_YUNMINI ) && not defined ( ARDUINO_AVR_INDUSTRIAL101 ) && not defined ( ARDUINO_AVR_LININO_ONE )
                Serial.println( F( "That space you entered also then requires a valid mode. The only valid characters allowed after that space are the options lower case a or o. They mean auto and on and optionally may be spelled out completely" ) );
#else
                Serial.println( F( "The only valid characters allowed after that space are the options lower case a or o (auto/on or may be spelled out)" ) );
#endif
            }
            Serial.print( F( "Fan is " ) );
            if( fan_mode == 'a' ) Serial.println( F( "auto" ) );
            else if( fan_mode == 'o' ) Serial.println( F( "on" ) );
           
        }
        else if( strstr_P( strFull, str_logging_temp_ch_o ) )
        {
           Serial.print( F( "Talkback for logging temp changes being turned o" ) );
           u8 charat = ( u8 )( strrchr( strFull, ' ' ) + 2 - strFull );
           if( strFull[ charat ] == 'n' ) logging_temp_changes = true;
           else logging_temp_changes = false;
#ifndef __LGT8FX8E__
           EEPROM.update( logging_temp_changes_address, ( uint8_t )logging_temp_changes );
#else
           EEPROMupdate( logging_temp_changes_address, ( uint8_t )logging_temp_changes );
#endif
           if( logging_temp_changes ) Serial.println( strFull[ charat ] );
           else  Serial.println( F( "ff" ) );
           
        }
        else if( strstr_P( strFull, str_logging_temp_ch ) )
        {
           Serial.print( F( "Talkback for logging temp changes is turned o" ) );
           if( logging_temp_changes ) Serial.println( F( "n" ) );
           else  Serial.println( F( "ff" ) );
           
        }
        else if( strstr_P( strFull, str_logging_o ) )
        {
           Serial.print( F( "Talkback for logging being turned o" ) );
           u8 charat = ( u8 )( strrchr( strFull, ' ' ) + 2 - strFull );
           if( strFull[ charat ] == 'n' ) logging = true;
           else logging = false;
#ifndef __LGT8FX8E__
           EEPROM.update( logging_address, ( uint8_t )logging );
#else
           EEPROMupdate( logging_address, ( uint8_t )logging );
#endif
           if( logging ) Serial.println( strFull[ charat ] );
           else  Serial.println( F( "ff" ) );
           
        }
        else if( strstr_P( strFull, str_logging ) )
        {
           Serial.print( F( "Talkback for logging is turned o" ) );
           if( logging ) Serial.println( F( "n" ) );
           else  Serial.println( F( "ff" ) );
           
        }
        else if( strstr_P( strFull, str_vi_pers ) )
        {
          address_str = strchr( &strFull[ 7 ], ' ' ) + 1;//.substring( address_str.indexOf( ' ' )+ 1 );
          unsigned int address_start = atoi( address_str );
          address_str = strrchr( address_str, ' ' ) + 1;//address_str.substring( address_str.indexOf( ' ' )+ 1 );
          unsigned int address_end = atoi( address_str );//.toInt();
          if( address_start < 0 || address_start >= EEPROMlength || address_end < 0 || address_end >= EEPROMlength )
          {
            Serial.print( F( "Out of range. Each address can only be 0 to " ) );
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
               if( data > 31 && data < 255 )
               {
                   Serial.print( F( "  " ) );
                   Serial.print( ( char )data );
               }
               Serial.println();
             }
          }
           
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
              EEPROMupdate( address, data );
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
                          EEPROMupdate( address, 0 );
#endif
                        }
                    }
                  }
                }
                }
                else
                {
                Serial.println( F( "Data string too long to fit there in EEPROM." ) );
                }
                
              }
              else
              {
                Serial.println( F( "Invalid data entered" ) );
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
              EEPROMupdate( address, data );
#endif
              Serial.print( F( " newly stored data: " ) );
              Serial.println( EEPROM.read( address ) );
          }
           
        }
        else if( strstr( strFull, "assign pins" ) )
        {
            assign_pins( ALREADY_RUNNING );
            digitalWrite( heat_pin, heat_state ); 
            digitalWrite( cool_pin, cool_state );
            Serial.println( F( "help will show new pin assignments." ) );
        }
        else if( strstr_P( strFull, str_vi_fact ) ) // The Leonardo does not have enough room in PROGMEM for this feature
        {
#ifdef __AVR_ATmega32U4__
            Serial.println( F( "Not supported for this board due to limited memory space. See source code instead" ) );//Don't know how to detect atmega168 boards
#else
            print_factory_defaults();
#endif
           
        }
#ifdef RESTORE_FACTORY_DEFAULTS
        else if( strstr_P( strFull, str_reset ) )
        {
           restore_factory_defaults();
        }
#endif
        else if( strstr_P( strFull, str_test_alert ) )
        {
           Serial.println( F( "time_stamp_this ALERT test as requested" ) );
        }
        else if( strstr_P( strFull, str_sens_read ) || strstr_P( strFull, str_read_sens ) )
        {
           int i = 9;
           while( strFull[ i ] == ' ' && strlen( strFull ) > i ) i++;
             if( IsValidPinNumber( &strFull[ i ], TYPE_ANALOG ) )
             {
                for( u8 pin_specified_local = pin_specified; ; pin_specified_local++ )
                {
                    print_the_pin_and_sensor_reading( pin_specified++, KY013 );
#ifdef PIN_A0
                    if( ( *number_specified_str != '.' && !( *number_specified_str == ' ' && *( number_specified_str + 1 ) == '.' ) ) || ( memchr( analog_pin_list, pin_specified_local, PIN_Amax ) == &analog_pin_list[ sizeof( analog_pin_list ) - 1 ] ) ) break;
                    if( pin_specified_local == NUM_DIGITAL_PINS - 1 ) Serial.println( F( "Remaining analog inputs:" ) );
#else
                    if( ( *number_specified_str != '.' && !( *number_specified_str == ' ' && *( number_specified_str + 1 ) == '.' ) ) || ( pin_specified_local == NUM_DIGITAL_PINS - 1 ) ) break;
#endif
               }
             }
        }
        else
        {
          if( !strstr_P( strFull, str_help ) )
          {
notValidCommand:;
                 Serial.println( F( "Not a valid command, note they are cAsE sEnSiTiVe" ) );
                Serial.println(); 
          }
          printBasicInfo();
        }
      strFull[ 0 ] = 0;
}

u8 last_three_temps_index = 0;
float old_getCelsius_temp = 0;
float oldtemp = 0;
unsigned long timeOfLastSensorTimeoutError = 0;
float temp_minimus;

void heat_on_loop()
{
   if( !heat_state )
   {
          if( last_three_temps[ 0 ] < lower_heat_temp_floated && last_three_temps[ 1 ] < lower_heat_temp_floated && last_three_temps[ 2 ] < lower_heat_temp_floated && last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ] > lower_heat_temp_floated )
          {
                digitalWrite( heat_pin, HIGH ); 
//                digitalWrite( cool_pin, LOW );
              heat_state = true;
//              cool_state = false;
              timer_alert_furnace_sent = 0;
              if( logging )
              {
                Serial.println( F( "time_stamp_this Furnace on" ) );
              }
              setFurnaceEffectivenessTime();
              temp_minimus = min( last_three_temps[ 0 ], last_three_temps[ 1 ] );//Be sure to set this when heat is manually operated by pin manipulation or any other means also
              temp_minimus = min( temp_minimus, last_three_temps[ 2 ] );
          }
   }
   else
   {
       if( last_three_temps[ 0 ] > upper_heat_temp_floated && last_three_temps[ 1 ] > upper_heat_temp_floated && last_three_temps[ 2 ] > upper_heat_temp_floated )
       {
            digitalWrite( heat_pin, LOW );
              heat_state = false;
              timer_alert_furnace_sent = 0;
            if( logging )
            {
                Serial.println( F( "time_stamp_this Furnace off" ) );
            }
       }
       else if( ( !timer_alert_furnace_sent && heat_started_temp_x_3 > last_three_temps[ 0 ] + last_three_temps[ 1 ] + last_three_temps[ 2 ] ) && ( millis() - check_furnace_effectiveness_time > minutes_furnace_should_be_effective_after * 60000 ) )
       {
//                    // check_furnace_effectiveness_time is set when the heat is ON'd (turned on)
        Serial.print( F( "time_stamp_this ALERT Furnace not heating enough after allowing " ) );                          
        Serial.print( ( u8 )( ( millis() - check_furnace_effectiveness_time ) / 60000 ) ); //GRANTED, THIS TIME IS NOT PERFECT PER THE VARIABLE NOMENCLATURE, SINCE THERE IS A BIT MORE OR LESS THAN 1000 ARDUINO MILLIS IN A SECOND
        Serial.println( F( " minutes" ) );
        timer_alert_furnace_sent = loop_cycles_to_skip_between_alert_outputs;
/*                    send an alert output if furnace not working: temperature dropping even though heat got turned on.  the way to determine this is to have a flag of whether the alert output is already sent;
*                     That flag will unconditionally get reset in the following situations: thermostat_mode setting gets changed, upper_heat_temp_floated or lower_heat_temp_floated gets changed, room temperature rises while heat is on, 
*/
       }
       else if( timer_alert_furnace_sent && ( oldtemp - temp_minimus > 2 ) )//enough temperature rise (2C) from temperature minimus which means furnace is assumed working so we say forget that alert was sent
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
//                digitalWrite( heat_pin, LOW ); 
                digitalWrite( cool_pin, HIGH );
//              heat_state = false;
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
//                digitalWrite( heat_pin, LOW );
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
#if not defined ( RESTORE_FACTORY_DEFAULTS ) || ( not defined ( __LGT8FX8E__ ) && not defined ( ARDUINO_AVR_YUN ) && not defined ( ARDUINO_AVR_LEONARDO ) && not defined ( ARDUINO_AVR_LEONARDO_ETH ) && not defined ( ARDUINO_AVR_MICRO ) && not defined ( ARDUINO_AVR_ESPLORA ) && not defined ( ARDUINO_AVR_LILYPAD_USB ) && not defined ( ARDUINO_AVR_YUNMINI ) && not defined ( ARDUINO_AVR_INDUSTRIAL101 ) && not defined ( ARDUINO_AVR_LININO_ONE ) )
    if( fresh_powerup && logging )
    {
        if( Serial )
        {
            printBasicInfo();
            fresh_powerup = false;
        }
    }
    else fresh_powerup = false;
        DHTresult* noInterrupt_result = ( DHTresult* )( FetchTemp( primary_temp_sensor_pin, RECENT ) ); 
        if( noInterrupt_result->ErrorCode != DEVICE_READ_SUCCESS && noInterrupt_result->Type != TYPE_ANALOG ) noInterrupt_result = ( DHTresult* )( FetchTemp( secondary_temp_sensor_pin, RECENT ) );
        if( ( noInterrupt_result->ErrorCode == DEVICE_READ_SUCCESS && noInterrupt_result->Type != TYPE_ANALOG ) || noInterrupt_result->Type == TYPE_ANALOG )
        {
            timeOfLastSensorTimeoutError = 0;
//            if( noInterrupt_result->TemperatureCelsius & 0x8000 ) _TemperatureCelsius = 0 - ( float )( ( float )( noInterrupt_result->TemperatureCelsius & 0x7FFF )/ 10 );
//            else _TemperatureCelsius = ( float )( ( float )( noInterrupt_result->TemperatureCelsius & 0x7FFF )/ 10 );
//            if( noInterrupt_result->TemperatureCelsius & 0x8000 ) _TemperatureCelsius = 0 - ( float )( ( float )( noInterrupt_result->TemperatureCelsius & 0x7FFF )/ 10 );
            _TemperatureCelsius = ( float )( ( float )( noInterrupt_result->TemperatureCelsius )/ 10 );
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
                    if( noInterrupt_result == &DHTfunctionResultsArray[ ( primary_temp_sensor_pin & 0x7F ) - 1 ] ) Serial.print( F( "prim" ) );
                    else Serial.print( F( "second" ) );
                    Serial.println( F( "ary sensor" ) );
                }
                old_getCelsius_temp = last_three_temps[ last_three_temps_index ];
                if( heat_state && !timer_alert_furnace_sent && newtemp < oldtemp ) //store temp at every small drop when heat is calling && !timer_alert_furnace_sent
                {
                      temp_minimus = min( temp_minimus, last_three_temps[ 0 ] ); //Be sure to set this when heat is manually operated by pin manipulation or any other means also
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
                digitalWrite( heat_pin, LOW ); 
                digitalWrite( cool_pin, LOW );
                heat_state = false;
                timer_alert_furnace_sent = 0;
                cool_state = false;
            }
    #if not defined ( __LGT8FX8E__ ) && not defined ( ARDUINO_AVR_YUN ) && not defined ( ARDUINO_AVR_LEONARDO ) && not defined ( ARDUINO_AVR_LEONARDO_ETH ) && not defined ( ARDUINO_AVR_MICRO ) && not defined ( ARDUINO_AVR_ESPLORA ) && not defined ( ARDUINO_AVR_LILYPAD_USB ) && not defined ( ARDUINO_AVR_YUNMINI ) && not defined ( ARDUINO_AVR_INDUSTRIAL101 ) && not defined ( ARDUINO_AVR_LININO_ONE )
                else if( thermostat_mode == 'a' )
                {
                    noInterrupt_result = ( DHTresult* )( FetchTemp( outdoor_temp_sensor1_pin, RECENT ) ); 
                    if( noInterrupt_result->ErrorCode != DEVICE_READ_SUCCESS && noInterrupt_result->Type != TYPE_ANALOG ) noInterrupt_result = ( DHTresult* )( FetchTemp( outdoor_temp_sensor2_pin, RECENT ) );
                    if( noInterrupt_result->ErrorCode == DEVICE_READ_SUCCESS || noInterrupt_result->Type == TYPE_ANALOG )
                    {
                        O_TemperatureCelsius = ( float )( ( float )( noInterrupt_result->TemperatureCelsius )/ 10 );
                        if( O_TemperatureCelsius >= _TemperatureCelsius ) cool_on_loop();//get outdoor temp, use second sensor if first fails, get indoor temp same way, if indoor < outdoor cool_on_loop();
                        else heat_on_loop();
                    }
                    else heat_on_loop();
                }
    #endif
            else if( thermostat_mode == 'h' ) heat_on_loop(); //This heat loop is all that the WeMo/TTGO XI can do as thermostat
            else if( thermostat_mode == 'c' ) cool_on_loop();
        }
        else
        {
            timeOfLastSensorTimeoutError++;//loop counter when digital sensor is timing out
            if( timeOfLastSensorTimeoutError % 100 == 2 )//every 100 loops beginning with the 2nd loop
            {
                Serial.print( F( "time_stamp_this " ) );
                if( timeOfLastSensorTimeoutError > loop_cycles_to_skip_between_alert_outputs )
                {
                    Serial.print( F( "ALERT " ) );//These ALERT prefixes get added after consecutive 100 timeout fails
                    timeOfLastSensorTimeoutError = 1;
                }
                Serial.println( F( "sensor TIMEOUT" ) );
            }
        }
        for( u8 i = 0; i < 4; i++ )
        {
            check_for_serial_input();
            delay( 500 );
        }
#endif
}
