#ifdef RESTORE_FACTORY_DEFAULTS
void restore_factory_defaults()
{
// no EEPROM updates allowed while in interrupts
    primary_temp_sensor_pin = factory_setting_primary_temp_sensor_pin;
    heat_pin = factory_setting_heat_pin;
    furnace_blower_pin = factory_setting_furnace_blower_pin;
    cool_pin = factory_setting_cool_pin;
    power_cycle_pin = factory_setting_power_cycle_pin;
    logging = factory_setting_logging_setting;
    logging_temp_changes = factory_setting_logging_temp_changes_setting;
    secondary_temp_sensor_pin = factory_setting_secondary_temp_sensor_pin;
    outdoor_temp_sensor1_pin = factory_setting_outdoor_temp_sensor1_pin;
    outdoor_temp_sensor2_pin = factory_setting_outdoor_temp_sensor2_pin;
    thermostat_mode = factory_setting_thermostat_mode;
    timer_alert_furnace_sent = 0;
    fan_mode = factory_setting_fan_mode;
    
    lower_heat_temp_floated = factory_setting_lower_heat_temp_floated;
    upper_heat_temp_floated = factory_setting_upper_heat_temp_floated;
    upper_cool_temp_floated = factory_setting_upper_cool_temp_floated;    
    lower_cool_temp_floated = factory_setting_lower_cool_temp_floated;    
    if( fan_mode == 'o' ) digitalWrite( furnace_blower_pin, HIGH );
    else digitalWrite( furnace_blower_pin, LOW );
    if( thermostat_mode == 'o' || thermostat_mode == 'c' )
    {
        digitalWrite( heat_pin, LOW );
        heat_state = false;
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
    EEPROM.update( primary_temp_sensor_address, factory_setting_primary_temp_sensor_pin );
#else
    EEPROMupdate( primary_temp_sensor_address, factory_setting_primary_temp_sensor_pin );
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
    EEPROM.update( power_cycle_address, factory_setting_power_cycle_pin );
#else
    EEPROMupdate( power_cycle_address, factory_setting_power_cycle_pin );
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
    EEPROM.update( secondary_temp_sensor_address, factory_setting_secondary_temp_sensor_pin );
#else
    EEPROMupdate( secondary_temp_sensor_address, factory_setting_secondary_temp_sensor_pin );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( outdoor_temp_sensor1_address, factory_setting_outdoor_temp_sensor1_pin );
#else
    EEPROMupdate( outdoor_temp_sensor1_address, factory_setting_outdoor_temp_sensor1_pin );
#endif
#ifndef __LGT8FX8E__
    EEPROM.update( outdoor_temp_sensor2_address, factory_setting_outdoor_temp_sensor2_pin );
#else
    EEPROMupdate( outdoor_temp_sensor2_address, factory_setting_outdoor_temp_sensor2_pin );
#endif
#ifndef __LGT8FX8E__
    EEPROM.put( 0, ( NUM_DIGITAL_PINS + 1 ) * 3 );//Tattoo the board
#else
    EEPROMupdate( 0, ( u8 )( ( NUM_DIGITAL_PINS + 1 ) * 3 ) );
    EEPROMupdate( 1, ( u8 )( ( ( NUM_DIGITAL_PINS + 1 ) * 3 ) >> 8 ) );
#endif
    Serial.println( F( "Done. Allowing you time to unplug the Arduino if desired." ) );
    delay( 10000 );
      printBasicInfo();
}
#endif
