#ifdef PIN_A0
unsigned short resistor_between_LED_BUILTIN_and_PIN_A0() //default purpose for this is to signify type of line-ends to send with serial communications: MS Windows-style ( resistor connects the pins ) will include a CR with the LF, while non-windows ( no resistor ) only has the LF
{ //Need to detect any device connected to this pin and add delay for it/them
    pinMode( LED_BUILTIN, OUTPUT );
    digitalWrite( LED_BUILTIN, HIGH );
    delay( 100 );
    pinMode( PIN_A0, OUTPUT );
//                   BEWARE:
//The following line or lines cause[ s ] any DHT device on this pin to go out of detect-ability for a while 
    digitalWrite( PIN_A0, LOW );                                                //A test here for 1 MOhm resistor between pins A0 and LED_BUILTIN while led is high:  make A0 an output, take it low, make an analog input and read it for a high
    pinMode( PIN_A0, INPUT );
    delay( 1 );                                                               //allow settling time, but not long enough to trigger a DHT device, thankfully
    if ( analogRead( PIN_A0 ) > 100 )                                           // in breadboard testing, this level needed to be at least 879 for reliable detection of a high if no delay allowance for settling time
    { 
        digitalWrite( LED_BUILTIN, LOW );                                        //A high was used to disable relays and whatnots so they don't get driven during the dht device detection process.  The circuitry for that is the end-user's responsibility
        pinMode( PIN_A0, OUTPUT );                                            
        digitalWrite( PIN_A0, HIGH );                                             //A test here for 1 MOhm resistor between pins A0 and LED_BUILTIN while led is high:  make A0 an output, take it low, make an analog input and read it for a high
        pinMode( PIN_A0, INPUT );
        unsigned short returnvalue = analogRead( PIN_A0 ) + 1;
        delay_if_device_triggered( PIN_A0 );

        pinMode( PIN_A0, OUTPUT );                                           
        digitalWrite( PIN_A0, HIGH );                                             //A test here for 1 MOhm resistor between pins A0 and LED_BUILTIN while led is high:  make A0 an output, take it low, make an analog input and read it for a high
//
        return ( returnvalue );      //Will be 1 to 101 if connected to LED_BUILTIN through resistor
    }
    else
    { 
        digitalWrite( LED_BUILTIN, LOW );                                        //A high was used to disable relays and whatnots so they don't get driven during the dht device detection process.  The circuitry for that is the end-user's responsibility
        pinMode( PIN_A0, OUTPUT );                                             
        digitalWrite( PIN_A0, HIGH );                                             //A test here for 1 MOhm resistor between pins A0 and LED_BUILTIN while led is high:  make A0 an output, take it low, make an analog input and read it for a high
        //delay( 2000 );//Allow time for any DHT device to settle down
        return ( 0 );
    }
}
#endif

