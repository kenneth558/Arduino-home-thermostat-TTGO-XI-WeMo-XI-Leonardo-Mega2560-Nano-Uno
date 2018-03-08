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
#define DEVICE_BYTE0_ERROR -13 //243
#define DEVICE_BYTE2_ERROR -14 //242
#define DEVICE_BYTES0and1_ERROR -15 //241
#define DEVICE_BYTES2and3_ERROR -16 //240
#define DEVICE_READ_SUCCESS -17 //239

#define REFUSED_ROLLOVER_EXPECTED -18 //238
#define REFUSED_INVALID_PIN -19 //237

#define TYPE_KNOWN_DHT11 1
#define TYPE_KNOWN_DHT22 2
#define TYPE_LIKELY_DHT11 3
#define TYPE_LIKELY_DHT22 4

#define LIVE 0
#define RECENT 1

float _TemperatureCelsius;  //GLOBAL TO SAVE SPACE IN STRUCT
float _HumidityPercent;  //GLOBAL TO SAVE SPACE IN STRUCT
float O_TemperatureCelsius;  //GLOBAL TO SAVE SPACE IN STRUCT
float O_HumidityPercent;  //GLOBAL TO SAVE SPACE IN STRUCT
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
            return;
        }
        else if( DataStreamBits[ 0 ] > 3 && DataStreamBits[ 0 ] < 18 ) //no values of 0 are ever valid if between 4 and 17 inclusive
            goto byte0_error;
        else if( DataStreamBits[ 2 ] > 129 || ( DataStreamBits[ 2 ] & 0x7f ) > 55 )
        {
            DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_BYTE2_ERROR;
            return;
        }
        else if( DataStreamBits[ 2 ] & 0x80 )
        {
            if( ( ( DataStreamBits[ 2 ] & 0x7f ) * 256 ) + DataStreamBits[ 3 ] > 410 ) //81.0C max allowed
            {
                DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_BYTES0and1_ERROR;
                return;
            }
            goto known_22;
        }
        else if( !DataStreamBits[ 1 ] && !DataStreamBits[ 3 ] )
        {
            if( DataStreamBits[ 0 ] < 4 && DataStreamBits[ 2 ] < 4 )
                goto likely_22;//likely, not known, this is the readings of commonality between both 11 and 22.  We favor the 22 b/c its rest time is compatible with 11, not other way around
            else if( DataStreamBits[ 0 ] > 81 || DataStreamBits[ 0 ] < 18 )//must be 18-81
                goto byte0_error;
            else if( DataStreamBits[ 2 ] > 55 )//must be 0-51 for 11, 22 already accounted for by above readings of commonality check
            {
                DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_BYTE2_ERROR;
                return;
            }
            goto known_11;
        }
//known 22
        else if( ( DataStreamBits[ 0 ] * 256 ) + DataStreamBits[ 1 ] > 1000 ) //100% max allowed
        {
            //check for 22 readings out of bounds 0-100%RH, -40-80C
            DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_BYTES0and1_ERROR;
            return;
        }
        else if( ( DataStreamBits[ 2 ] * 256 ) + DataStreamBits[ 3 ] > 810 ) //81.0C max allowed
        {
            DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_BYTES0and1_ERROR;
            return;
        }
//fall through as known dht22
known_22:;
            DHTfunctionResultsArray[ pin - 1 ].Type = TYPE_KNOWN_DHT22;
known_22_plus_one:;
            //changing from 22 to 11 is fine but not the other way
            DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_READ_SUCCESS;
            DataStreamBits[ 4 ] = DataStreamBits[ 3 ];
            DataStreamBits[ 3 ] = DataStreamBits[ 2 ];
            DataStreamBits[ 2 ] = DataStreamBits[ 1 ];
            DataStreamBits[ 1 ] = DataStreamBits[ 0 ];
            DataStreamBits[ 0 ] = DataStreamBits[ 2 ];
            DataStreamBits[ 2 ] = DataStreamBits[ 4 ];
//        }
        goto past_device_type_sort;
likely_22:;
            //changing from 22 to 11 is fine but not the other way
        DHTfunctionResultsArray[ pin - 1 ].Type = TYPE_LIKELY_DHT22;
        goto known_22_plus_one;
byte0_error:;
        DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_BYTE0_ERROR;
        return;
likely_11:;
            //changing from 22 to 11 is fine but not the other way
        DHTfunctionResultsArray[ pin - 1 ].Type = TYPE_LIKELY_DHT11;
        goto known_11_plus_one;
known_11:;
        DHTfunctionResultsArray[ pin - 1 ].Type = TYPE_KNOWN_DHT11;
known_11_plus_one:;
        DHTfunctionResultsArray[ pin - 1 ].ErrorCode = DEVICE_READ_SUCCESS;
        *DataStreamBits0 = ( u16 )( ( DataStreamBits[ 0 ] << 1 ) + ( DataStreamBits[ 0 ] << 3 ) );//multiplies by 10
        *DataStreamBits2 = ( u16 )( ( DataStreamBits[ 2 ] << 1 ) + ( DataStreamBits[ 2 ] << 3 ) );//multiplies by 10
past_device_type_sort:;
        DHTfunctionResultsArray[ pin - 1 ].TemperatureCelsius = *DataStreamBits2;
        DHTfunctionResultsArray[ pin - 1 ].HumidityPercent = *DataStreamBits0;
}

DHTresult* FetchTemp( u8 pin, u8 LiveOrRecent )
{
    if( ( pin < 1 ) || ( pin > NUM_DIGITAL_PINS ) )
    {
        DHTfunctionResultsArray[ NUM_DIGITAL_PINS ].ErrorCode = REFUSED_INVALID_PIN;
        return &DHTfunctionResultsArray[ NUM_DIGITAL_PINS ];
    }
    for( u8 d = 0; d < NUM_DIGITAL_PINS; d++ )
    {
        if( DHTfunctionResultsArray[ d ].ErrorCode < DEVICE_READ_SUCCESS )
        {
            for( d = 0; d < NUM_DIGITAL_PINS; d++ )
                DHTfunctionResultsArray[ d ].ErrorCode = DEVICE_NOT_YET_ACCESSED;
            break;
        }
    }

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
        u16 rest_time = 1000;//1000 mSec or 1 full second
        rest_time += 30; // account for the 19 mSec plus 5 mSec start prep plus 6 mSec for  data stream
        if( ( DHTfunctionResultsArray[ pin - 1 ].Type == TYPE_KNOWN_DHT22 ) || ( DHTfunctionResultsArray[ pin - 1 ].Type == TYPE_LIKELY_DHT22 ) )
            rest_time += 1000;//adds another 1000 mSec on so now = 2 full seconds
        if( LiveOrRecent == RECENT && millis() - DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis < rest_time && DHTfunctionResultsArray[ pin - 1 ].ErrorCode == DEVICE_READ_SUCCESS )
        {
            return &DHTfunctionResultsArray[ pin - 1 ];
        }
        while( millis() - DHTfunctionResultsArray[ pin - 1 ].timeOfLastAccessMillis < rest_time );
    }
    GetReading( pin );
deviceReadDone:;    
    digitalWrite( pin, HIGH );
    return &DHTfunctionResultsArray[ pin - 1 ];
}
