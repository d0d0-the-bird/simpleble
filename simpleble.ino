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

    Serial.println("Example started");

    // Begin BLE. UART speed is at 9600 by default
    if( !ble.begin() )
    {
        Serial.println("Failed to initialise SimpleBLE.");
        delay(10); exit(1);
    }

    secondsTankId = ble.addTank(SimpleBLE::READ, 15);
    if( secondsTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println("Failed to add seconds tank.");
        delay(10); exit(1);
    }

    btnStateTankId = ble.addTank(SimpleBLE::READ, 15);
    if( btnStateTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println("Failed to add button state tank.");
        delay(10); exit(1);
    }

    buttonTankId = ble.addTank(SimpleBLE::WRITE_CONFIRMED, 1);
    if( buttonTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println("Failed to add button tank.");
        delay(10); exit(1);
    }

    Serial.println("Added all data tanks");
    Serial.print("Seconds Tank ID: "); Serial.println(secondsTankId);
    Serial.print("Button state Tank ID: "); Serial.println(btnStateTankId);
    Serial.print("Button Tank ID: "); Serial.println(buttonTankId);

    Serial.println("Starting advertisement.");
    ble.setDeviceName("SimpleBLE example");
    ble.startAdvertisement(300);
}

bool buttonState = false;

void loop()
{
    SimpleBLE::TankData updatedTank = ble.manageUpdates();

    Serial.print("Got update from tank "); Serial.print(updatedTank.getId());
    Serial.print(" data "); Serial.println(updatedTank[0]);

    if( updatedTank.getId() == buttonTankId )
    {
        buttonState = !!(updatedTank[0]);

        // For catching fast updates, our UART speed is a bit too low
        SimpleBLE::TankData updatedTank = ble.manageUpdates(10);
        if( updatedTank.getId() == buttonTankId )
            buttonState = !!(updatedTank[0]);

        String btnState(buttonState ? "down" : "up");
        ble.writeTank(btnStateTankId, btnState.c_str(), btnState.length());
    }
    else
    {
        String seconds(millis()/1000);
        ble.writeTank(secondsTankId, seconds.c_str(), seconds.length());
    }
}