#ifdef PIN_A1
unsigned short resistor_between_LED_BUILTIN_and_PIN_A1()//default purpose for this is to signify the verbosity level end-user wants, no resistor = max verbosity, user can utilize various resistor configurations to adjust verbosity
{ 
//  The default ( no resistor ) is for maximum capability of this software product ( no memory fragmentation when devices are changed without rebooting ) which leaves a little less memory for end-user-added functionality.
//Need to detect any device connected to this pin and add delay for it/them
    pinMode( LED_BUILTIN, OUTPUT );
    digitalWrite( LED_BUILTIN, HIGH );
    delay( 100 );
    pinMode( PIN_A1, OUTPUT );                                              
    digitalWrite( PIN_A1, LOW );                                                //A test here for 1 MOhm resistor between pins A1 and LED_BUILTIN while led is high:  make A1 an output, take it low, make an analog input and read it for a high
    pinMode( PIN_A1, INPUT );
    delay( 1 );                                                               //allow settling time
    if ( analogRead( PIN_A1 ) > 100 )                                           // in breadboard testing, this level needed to be at least 879 for reliable detection of a high if no delay allowance for settling time
    { 
        digitalWrite( LED_BUILTIN, LOW );                                        //A high was used to disable relays and whatnots so they don't get driven during the dht device detection process.  The circuitry for that is the end-user's responsibility
        pinMode( PIN_A1, OUTPUT );                                         
        digitalWrite( PIN_A1, HIGH );                                             //A test here for 1 MOhm resistor between pins A1 and LED_BUILTIN while led is high:  make A1 an output, take it low, make an analog input and read it for a high
        pinMode( PIN_A1, INPUT );
        unsigned short returnvalue = analogRead( PIN_A1 ) + 1;
        delay_if_device_triggered( PIN_A1 );
        pinMode( PIN_A1, OUTPUT );                                          
        digitalWrite( PIN_A1, HIGH );                                             //A test here for 1 MOhm resistor between pins A1 and LED_BUILTIN while led is high:  make A1 an output, take it low, make an analog input and read it for a high
        return ( returnvalue );      //Will be 1 to 101 if connected to LED_BUILTIN through resistor
    }
    else
    { 
        pinMode( PIN_A1, OUTPUT );                                             
        digitalWrite( PIN_A1, HIGH );                                             //A test here for 1 MOhm resistor between pins A1 and LED_BUILTIN while led is high:  make A1 an output, take it low, make an analog input and read it for a high
        //delay( 2000 );//Allow time for any DHT device to settle down
        digitalWrite( LED_BUILTIN, LOW );                                        //A high was used to disable relays and whatnots so they don't get driven during the dht device detection process.  The circuitry for that is the end-user's responsibility
        return ( 0 );
    }
}
#endif

#ifdef PIN_A2
unsigned short resistor_between_LED_BUILTIN_and_PIN_A2()//default purpose for this is for end-user to indicate the host system might provide bootup configuration information.  True means to ask host and wait a short while for response
{ //Need to detect any device connected to this pin and add delay for it/them
    pinMode( LED_BUILTIN, OUTPUT );
    digitalWrite( LED_BUILTIN, HIGH );
    delay( 100 );
    pinMode( PIN_A2, OUTPUT );                                               
    digitalWrite( PIN_A2, LOW );                                                //A test here for 1 MOhm resistor between pins A2 and LED_BUILTIN while led is high:  make A2 an output, take it low, make an analog input and read it for a high
    pinMode( PIN_A2, INPUT );
    delay( 1 );                                                               //allow settling time
    if ( analogRead( PIN_A2 ) > 100 )                                           // in breadboard testing, this level needed to be at least 879 for reliable detection of a high if no delay allowance for settling time
    { 
        digitalWrite( LED_BUILTIN, LOW );                                        //A high was used to disable relays and whatnots so they don't get driven during the dht device detection process.  The circuitry for that is the end-user's responsibility
        pinMode( PIN_A2, OUTPUT );                                             
        digitalWrite( PIN_A2, HIGH );                                             //A test here for 1 MOhm resistor between pins A2 and LED_BUILTIN while led is high:  make A2 an output, take it low, make an analog input and read it for a high
        pinMode( PIN_A2, INPUT );
        unsigned short returnvalue = analogRead( PIN_A2 ) + 1;
        delay_if_device_triggered( PIN_A2 );
        pinMode( PIN_A2, OUTPUT );                                             
        digitalWrite( PIN_A2, HIGH );                                             //A test here for 1 MOhm resistor between pins A2 and LED_BUILTIN while led is high:  make A2 an output, take it low, make an analog input and read it for a high
        return ( returnvalue );      //Will be 1 to 101 if connected to LED_BUILTIN through resistor
    }
    else
    { 
        pinMode( PIN_A2, OUTPUT );                                             
        digitalWrite( PIN_A2, HIGH );                                             //A test here for 1 MOhm resistor between pins A2 and LED_BUILTIN while led is high:  make A2 an output, take it low, make an analog input and read it for a high
        //delay( 2000 );//Allow time for any DHT device to settle down
        digitalWrite( LED_BUILTIN, LOW );                                        //A high was used to disable relays and whatnots so they don't get driven during the dht device detection process.  The circuitry for that is the end-user's responsibility
        return ( 0 );
    }
}
#endif
