#include "Arduino.h"

#include "simpleble/simple_ble.h"


static SimpleBLE ble;

// Storage for IDs of your data tanks
SimpleBLE::TankId secondsTankId;
SimpleBLE::TankId counterTankId;
SimpleBLE::TankId buttonTankId;


void setup()
{
    Serial.begin(115200);

    Serial.println("Example started");

    ble.begin();

    secondsTankId = ble.addTank(SimpleBLE::READ, 15);
    if( secondsTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println("Failed to add seconds tank.");
    }

    counterTankId = ble.addTank(SimpleBLE::READ, 15);
    if( counterTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println("Failed to add counter tank.");
    }

    buttonTankId = ble.addTank(SimpleBLE::WRITE_WITH_RESPONSE, 1);
    if( buttonTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println("Failed to add button tank.");
    }

    Serial.println("Added all data tanks");

    Serial.println("Starting advertisement.");
    ble.setDeviceName("SimpleBLE example");
    ble.startAdvertisement(100, SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION);
}

void loop()
{
    if( ble.getAtInterface()->waitURC("^CHARWRITE", urcBuff, sizeof(urcBuff), 1000) > 0 )
            {
                char *urcStart = urcBuff; while(*urcStart != '^') urcStart++;

                // Convert char to char number (0:'^' 1:'C' ... 10:':' 11:' ' 12:'X' 14:'X')
                uint8_t writtenChar = urcStart[14] - '0' ;
                uint8_t writtenSize = urcStart[16] - '0' ;

                uint8_t characteristicData = 0;

                if( ble.readChar(ioServiceId, writtenChar, &characteristicData, writtenSize) > 0)
                {
                    if( writtenChar == buttonCharId )
                    {
                        buttonEvents.put(characteristicData);
                    }
                    else if( writtenChar == switchCharId )
                    {
                        switchEvents.put(characteristicData);
                    }
                    else if( writtenChar == sliderCharId )
                    {
                        sliderEvents.put(characteristicData);
                    }
                }
            }
            if( checkcomm.expired() )
            {
                bool commCheck = false;
                do{
                    if( ble.sendReceiveCmd("AT", 100) == AtProcess::SUCCESS )
                    {
                        //setScore("S");
                        commCheck = true;
                    }
                    else
                    {
                        ble.deactivateModuleRx();
                        vTaskDelay(10);
                        ble.activateModuleRx();
                        vTaskDelay(10);
                        //setScore("F");
                        commCheck = false;
                    }
                }while(!commCheck);
                checkcomm.restart();
            }
            if( scoreBuff[0] )
            {
                scoreBuff[sizeof(scoreBuff)-1] = '\0'; // Null terminate string just in case
                ble.writeChar(ioServiceId, scoreCharId, (uint8_t*)scoreBuff, strlen(scoreBuff));
                scoreBuff[0] = '\0';
            }
            if( gameNameBuff[0] )
            {
                gameNameBuff[sizeof(gameNameBuff)-1] = '\0'; // Null terminate string just in case
                ble.writeChar(ioServiceId, gameCharId, (uint8_t*)gameNameBuff, strlen(gameNameBuff));
                gameNameBuff[0] = '\0';
            }

            // Take the notification and go to low power more
            if( !isAccActive() )
            {
                ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(0));
                ble.deactivateModuleRx();
                lpEnableSleep();
            }
    
    delay(10);
}
