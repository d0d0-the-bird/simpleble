#include "Arduino.h"
#include "AltSoftSerial.h"

#include "simpleble/simple_ble.h"


// AltSoft lib uses these RX and TX pins for communication but it doesn't
// realy nead them to be defined here. This is just for reference.
#define RX_PIN 8
#define TX_PIN 9
#define RX_ENABLE_PIN 10
#define MODULE_RESET_PIN 11


static AltSoftSerial altSerial;

const static SimpleBLEInterface simpleBleIf =
{
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
};

static SimpleBLE ble(
    &simpleBleIf
);

void bleReset()
{
    delay(10);
}

int exampleService = -1;
int exampleChar = -1;


void setup()
{
    pinMode(RX_ENABLE_PIN, OUTPUT);
    pinMode(MODULE_RESET_PIN, OUTPUT);

    Serial.begin(115200);
    altSerial.begin(9600);

    Serial.println("Example started");

    ble.begin();

    Serial.println("Reseting the device");
    ble.softRestart();

    Serial.println("Setting TX power");
    ble.setTxPower(SimpleBLE::POW_0DBM);

    Serial.println("Adding the service");
    exampleService = ble.addService(0xA0);
    Serial.print("Added service: ");
    Serial.println(exampleService);

    if( exampleService >= 0 )
    {
        exampleChar = ble.addChar(
            exampleService,
            20,
            SimpleBLE::READ | SimpleBLE::WRITE | SimpleBLE::NOTIFY);

        Serial.print("Added characteristic: ");
        Serial.println(exampleChar);
    }

    const char devName[] = "SimpleBLE example";
    ble.setAdvPayload(SimpleBLE::COMPLETE_LOCAL_NAME, (uint8_t*)devName, sizeof(devName)-1);

    Serial.println("Starting advertisement.");
    ble.startAdvertisement(100, SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION, true);
}

void loop()
{
    
    delay(10);
}
