#include "Arduino.h"
#include "LiquidCrystal_I2C.h"

#include <DHT11.h>

#include "simpleble/simple_ble.h"

// DHT11 library available in Arduino IDE Library Manager by the name "DHT11", or at the link:
// https://github.com/dhrubasaha08/DHT11/tree/main
DHT11 dht11(3); // OUT -> D3

// Temperature measurement period. By default it is set to 2 seconds since this
// is the limitation of the sensor.
uint8_t tempMeasPeriod = 0;

// LiquidCristal library available in Arduino IDE Library Manager by the name "LiquidCrystal I2C", or at the link:
// https://github.com/johnrickman/LiquidCrystal_I2C/tree/master
LiquidCrystal_I2C lcd(0x27, 20, 4);  // SDA -> A4, SCL -> A5

static SimpleBLE ble;

// Storage for IDs of your data tanks
SimpleBLE::TankId tempMeasPeriodId;
SimpleBLE::TankId tempId;
SimpleBLE::TankId lcdTankId;
SimpleBLE::TankId buttonTankId;

void setupDHT11()
{
    dht11.setDelay(500); // Set this to the desired delay. Default is 500ms.
}
void setupLCD()
{
    lcd.init(); 
    lcd.backlight();
}
void setupRelay()
{
    pinMode(2, OUTPUT); // Connect relay to D3
    digitalWrite(2, LOW);
}
int measureTemp()
{
    int temperature = dht11.readTemperature();
    if (temperature != DHT11::ERROR_CHECKSUM && temperature != DHT11::ERROR_TIMEOUT)
        return temperature;
    else
        return -1000; // Impossible temp to signal error
}
void lcdPrintTopRow(const char *text)
{
    lcd.setCursor(0, 0);
    lcd.printstr(text);
}
void lcdPrintBottomRow(const char *text)
{
    lcd.setCursor(0, 1);
    lcd.printstr(text);
}
void setRelay(bool newState)
{
    if(newState)
    {
      digitalWrite(2, HIGH);
    }
    else
    {
      digitalWrite(2, LOW);
    }
}

void setup()
{
    Serial.begin(115200);

    setupDHT11();
    setupLCD();
    setupRelay();

    Serial.println(F("Example started"));

    // Begin BLE. UART speed is at 9600 by default
    if( !ble.begin() )
    {
        Serial.println(F("Failed to initialise SimpleBLE."));
        delay(10); exit(1);
    }

    tempMeasPeriodId = ble.addTank(SimpleBLE::WRITE_CONFIRMED, 15);
    if( tempMeasPeriodId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println(F("Failed to add temperature measurement period tank."));
        delay(10); exit(1);
    }
    
    tempId = ble.addTank(SimpleBLE::READ, 15);
    if( tempId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println(F("Failed to add temperature tank."));
        delay(10); exit(1);
    }

    lcdTankId = ble.addTank(SimpleBLE::WRITE_CONFIRMED, 16);
    if( lcdTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println(F("Failed to add LCD display output tank."));
        delay(10); exit(1);
    }

    buttonTankId = ble.addTank(SimpleBLE::WRITE_CONFIRMED, 1);
    if( buttonTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println(F("Failed to add button tank."));
        delay(10); exit(1);
    }

    Serial.println(F("Added all data tanks"));
    Serial.print(F("Temperature measure period Tank ID: ")); Serial.println(tempMeasPeriodId);
    Serial.print(F("Temperature Tank ID: ")); Serial.println(tempId);
    Serial.print(F("LCD display output Tank ID: ")); Serial.println(lcdTankId);
    Serial.print(F("Button Tank ID: ")); Serial.println(buttonTankId);

    Serial.println(F("Starting advertisement."));
    ble.setDeviceName("SimpleBLE example");
    ble.startAdvertisement(300);
}

void loop()
{
    SimpleBLE::TankData updatedTank = ble.manageUpdates((tempMeasPeriod > 0 ? tempMeasPeriod : 2)*1000);

    Serial.print(F("Got update from tank ")); Serial.print(updatedTank.getId());
    Serial.print(F(" data ")); Serial.println(updatedTank[0]);

    if( updatedTank.getId() == tempMeasPeriodId )
    {
        tempMeasPeriod = atoi(updatedTank.getCString());
        Serial.print(F("New temperature update period in seconds: ")); Serial.println(tempMeasPeriod);
    }
    else if( updatedTank.getId() == lcdTankId )
    {
        // Clear the display and print the string from app.
        lcdPrintBottomRow("                ");
        lcdPrintBottomRow(updatedTank.getCString());
    }
    else if( updatedTank.getId() == buttonTankId )
    {
        int tmpBtnState = updatedTank.getInt();

        if( tmpBtnState == 0 )
        {
            setRelay(true);
        }
        else if( tmpBtnState == 2 )
        {
            setRelay(false);
        }
    }
    else
    {
        int temperature = measureTemp();
        String temperatureStr(temperature);
        String tempPrint = String("Temp: ") + temperatureStr + String(" C ");
        lcdPrintTopRow(tempPrint.c_str());
        ble.writeTank(tempId, temperatureStr.c_str(), temperatureStr.length());
        Serial.println(tempPrint);
    }
}
