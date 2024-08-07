#include "Arduino.h"
#include "LiquidCrystal_I2C.h"

#include <DHT11.h>

#include "simpleble/simple_ble.h"


DHT11 dht11(3);
//float measuredTemp;

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
void measureTemp()
{
  //sensors_event_t event;
  //dht.temperature().getEvent(&event);
  //if(!isnan(event.temperature))
  //{
  //  measuredTemp = event.temperature;
  //}
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

    Serial.println("Example started");

    // Begin BLE. UART speed is at 9600 by default
    if( !ble.begin() )
    {
        Serial.println("Failed to initialise SimpleBLE.");
        delay(10); exit(1);
    }

    tempMeasPeriodId = ble.addTank(SimpleBLE::WRITE_CONFIRMED, 15);
    if( tempMeasPeriodId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println("Failed to add temperature measurement period tank.");
        delay(10); exit(1);
    }
    
    tempId = ble.addTank(SimpleBLE::READ, 15);
    if( tempId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println("Failed to add temperature tank.");
        delay(10); exit(1);
    }

    lcdTankId = ble.addTank(SimpleBLE::WRITE_CONFIRMED, 16);
    if( lcdTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println("Failed to add LCD display output tank.");
        delay(10); exit(1);
    }

    buttonTankId = ble.addTank(SimpleBLE::WRITE_CONFIRMED, 1);
    if( buttonTankId == SimpleBLE::INVALID_TANK_ID )
    {
        Serial.println("Failed to add button tank.");
        delay(10); exit(1);
    }

    Serial.println("Added all data tanks");
    Serial.print("Temperature measure period Tank ID: "); Serial.println(tempMeasPeriodId);
    Serial.print("Temperature Tank ID: "); Serial.println(tempId);
    Serial.print("LCD display output Tank ID: "); Serial.println(lcdTankId);
    Serial.print("Button Tank ID: "); Serial.println(buttonTankId);

    Serial.println("Starting advertisement.");
    ble.setDeviceName("SimpleBLE example");
    ble.startAdvertisement(300);
}

uint8_t tempMeasPeriod = 0;

void loop()
{
    SimpleBLE::TankData updatedTank = ble.manageUpdates((tempMeasPeriod > 0 ? tempMeasPeriod : 2)*1000);

    Serial.print("Got update from tank "); Serial.print(updatedTank.getId());
    Serial.print(" data "); Serial.println(updatedTank[0]);

    if( updatedTank.getId() == tempMeasPeriodId )
    {
        char tmpTempMeasStr[20];
        memcpy(tmpTempMeasStr, updatedTank.getData(), updatedTank.getSize());
        tmpTempMeasStr[updatedTank.getSize()] = '\0';
        tempMeasPeriod = atoi(tmpTempMeasStr);
    }
    else if( updatedTank.getId() == lcdTankId )
    {
        char tmpLcdStr[20];
        memcpy(tmpLcdStr, updatedTank.getData(), updatedTank.getSize());
        tmpLcdStr[updatedTank.getSize()] = '\0';

        // Clear the display and print the string from app.
        lcdPrintBottomRow("                ");
        lcdPrintBottomRow(tmpLcdStr);
    }
    else if( updatedTank.getId() == buttonTankId )
    {
        uint8_t tmpBtnState = updatedTank[0];

        // For catching fast updates, our UART speed is a bit too low
        SimpleBLE::TankData updatedTank = ble.manageUpdates(10);
        if( updatedTank.getId() == buttonTankId )
            tmpBtnState = updatedTank[0];

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
        int temperature = dht11.readTemperature();
        if (temperature != DHT11::ERROR_CHECKSUM && temperature != DHT11::ERROR_TIMEOUT)
        {
            String temperatureStr(temperature);
            String tempPrint = String("Temp: ") + temperatureStr + String(" C ");
            lcdPrintTopRow(tempPrint.c_str());
            ble.writeTank(tempId, temperatureStr.c_str(), temperatureStr.length());
            //Serial.println(tempPrint);
        }
    }
}
