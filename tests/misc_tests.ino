#include "Arduino.h"
#include "AltSoftSerial.h"

#include "simple_ble.h"
#include "timeout.h"


// AltSoft lib uses these RX and TX pins for communication but it doesn't
// realy nead them to be defined here. This is just for reference.
#define RX_PIN 8
#define TX_PIN 9
#define RX_ENABLE_PIN 10
#define MODULE_RESET_PIN 11


static AltSoftSerial altSerial;


static SimpleBLE ble(
    [](bool state) { digitalWrite(RX_ENABLE_PIN, state ? HIGH : LOW); },
    [](bool state) { digitalWrite(MODULE_RESET_PIN, state ? HIGH : LOW); },
    [](char c) { return altSerial.write(c) > 0; },
    [](char *c)
    {
        bool availableChars = altSerial.available() > 0;

        *c = availableChars ? altSerial.read() : *c ;

        return availableChars;
    },
    [](void) { return (uint32_t)millis(); },
    [](uint32_t ms) { delay(ms); },
    [](const char *dbg) { Serial.print(dbg); }
    //NULL
);

void bleReset()
{
    Serial.println("Reseting the device");
    ble.softRestart();
    delay(10);
}

void setup()
{
    Timeout::init(millis);

    pinMode(RX_ENABLE_PIN, OUTPUT);
    pinMode(MODULE_RESET_PIN, OUTPUT);

    Serial.begin(115200);
    altSerial.begin(9600);

    Serial.println("Example started");

    ble.begin();

    if(0)
    {
        bleReset();

        ble.sendReceiveCmd("AT+STAT?");

        Serial.println("Setting adv payload complete local name.");
        const char devName[] = "SimpleBLE example";
        ble.setAdvPayload(SimpleBLE::COMPLETE_LOCAL_NAME, (uint8_t*)devName, sizeof(devName)-1);
        delay(100);
    
        Serial.println("Starting advertisement.");
        ble.startAdvertisement(100, SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION, true);
        delay(10000);

        Serial.println("Changing adv payload complete local name.");
        const char devName2[] = "Test example";
        ble.setAdvPayload(SimpleBLE::COMPLETE_LOCAL_NAME, (uint8_t*)devName2, sizeof(devName2)-1);
        delay(10000);
        
        Serial.println("Changing adv payload MSD.");
        const uint8_t msd[] = {0x34, 0xf4};
        ble.setAdvPayload(SimpleBLE::MANUFACTURER_SPECIFIC_DATA, (uint8_t*)msd, sizeof(msd));
        delay(10000);
        
        Serial.println("Changing adv payload MSD.");
        const uint8_t msd2[] = {0x22, 0xd8};
        ble.setAdvPayload(SimpleBLE::MANUFACTURER_SPECIFIC_DATA, (uint8_t*)msd2, sizeof(msd2));
        delay(10000);
       
        Serial.println("Stoping advertisement.");
        ble.stopAdvertisement();
        delay(10000);
        
        Serial.println("Starting advertisement.");
        ble.startAdvertisement(100, 10000, true);
        delay(10000);
    }

    if(0)
    {
        int8_t services[3] = {-1,-1,-1};
        int8_t chars[7] = {-1,-1,-1,-1,-1,-1, -1};

        bleReset();

        services[0] = ble.addService(0xA0);

        chars[0] = ble.addChar(
            services[0],
            100,
            SimpleBLE::READ);

        services[1] = ble.addService(0xB0);
        // Show that we cant add new service while previous is empty.
        services[2] = ble.addService(0xC0);

        chars[1] = ble.addChar(
            services[1],
            80,
            SimpleBLE::READ | SimpleBLE::WRITE);
        chars[2] = ble.addChar(
            services[1],
            80,
            SimpleBLE::READ | SimpleBLE::WRITE | SimpleBLE::NOTIFY);

        services[2] = ble.addService(0xC0);

        chars[3] = ble.addChar(
            services[2],
            60,
            SimpleBLE::WRITE);
        chars[4] = ble.addChar(
            services[2],
            60,
            SimpleBLE::NOTIFY);
        chars[5] = ble.addChar(
            services[2],
            60,
            SimpleBLE::WRITE | SimpleBLE::NOTIFY);

        // Show that we cant add to previous service.
        chars[7] = ble.addChar(
            services[1],
            80,
            SimpleBLE::READ | SimpleBLE::WRITE);
        
        Serial.print("Added services: ");
        Serial.print(String(services[0]) + ", ");
        Serial.print(String(services[1]) + ", ");
        Serial.println(services[2]);

        Serial.print("Added characteristics: ");
        Serial.print(String(chars[0]) + ", ");
        Serial.print(String(chars[1]) + ", ");
        Serial.print(String(chars[2]) + ", ");
        Serial.print(String(chars[3]) + ", ");
        Serial.print(String(chars[4]) + ", ");
        Serial.println(chars[5]);
        
        Serial.println("Setting adv payload complete local name.");
        const char devName[] = "SimpleBLE example";
        ble.setAdvPayload(SimpleBLE::COMPLETE_LOCAL_NAME, (uint8_t*)devName, sizeof(devName)-1);
        delay(100);
    
        Serial.println("Starting advertisement.");
        ble.startAdvertisement(100, SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION, true);
        delay(10000);
    }

    if(1)
    {
        const int charSize = 200;

        int8_t service = -1;
        int8_t characteristic = -1;
  
        bleReset();

        ble.sendReceiveCmd("AT+STAT?");
  
        service = ble.addService(0xA0);
  
        characteristic = ble.addChar(
            service,
            charSize,
            SimpleBLE::READ |
            SimpleBLE::WRITE |
            SimpleBLE::NOTIFY
            );
        
        Serial.println("Setting adv payload complete local name.");
        const char devName[] = "SimpleBLE example";
        ble.setAdvPayload(SimpleBLE::COMPLETE_LOCAL_NAME, (uint8_t*)devName, sizeof(devName)-1);
        delay(100);
    
        Serial.println("Starting advertisement.");
        ble.startAdvertisement(100, SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION, true);
        delay(1000);

        uint8_t fillBuff[charSize];
        uint8_t checkBuff[charSize];
        uint8_t fillValue = 0;
        while(1)
        {
            for(int i = 0; i < sizeof(fillBuff); i++)
            {
                fillBuff[i] = fillValue;
            }
            ble.writeChar(service, characteristic, fillBuff, sizeof(fillBuff));
            ble.checkChar(service, characteristic);
            ble.readChar(service, characteristic, checkBuff, sizeof(checkBuff));
            bool allGood = true;
            for(int i = 0; i < sizeof(checkBuff); i++)
            {
                if( checkBuff[i] != fillValue )
                {
                    allGood = false;
                    break;
                }
            }
            unsigned long lastGood = millis();
            if( allGood )
            {
                Serial.print("All is good ");
                Serial.println(lastGood/1000/3600.);
            }
            else
            {
                while(1)
                {              
                    Serial.print("Not all is good ");
                    Serial.println(lastGood/1000/3600.);
                    delay(1000);
                }
            }
            fillValue += 0x11;
            delay(1);
        }
    }

/*
    Serial.println("Setting TX power");
    ble.setTxPower(SimpleBLE::POW_0DBM);
    delay(100);

    Serial.println("Adding the service");
    exampleService = ble.addService(0xA0);
    Serial.print("Added service: ");
    Serial.println(exampleService);
    delay(100);

    if( exampleService >= 0 )
    {
        exampleChar = ble.addChar(
            exampleService,
            20,
            SimpleBLE::READ | SimpleBLE::WRITE | SimpleBLE::NOTIFY);

        Serial.print("Added characteristic: ");
        Serial.println(exampleChar);
        delay(100);
    }

    const char devName[] = "SimpleBLE example";
    ble.setAdvPayload(SimpleBLE::COMPLETE_LOCAL_NAME, (uint8_t*)devName, sizeof(devName)-1);
    delay(100);

    Serial.println("Starting advertisement.");
    ble.startAdvertisement(100, SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION, true);
    delay(100);
*/
}

void loop()
{
/*
    int32_t availableData = 100;
    availableData = ble.checkChar(exampleService, exampleChar);

    if( availableData != 0 )
    {
        Serial.print("Read length: ");
        Serial.print(availableData);

        availableData = abs(availableData);
        uint8_t recvData[availableData];

        ble.readChar(exampleService, exampleChar, recvData, availableData);

        Serial.print(", data: ");
        for(int i = 0; i < availableData; i++)
        {
            Serial.print(recvData[i], HEX);
            Serial.print(" ");
        }
        Serial.println("");
    }

    if( secondTimeout.expired() )
    {
        secondTimeout.restart();

        uint32_t currMillis = millis();

        ble.writeChar(exampleService, exampleChar,
                      (uint8_t*)&currMillis, sizeof(currMillis));

    }
*/
    delay(30);
}
