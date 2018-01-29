
#define DEVICE_NOT_YET_ACCESSED -1 //255
#define DEVICE_FAILS_DURING_INITIALIZE -2  //254
#define DEVICE_FAILS_DURING_INITIALIZE1 -3 //253 common to dht22
#define DEVICE_FAILS_DURING_INITIALIZE2 -4 //252
#define DEVICE_FAILS_DURING_INITIALIZE3 -5 //251
#define DEVICE_FAILS_DURING_INITIALIZE4 -6 //250
#define DEVICE_FAILS_DURING_INITIALIZE5 -7 //249
#define DEVICE_FAILS_DURING_INITIALIZE6 -8 //248
#define DEVICE_FAILS_DURING_INITIALIZE7 -9 //247
#define DEVICE_FAILS_DURING_INITIALIZE8 -10 //246
#define DEVICE_FAILS_DURING_DATA_STREAM -11 //245
#define DEVICE_CRC_ERROR -12 //244
#define DEVICE_READ_SUCCESS -13 //243

#define REFUSED_ROLLOVER_EXPECTED -14
#define REFUSED_INVALID_PIN -15

#define TYPE_KNOWN_DHT11 1
#define TYPE_KNOWN_DHT22 2
#define TYPE_LIKELY_DHT11 3
#define TYPE_LIKELY_DHT22 4


float _TemperatureCelsius;  //GLOBAL TO SAVE SPACE IN STRUCT
float _HumidityPercent;  //GLOBAL TO SAVE SPACE IN STRUCT

typedef struct DHTresultStruct
{
    signed char ErrorCode = DEVICE_NOT_YET_ACCESSED;//
    u8 Type = 0;//
    short TemperatureCelsius;
    short HumidityPercent;
    unsigned long timeOfLastAccessMillis;
} DHTresult;

DHTresult DHTfunctionResultsArray[ NUM_DIGITAL_PINS + 1 ]; //The last entry will be the return values for "invalid pin numbers sent into the function" 
                                                           //and others like "rollover expected need to wait" where device type shouldn't be mucked with
//TODO: make retrieve data into a function


//TODO: verify and enforce rest time

