#include "Arduino.h"

#include "simple_ble.h"
#include "timeout.h"

#define rxPin 10
#define txPin 11
#define rxEnablePin 12
#define moduleResetPin 9


SimpleBLE ble(rxPin, txPin, rxEnablePin, moduleResetPin);

int exampleService = -1;
int exampleChar = -1;

void setup()
{
    ble.begin();

    ble.softRestart();

    ble.setTxPower(SimpleBLE::POW_0DBM);

    exampleService = ble.addService(0xA0);

    if( exampleService >= 0 )
    {
        exampleChar = ble.addChar(
            exampleService,
            10,
            SimpleBLE::READ | SimpleBLE::WRITE | SimpleBLE::NOTIFY);
    }

    const char devName[] = "SimpleBLE example";
    ble.setAdvPayload(SimpleBLE::COMPLETE_LOCAL_NAME, (uint8_t*)devName, sizeof(devName)-1);

    ble.startAdvertisement(100, 10000, true);
}

void loop()
{
    int32_t availableData = ble.checkChar(exampleService, exampleChar);

    if( availableData > 0 )
    {
        uint8_t recvData[availableData];

        ble.readChar(exampleService, exampleChar, recvData, availableData);

        for(int i = 0; i < availableData; i++)
        {
            Serial.print(recvData[i]);
            Serial.print(" ");
        }
        Serial.println("");
    }

    Timeout secondTimeout(1000);

    if( secondTimeout.expired() )
    {
        uint32_t currMillis = millis();

        ble.writeChar(exampleService, exampleChar,
                      (uint8_t*)&currMillis, sizeof(currMillis));
    }
}
