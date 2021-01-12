#include "Arduino.h"
#include "SoftwareSerial.h"

#include "simple_ble.h"
#include "timeout.h"

#define RX_PIN 10
#define TX_PIN 11
#define RX_ENABLE_PIN 12
#define MODULE_RESET_PIN 9


static SoftwareSerial softSerial(RX_PIN, TX_PIN);

//SimpleBLE ble(RX_PIN, TX_PIN, RX_ENABLE_PIN, MODULE_RESET_PIN);
static SimpleBLE ble(
    [](bool state) { digitalWrite(RX_ENABLE_PIN, state ? HIGH : LOW); },
    [](bool state) { digitalWrite(MODULE_RESET_PIN, state ? HIGH : LOW); },
    [](char c) { return softSerial.write(c) > 0; },
    [](char *c)
    {
        bool availableChars = softSerial.available() > 0;

        *c = availableChars ? softSerial.read() : *c ;

        return availableChars;
    },
    [](void) { return (uint32_t)millis(); },
    [](uint32_t ms) { delay(ms); },
    [](const char *dbg) { Serial.print(dbg); }
);

int exampleService = -1;
int exampleChar = -1;

void setup()
{
    pinMode(RX_ENABLE_PIN, OUTPUT);
    pinMode(MODULE_RESET_PIN, OUTPUT);

    Serial.begin(115200);
    softSerial.begin(9600);

    Serial.println("Example started");

    ble.begin();

    Serial.println("Reseting the device");
    ble.softRestart();
    delay(10);

    Serial.println("Setting TX power");
    ble.setTxPower(SimpleBLE::POW_0DBM);
    delay(10);

    Serial.println("Adding the service");
    exampleService = ble.addService(0xA0);
    delay(10);

    if( exampleService >= 0 )
    {
        exampleChar = ble.addChar(
            exampleService,
            10,
            SimpleBLE::READ | SimpleBLE::WRITE | SimpleBLE::NOTIFY);
    delay(10);
    }

    const char devName[] = "SimpleBLE example";
    ble.setAdvPayload(SimpleBLE::COMPLETE_LOCAL_NAME, (uint8_t*)devName, sizeof(devName)-1);
    delay(10);

    ble.startAdvertisement(100, 10000, true);
    delay(10);
}

void loop()
{
    int32_t availableData = ble.checkChar(exampleService, exampleChar);
    delay(10);

    if( availableData > 0 )
    {
        uint8_t recvData[availableData];

        ble.readChar(exampleService, exampleChar, recvData, availableData);
    delay(10);

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
    delay(10);
    }
}
