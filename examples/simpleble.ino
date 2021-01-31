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
);

int exampleService = -1;
int exampleChar = -1;

static Timeout secondTimeout(10000);


void setup()
{
    Timeout::init(millis);

    pinMode(RX_ENABLE_PIN, OUTPUT);
    pinMode(MODULE_RESET_PIN, OUTPUT);

    Serial.begin(115200);
    altSerial.begin(9600);

    Serial.println("Example started");

    ble.begin();

    Serial.println("Reseting the device");
    ble.softRestart();
    delay(10);

    Serial.println("Setting TX power");
    ble.setTxPower(SimpleBLE::POW_0DBM);
    delay(1000);

    Serial.println("Adding the service");
    exampleService = ble.addService(0xA0);
    Serial.print("Added service: ");
    Serial.println(exampleService);
    delay(1000);

    if( exampleService >= 0 )
    {
        exampleChar = ble.addChar(
            exampleService,
            10,
            SimpleBLE::READ | SimpleBLE::WRITE | SimpleBLE::NOTIFY);

        Serial.print("Added characteristic: ");
        Serial.println(exampleChar);
        delay(1000);
    }

    const char devName[] = "SimpleBLE example";
    ble.setAdvPayload(SimpleBLE::COMPLETE_LOCAL_NAME, (uint8_t*)devName, sizeof(devName)-1);
    delay(1000);

    ble.startAdvertisement(100, SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION, true);
    delay(1000);
}

void loop()
{
    int32_t availableData = ble.checkChar(exampleService, exampleChar);
    delay(1000);

    if( availableData > 0 )
    {
        uint8_t recvData[availableData];

        ble.readChar(exampleService, exampleChar, recvData, availableData);
        delay(1000);

        Serial.print("Read data: ");
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
        delay(1000);
    }
}
