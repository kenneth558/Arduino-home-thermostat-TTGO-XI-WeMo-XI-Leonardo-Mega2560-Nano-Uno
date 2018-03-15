#ifdef RESTORE_FACTORY_DEFAULTS
void restore_factory_defaults()
{
    Serial.print( F( "Storing thermostat-related.  Primary temperature sensor goes on pin " ) );
    Serial.print( factory_setting_primary_temp_sensor_pin );
    Serial.print( F( ", secondary sensor on pin " ) );
    Serial.print( factory_setting_secondary_temp_sensor_pin );
    Serial.print( F( ", heat controlled by pin " ) );
    Serial.print( factory_setting_heat_pin );
    Serial.print( F( ", furnace blower controlled by pin " ) );
    Serial.print( factory_setting_furnace_blower_pin );
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
    Serial.print( ( const __FlashStringHelper * )str_heatStartLowTemp );
    Serial.print( '=' );
    Serial.print( factory_setting_lower_heat_temp_floated, 1 );
    Serial.print( F( ", heat stop high temp=" ) );
    Serial.print( factory_setting_upper_heat_temp_floated, 1 );
    Serial.print( F( ", cool stop low temp=" ) );
    Serial.print( factory_setting_lower_cool_temp_floated, 1 );
    Serial.print( F( ", cool start high temp=" ) );
    Serial.print( factory_setting_upper_cool_temp_floated, 1 );
    Serial.print( F( ", heat mode=" ) );
    printThermoModeWord( factory_setting_thermostat_mode, true );
    Serial.print( F( ", fan mode=" ) );
    Serial.println( factory_setting_fan_mode );
#ifndef __LGT8FX8E__
    EEPROM.update( primary_temp_sensor_pin_address, factory_setting_primary_temp_sensor_pin );
#else
    EEPROMupdate( primary_temp_sensor_pin_address, factory_setting_primary_temp_sensor_pin );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( heat_pin_address, factory_setting_heat_pin );
#else
    EEPROMupdate( heat_pin_address, factory_setting_heat_pin );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( furnace_blower_pin_address, factory_setting_furnace_blower_pin );
#else
    EEPROMupdate( furnace_blower_pin_address, factory_setting_furnace_blower_pin );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( power_cycle_pin_address, factory_setting_power_cycle_pin );
#else
    EEPROMupdate( power_cycle_pin_address, factory_setting_power_cycle_pin );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( logging_address, factory_setting_logging_setting );
#else
    EEPROMupdate( logging_address, factory_setting_logging_setting );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( logging_temp_changes_address, factory_setting_logging_temp_changes_setting );
#else
    EEPROMupdate( logging_temp_changes_address, factory_setting_logging_temp_changes_setting );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( thermostat_mode_address, factory_setting_thermostat_mode );
#else
    EEPROMupdate( thermostat_mode_address, factory_setting_thermostat_mode );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( fan_mode_address, factory_setting_fan_mode );
#else
    EEPROMupdate( fan_mode_address, factory_setting_fan_mode );
#endif
//These two must be put's because their data types are longer than one.  Data types of length one can be either updates or puts.
    short factory_setting_lower_heat_temp_shorted_times_ten = ( short )( factory_setting_lower_heat_temp_floated * 10 );
    short factory_setting_upper_heat_temp_shorted_times_ten = ( short )( factory_setting_upper_heat_temp_floated * 10 );
    short factory_setting_lower_cool_temp_shorted_times_ten = ( short )( factory_setting_lower_cool_temp_floated * 10 );
    short factory_setting_upper_cool_temp_shorted_times_ten = ( short )( factory_setting_upper_cool_temp_floated * 10 );
#ifndef __LGT8FX8E__
    EEPROM.put( lower_heat_temp_address, factory_setting_lower_heat_temp_shorted_times_ten );
#else
    EEPROMupdate( lower_heat_temp_address, ( u8 )factory_setting_lower_heat_temp_shorted_times_ten );
    EEPROMupdate( lower_heat_temp_address + 1, ( u8 )( factory_setting_lower_heat_temp_shorted_times_ten >> 8 ) );
#endif
#ifndef __LGT8FX8E__
    EEPROM.put( upper_heat_temp_address, factory_setting_upper_heat_temp_shorted_times_ten );
#else
    EEPROMupdate( upper_heat_temp_address, ( u8 )factory_setting_upper_heat_temp_shorted_times_ten );
    EEPROMupdate( upper_heat_temp_address + 1, ( u8 )( factory_setting_upper_heat_temp_shorted_times_ten >> 8 ) );
#endif
#ifndef __LGT8FX8E__
    EEPROM.put( lower_cool_temp_address, factory_setting_lower_cool_temp_shorted_times_ten );
#else
    EEPROMupdate( lower_cool_temp_address, ( u8 )factory_setting_lower_cool_temp_shorted_times_ten );
    EEPROMupdate( lower_cool_temp_address + 1, ( u8 )( factory_setting_lower_cool_temp_shorted_times_ten >> 8 ) );
#endif
#ifndef __LGT8FX8E__
    EEPROM.put( upper_cool_temp_address, factory_setting_upper_cool_temp_shorted_times_ten );
#else
    EEPROMupdate( upper_cool_temp_address, ( u8 )factory_setting_upper_cool_temp_shorted_times_ten );
    EEPROMupdate( upper_cool_temp_address + 1, ( u8 )( factory_setting_upper_cool_temp_shorted_times_ten >> 8 ) );
#endif

#ifndef __LGT8FX8E__
    EEPROM.update( secondary_temp_sensor_pin_address, factory_setting_secondary_temp_sensor_pin );
#else
    EEPROMupdate( secondary_temp_sensor_pin_address, factory_setting_secondary_temp_sensor_pin );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( outdoor_temp_sensor1_pin_address, factory_setting_outdoor_temp_sensor1_pin );
#else
    EEPROMupdate( outdoor_temp_sensor1_pin_address, factory_setting_outdoor_temp_sensor1_pin );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( outdoor_temp_sensor2_pin_address, factory_setting_outdoor_temp_sensor2_pin );
#else
    EEPROMupdate( outdoor_temp_sensor2_pin_address, factory_setting_outdoor_temp_sensor2_pin );
#endif

// no EEPROM updates allowed while in interrupts
    logging = factory_setting_logging_setting;
    logging_temp_changes = factory_setting_logging_temp_changes_setting;
    thermostat_mode = factory_setting_thermostat_mode;
    timer_alert_furnace_sent = 0;
// gets done later    fan_mode = factory_setting_fan_mode;
    
    lower_heat_temp_floated = factory_setting_lower_heat_temp_floated;
    upper_heat_temp_floated = factory_setting_upper_heat_temp_floated;
    upper_cool_temp_floated = factory_setting_upper_cool_temp_floated;    
    lower_cool_temp_floated = factory_setting_lower_cool_temp_floated;    
#ifndef __LGT8FX8E__
    EEPROM.put( 0, ( NUM_DIGITAL_PINS + 1 ) * 3 );//Tattoo the board
#else
    EEPROMupdate( 0, ( u8 )( ( NUM_DIGITAL_PINS + 1 ) * 3 ) );
    EEPROMupdate( 1, ( u8 )( ( ( NUM_DIGITAL_PINS + 1 ) * 3 ) >> 8 ) );
#endif
/*
#ifdef PIN_A0
    #ifndef __LGT8FX8E__
        EEPROM.put( calibration_offset_array_start_address_first_byte, ( u16 )lower_cool_temp_address - sizeof( analog_pin_list ) );//This is done so we can more easily change location of this array in future revision, maybe to an end-of-EEPROM-refenced location
    #else
        EEPROMupdate( calibration_offset_array_start_address_first_byte, ( u8 )( lower_cool_temp_address - sizeof( analog_pin_list ) ) );
        EEPROMupdate( calibration_offset_array_start_address_first_byte + 1, ( u8 )( lower_cool_temp_address - sizeof( analog_pin_list ) >> 8 ) );
    #endif
    calibration_offset = EEPROM.read( calibration_offset_array_start_address_first_byte );
    calibration_offset += ( ( u16 )EEPROM.read( calibration_offset_array_start_address_first_byte + 1 ) ) << 8;
    u8 i;
    for( i = 0; i < sizeof( analog_pin_list ); i++ )
    {
#ifndef __LGT8FX8E__
        EEPROM.update( calibration_offset + i, 225 ); //the 225 unsigned equates to -31 signed, adjust to your heart's content for a default analog calibration adjust based on supply voltage.  Tweak each sensor individually in the array for further accuracy
//The calibration offset is applied in both of two ways: 80% is applied up front to the raw device reading in a "regressive-differential-from-midpoint" style, the other 20% is applied after computation to the resultant temperature directly.    The 80-20 split is absolutely total guesswork on my part.
//Calibration offset values from 0 to 127 move the temperature positive; values from 128 to 255 move it negative.  Both the 80% and 20% portions affects the resultant in the same direction as each other.
#else
        EEPROMupdate( calibration_offset + i, 225 );
#endif
//        Serial.println( analog_pin_list[ i ] );
    }
#endif
*/
//    Serial.println( logging_address );//
//    Serial.println( upper_heat_temp_address );// = logging_address - sizeof( short );//EEPROMlength - 2;
//    Serial.println( lower_heat_temp_address );// = upper_heat_temp_address - sizeof( short );//EEPROMlength - 3;
//    Serial.println( logging_temp_changes_address );// = lower_heat_temp_address - sizeof( boolean );//EEPROMlength - 4;
//    Serial.println( upper_cool_temp_address );// = logging_temp_changes_address - sizeof( short );
//    Serial.println( lower_cool_temp_address );// = upper_cool_temp_address - sizeof( short );
//    Serial.println( calibration_offset );// = upper_cool_temp_address - sizeof( short );
//    Serial.println( i );// = upper_cool_temp_address - sizeof( short );
    assign_pins( NOT_RUNNING );

    Serial.println( F( "Done. Unplug board now if desired." ) );
    delay( 10000 );
      printBasicInfo();
}
#endif
