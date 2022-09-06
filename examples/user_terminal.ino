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

char cmdStr[6] = {'\0', '\0', '\0', '\0', '\0', '\0'};
int cmdStrIndex = 0;
bool enableRecv = false;


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

    Serial.println("Device ready to receive commands");
}

void loop()
{
    if( Serial.available() )
    {
        char c = Serial.read();
        Serial.write(c);

        enableRecv = false;

        if( c == '\n' )
        {
            enableRecv = true;
            if( cmdStrIndex > 0 )
            {
                altSerial.write(cmdStr, sizeof(cmdStr));
                memset(cmdStr, '\0', sizeof(cmdStr));
                cmdStrIndex = 0;
            }
        }
        else
        {
            cmdStr[cmdStrIndex++] = c;
            if( cmdStrIndex >= sizeof(cmdStr) )
            {
                altSerial.write(cmdStr, sizeof(cmdStr));
                memset(cmdStr, '\0', sizeof(cmdStr));
                cmdStrIndex = 0;
            }
        }
    }

    if( enableRecv && altSerial.available() )
    {
        char c = altSerial.read();

        Serial.write(c);
    }
}
