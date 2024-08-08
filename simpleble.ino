#include "Arduino.h"

#include "simpleble/simple_ble.h"


static SimpleBLE ble;

// Storage for IDs of your data tanks
SimpleBLE::TankId secondsTankId;
SimpleBLE::TankId btnStateTankId;
SimpleBLE::TankId buttonTankId;

void setup()
{
    Serial.begin(115200);

    Serial.println(F("Example started"));

    // Begin BLE. UART speed is at 9600 by default
    if( !ble.begin() )
    {
        Serial.println(F("Failed to initialise SimpleBLE."));
        delay(10); exit(1);
    }

    secondsTankId = ble.addTank(SimpleBLE::READ, 15);
    if( secondsTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println(F("Failed to add seconds tank."));
        delay(10); exit(1);
    }

    btnStateTankId = ble.addTank(SimpleBLE::READ, 15);
    if( btnStateTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println(F("Failed to add button state tank."));
        delay(10); exit(1);
    }

    buttonTankId = ble.addTank(SimpleBLE::WRITE_CONFIRMED, 1);
    if( buttonTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println(F("Failed to add button tank."));
        delay(10); exit(1);
    }

    Serial.println(F("Added all data tanks"));
    Serial.print(F("Seconds Tank ID: ")); Serial.println(secondsTankId);
    Serial.print(F("Button state Tank ID: ")); Serial.println(btnStateTankId);
    Serial.print(F("Button Tank ID: ")); Serial.println(buttonTankId);

    Serial.println(F("Starting advertisement."));
    ble.setDeviceName("SimpleBLE example");
    ble.startAdvertisement(300);
}

void loop()
{
    static uint32_t startSecond = 0;

    SimpleBLE::TankData updatedTank = ble.manageUpdates();

    Serial.print(F("Got update from tank ")); Serial.print(updatedTank.getId());
    Serial.print(F(" data ")); Serial.println(updatedTank[0]);

    if( updatedTank.getId() == buttonTankId )
    {
        bool buttonState = updatedTank.getBool();

        String btnStateStr(buttonState ? "down" : "up");
        ble.writeTank(btnStateTankId, btnStateStr.c_str(), btnStateStr.length());
    }

    if( millis()-startSecond >= 1000 )
    {
        startSecond = millis();

        String seconds(millis()/1000);
        ble.writeTank(secondsTankId, seconds.c_str(), seconds.length());
    }
}