void GetReading( u8 pin )
{
      long unsigned startBitTime;
        unsigned long turnover_reference_time;
        DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis = millis(); 
        digitalWrite( pin, LOW );
//        pinMode( pin, INPUT );
        if( digitalRead( pin ) == HIGH )
        {
            DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_FAILS_DURING_INITIALIZE;
            return; //to ensure the LOW level remains to ensure no conduction to high level
        }
//        pinMode( pin, OUTPUT );
        delay( 5 + 19 );//in case the device is in process of giving back data:  Wait for it to finish plus rest time (newer devices need less rest time than this)
        if( digitalRead( pin ) == HIGH )
        {
            DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_FAILS_DURING_INITIALIZE1;
            return; //to ensure the LOW level remains to ensure no conduction to high level
        }
        digitalWrite( pin, HIGH );//Now is safe to put a high on the pin, assuming a DHT data pin is there 
        turnover_reference_time = micros();//Handover. This marks end of host drive, beginning of device drive
        if( digitalRead( pin ) == LOW )
        {
            DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_FAILS_DURING_INITIALIZE2;
            return; //to ensure the LOW level remains to ensure no conduction to high level
        }

        *portModeRegister( digitalPinToPort( pin ) ) &= ~digitalPinToBitMask( pin );
        *portOutputRegister( digitalPinToPort( pin ) ) |= digitalPinToBitMask( pin );
        startBitTime = micros();//Handover. This marks end of host drive, beginning of device drive
        while( ( micros() - startBitTime < 200 ) && ( digitalRead( pin ) == HIGH ) );
        while( ( micros() - startBitTime < 200 ) && ( digitalRead( pin ) == LOW ) );
        if( digitalRead( pin ) == LOW )
        {//dht22 errors here with 104 if loop above is skipped
            DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_FAILS_DURING_INITIALIZE3;//88 uSec from pullup applied to high level
            return; //to ensure the LOW level remains to ensure no conduction to high level
        }
        while( ( micros() - startBitTime < 300 ) && ( digitalRead( pin ) == HIGH ) );
        if( digitalRead( pin ) == HIGH )
        {//dht11 errors here at 56-60 uSec if loop above is executed
            DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_FAILS_DURING_INITIALIZE4;//172 uSec from pullup applied to low level
            return; //to ensure the LOW level remains to ensure no conduction to high level
        }
        startBitTime = micros();
        u8 bitnumber = 0;
        u8 DataStreamBits[ 5 ];
        u16* DataStreamBits0 = ( u16* )&DataStreamBits[ 0 ];
        u16* DataStreamBits2 = ( u16* )&DataStreamBits[ 2 ];
        for( u8 d = 0; d < sizeof( DataStreamBits ); d++ ) DataStreamBits[ d ] = 0;
        while( bitnumber < 40 )
        {
            while( ( micros() - startBitTime < 70 ) && ( digitalRead( pin ) == LOW ) );
            if( digitalRead( pin ) == LOW )
            {
                DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_FAILS_DURING_INITIALIZE5;//224 uSec from pullup applied to high level
                return; //to ensure the LOW level remains to ensure no conduction to high level
            }
            startBitTime = micros();
            while( ( micros() - startBitTime < 170 ) && ( digitalRead( pin ) == HIGH ) );
            if( digitalRead( pin ) == HIGH )
            {
                DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_FAILS_DURING_INITIALIZE6;//248-252 uSec from pullup applied to high level
                return; //to ensure the LOW level remains to ensure no conduction to high level
            }
            /*bit is one if micros() - startBitTime > 48, zero otherwise */
            else if( micros() - startBitTime > 48 ) 
            {
                 startBitTime = micros();//capture time of signal going low
//here we have some time to compute
                 if( bitnumber < 8 )
                      DataStreamBits[ 0 ] |= bit( 7 - bitnumber );//or do we need to adjust bitnumber to eliminate rounding error?
                 else if( bitnumber < 16 )
                      DataStreamBits[ 1 ] |= bit( 15 - bitnumber );//or do we need to adjust bitnumber to eliminate rounding error?
                 else if( bitnumber < 24 )
                      DataStreamBits[ 2 ] |= bit( 23 - bitnumber );//or do we need to adjust bitnumber to eliminate rounding error?
                 else if( bitnumber < 32 )
                      DataStreamBits[ 3 ] |= bit( 31 - bitnumber );//or do we need to adjust bitnumber to eliminate rounding error?
                 else 
                      DataStreamBits[ 4 ] |= bit( 39 - bitnumber );//or do we need to adjust bitnumber to eliminate rounding error?
            }
            else startBitTime = micros();
            bitnumber++;
        }

        pinMode( pin, OUTPUT );
        digitalWrite( pin, HIGH );
        if( ( u8 )( DataStreamBits[ 0 ] + DataStreamBits[ 1 ] + DataStreamBits[ 2 ] + DataStreamBits[ 3 ] ) !=  DataStreamBits[ 4 ] )
        {
            DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_CRC_ERROR;
/*
        Serial.print( DataStreamBits[ 0 ], BIN );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        Serial.print( DataStreamBits[ 1 ], BIN );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        Serial.print( DataStreamBits[ 2 ], BIN );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        Serial.print( DataStreamBits[ 3 ], BIN );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        Serial.print( DataStreamBits[ 4 ], BIN );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
*/
            return;
        }
        DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_READ_SUCCESS;
        if( !DataStreamBits[ 1 ] && !DataStreamBits[ 3 ] && DataStreamBits[ 0 ] < 100 && DataStreamBits[ 2 ] < 50 )
        {//big endian adjustment included
            DHTfunctionResultsArray[ pin - 1 ].Type = TYPE_LIKELY_DHT11;
            *DataStreamBits0 = ( u16 )( ( DataStreamBits[ 0 ] << 1 ) + ( DataStreamBits[ 0 ] << 3 ) );//multiplies by 10
            *DataStreamBits2 = ( u16 )( ( DataStreamBits[ 2 ] << 1 ) + ( DataStreamBits[ 2 ] << 3 ) );//multiplies by 10
        }
        else 
        {//big endian adjustment
            DHTfunctionResultsArray[ pin - 1 ].Type = TYPE_KNOWN_DHT22;
            DataStreamBits[ 4 ] = DataStreamBits[ 3 ];
            DataStreamBits[ 3 ] = DataStreamBits[ 2 ];
            DataStreamBits[ 2 ] = DataStreamBits[ 1 ];
            DataStreamBits[ 1 ] = DataStreamBits[ 0 ];
            DataStreamBits[ 0 ] = DataStreamBits[ 2 ];
            DataStreamBits[ 2 ] = DataStreamBits[ 4 ];
        }
/*
float _TemperatureCelsius;  //GLOBAL TO SAVE SPACE IN STRUCT
float _HumidityPercent;  //GLOBAL TO SAVE SPACE IN STRUCT

 */
        DHTfunctionResultsArray[ pin - 1 ].TemperatureCelsius = *DataStreamBits2;
        DHTfunctionResultsArray[ pin - 1 ].HumidityPercent = *DataStreamBits0;
/*
        Serial.print( ( float )( *DataStreamBits0 / 10 ), 1 );
        Serial.print( F( " % Humidity, " ) );
        Serial.print( ( float )( *DataStreamBits2 / 10 ), 1 );
        Serial.print( F( " °C, " ) );
        if( DHTfunctionResultsArray[ pin - 1 ].ErrorCode == TYPE_KNOWN_DHT22 )
            Serial.print( F( "0.1" ) );
        else
            Serial.print( F( "1.0" ) );
        Serial.print( F( " resolution" ) );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
*/
//        Serial.print( DataStreamBits[ 0 ], BIN );
//        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//        Serial.print( DataStreamBits[ 1 ], BIN );
//        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//        Serial.print( DataStreamBits[ 2 ], BIN );
//        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//        Serial.print( DataStreamBits[ 3 ], BIN );
//        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
//        Serial.print( DataStreamBits[ 4 ], BIN );
//        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
/*        
#define DEVICE_NOT_YET_ACCESSED ( u8 ) -1
#define DEVICE_FAILS_DURING_INITIALIZE ( u8 ) -2
#define DEVICE_FAILS_DURING_INITIALIZE1 ( u8 ) -3
#define DEVICE_FAILS_DURING_INITIALIZE2 ( u8 ) -4
#define DEVICE_FAILS_DURING_INITIALIZE3 ( u8 ) -5
#define DEVICE_FAILS_DURING_INITIALIZE4 ( u8 ) -6
#define DEVICE_FAILS_DURING_INITIALIZE5 ( u8 ) -7
#define DEVICE_FAILS_DURING_INITIALIZE6 ( u8 ) -8
#define DEVICE_FAILS_DURING_INITIALIZE7 ( u8 ) -9
#define DEVICE_FAILS_DURING_INITIALIZE8 ( u8 ) -10
#define DEVICE_FAILS_DURING_DATA_STREAM ( u8 ) -11
#define DEVICE_CRC_ERROR ( u8 ) -12
#define DEVICE_READ_SUCCESS ( u8 ) -13

#define REFUSED_ROLLOVER_EXPECTED ( u8 ) -14
#define REFUSED_INVALID_PIN ( u8 ) -15

 
        Serial.print( F( "DEVICE_NOT_YET_ACCESSED ( u8 ) -1 = " ) );
        Serial.print( DEVICE_NOT_YET_ACCESSED );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        Serial.print( F( "DEVICE_FAILS_DURING_INITIALIZE ( u8 ) -2 = " ) );
        Serial.print( DEVICE_FAILS_DURING_INITIALIZE );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        Serial.print( F( "REFUSED_INVALID_PIN ( u8 ) -15 = " ) );
        Serial.print( REFUSED_INVALID_PIN );
        Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
*/
}


