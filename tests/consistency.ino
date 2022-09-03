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

int exampleService = -1;
int exampleChar = -1;
int exampleService2 = -1;
int exampleChar2 = -1;

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
            SimpleBLE::READ | SimpleBLE::WRITE);

        Serial.print("Added characteristic: ");
        Serial.println(exampleChar);
        delay(100);
    }
    
    exampleService2 = ble.addService(0xB0);

    if( exampleService2 >= 0 )
    {
        exampleChar2 = ble.addChar(
            exampleService2,
            20,
            SimpleBLE::READ | SimpleBLE::NOTIFY);

        Serial.print("Added characteristic: ");
        Serial.println(exampleChar2);
        delay(100);
    }

    const char devName[] = "Za Bornu ❤";
    ble.setAdvPayload(SimpleBLE::COMPLETE_LOCAL_NAME, (uint8_t*)devName, sizeof(devName)-1);
    delay(100);

    Serial.println("Starting advertisement.");
    ble.startAdvertisement(100, SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION, true);
    delay(100);
}

void loop()
{
    if( secondTimeout.expired() )
    {
        secondTimeout.restart();

        uint32_t currMillis = millis();

        ble.writeChar(exampleService2, exampleChar2,
                      (uint8_t*)&currMillis, sizeof(currMillis));

    }

    delay(30);
}