unsigned long DHTreadWhenRested( u8 pin )
{
//           Serial.print( F( "..calling for pin " ) );
//           Serial.print( pin );
//           Serial.print( F( ".." ) );
    if( ( pin < 1 ) || ( pin > NUM_DIGITAL_PINS ) )
    {
        DHTfunctionResultsArray[ NUM_DIGITAL_PINS ].ErrorCode = REFUSED_INVALID_PIN;
        return &DHTfunctionResultsArray[ NUM_DIGITAL_PINS ];
    }
    while( ( micros() > ( long unsigned )-30000 ) || ( millis() > ( long unsigned )-30 ) );
/*
        if( ( micros() > ( long unsigned )-30000 ) || ( millis() > ( long unsigned )-30 ) )
        {
            DHTfunctionResultsArray[ NUM_DIGITAL_PINS ].ErrorCode = REFUSED_ROLLOVER_EXPECTED;
            return DHTfunctionResultsArray[ NUM_DIGITAL_PINS ];
        }
*/
    for( u8 d = 0; d < NUM_DIGITAL_PINS; d++ )
    {
        if( DHTfunctionResultsArray[ d ].ErrorCode < DEVICE_READ_SUCCESS )
        {
            for( d = 0; d < NUM_DIGITAL_PINS; d++ )
                DHTfunctionResultsArray[ d ].ErrorCode = DEVICE_NOT_YET_ACCESSED;
            break;
        }
    }

//    DHTfunctionResultsArray[ pin - 1 ];
    if( ( DHTfunctionResultsArray[ pin - 1 ].ErrorCode == DEVICE_NOT_YET_ACCESSED ) || !( *portModeRegister( digitalPinToPort( pin ) ) & digitalPinToBitMask( pin ) /* UNTESTED if pin voltage is low something is wrong  */) )
    {//"first read" loop
        DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis = millis();
//Yes, we are assuming the high level has been there long enough.  The main loop needs to enforce that
        pinMode( pin, OUTPUT );//Now is safe to put a high on the pin, assuming a DHT data pin is there 
        digitalWrite( pin, HIGH );//Now is safe to put a high on the pin, assuming a DHT data pin is there 
        delay( 2000 );
    }
    else
    {
//           Serial.print( F( "..not the first time for pin " ) );
//           Serial.print( pin );
//           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        u16 rest_time = 1000;//1000 mSec or 1 full second
        rest_time += 30; // account for the 19 mSec plus 5 mSec start prep plus 6 mSec for  data stream
        if( ( DHTfunctionResultsArray[ pin - 1 ].Type == TYPE_KNOWN_DHT22 ) || ( DHTfunctionResultsArray[ pin - 1 ].Type == TYPE_LIKELY_DHT22 ) )
            rest_time += 1000;//adds another 1000 mSec on so now = 2 full seconds
//           Serial.print( F( "..rest_time=" ) );
//           Serial.print( rest_time );
//           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
        for( u8 d = 0; d < 1; d++ )
        {
//                       Serial.print( F( "..checking for need to delay.." ) );

            if( DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis + rest_time > DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis )
            {
//           Serial.print( F( " " ) );
//           Serial.print( millis() );
//           Serial.print( F( " now, millis pin " ) );
//           Serial.print( pin );
//           Serial.print( F( " was started last: " ) );
//           Serial.print( DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis );
//           Serial.print( F( ",  millis pin needed to rest to: " ) );
//           Serial.print( DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis + rest_time );
//           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );
                 if( ( millis() < DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis ) || ( millis() > DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis + rest_time ) )
                     break;
            }
            else
            {
                 if( ( millis() < DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis ) && ( millis() > DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis + rest_time ) )
                      break;
                 while( millis() > DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis );
            }
            while( millis() < DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis + rest_time );
//           Serial.print( F( "..delayed.." ) );
//           Serial.print( ( char )10 );if( mswindows ) Serial.print( ( char )13 );

        }
    }
    GetReading( pin );
deviceReadDone:;    
    digitalWrite( pin, HIGH );
    return &DHTfunctionResultsArray[ pin - 1 ];
}
