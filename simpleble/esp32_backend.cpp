#include "esp32_backend.h"
#include "timeout.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <string.h>

#define BASE_SERVER_UUID "91ba0000-b950-4226-aa2b-4ede9fa42f59"

BLEUUID baseUuid(BASE_SERVER_UUID);
BLEUUID notifyDescUuid((uint16_t)0x2902);


class SimpleBLEServerCallbacks: public BLEServerCallbacks
{
public:
    SimpleBLEServerCallbacks(Esp32Backend* owner) : owner(owner) {}

    Esp32Backend* owner;

    void onConnect(BLEServer* pServer)
    {
        //deviceConnected = true;
    }
    void onDisconnect(BLEServer* pServer)
    {
        //deviceConnected = false;
        if( owner->restartAdvOnDisc )
            pServer->getAdvertising()->start();
    }
};

class SimpleBLECharCallbacks: public BLECharacteristicCallbacks
{
public:
    SimpleBLECharCallbacks(Esp32Backend* owner) : owner(owner) {}

    Esp32Backend* owner;

    void onWrite(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        (void)param;

        BLEUUID charUuid = pCharacteristic->getUUID();

        int8_t servIndex;
        int8_t charIndex = getServCharIdx(charUuid, servIndex);

        if( charIndex >= 0)
        {
            owner->receivedData[servIndex].setFlag(charIndex);
        }
    }
    void onRead(BLECharacteristic *pCharacteristic, esp_ble_gatts_cb_param_t *param)
    {
        (void)param;

        BLEUUID charUuid = pCharacteristic->getUUID();

        int8_t servIndex;
        int8_t charIndex = getServCharIdx(charUuid, servIndex);

        if( charIndex >= 0)
        {
            owner->readData[servIndex].setFlag(charIndex);
        }
    }

    int8_t getServCharIdx(BLEUUID& charUuid, int8_t& servIndex)
    {
        uint8_t charIndex = -1;
        for(servIndex = 0; servIndex < owner->servNum; servIndex++)
        {
            if( owner->services[servIndex].serv->getCharacteristic(charUuid) != nullptr )
            {
                BLEUUID servUuid = owner->services[servIndex].serv->getUUID();
                const uint8_t uuidLen = servUuid.getNative()->len;
                charIndex = charUuid.getNative()->uuid.uuid128[uuidLen - 4] -
                            servUuid.getNative()->uuid.uuid128[uuidLen - 4] -
                            1;
                break;
            }
        }

        return charIndex;
    }
};

Esp32Backend::Esp32Backend(const Esp32BackendInterface *ifc) :
    ifc(ifc),
    servNum(0),
    restartAdvOnDisc(false)
{
    Timeout::init(ifc->millis);

    pServer = NULL;
}

void Esp32Backend::activateModuleRx(void)
{
    //TODO: implement going out of sleep
}
void Esp32Backend::deactivateModuleRx(void)
{
    //TODO: implement going to sleep
}
void Esp32Backend::hardResetModule(void)
{
    // Leave empty for compatibility
}

void Esp32Backend::begin()
{
    BLEDevice::init("");

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new SimpleBLEServerCallbacks(this));

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

    // Set custom advertising data
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMaxPreferred(0x12);
}

bool Esp32Backend::softRestart(void)
{
    bool retval = true;
/*
    char cmdStr[20]; cmdStr[0] = '\0';
    strcat(cmdStr, "AT+RESTART");

    if( sendReceiveCmd(cmdStr) != AtProcess::SUCCESS )
    {
        retval = false;
    }
    else
    {
        at.waitURC("^START", NULL, 0, 5000);
    }
*/
    return retval;
}

bool Esp32Backend::startAdvertisement(uint32_t advPeriodMs,
                                   int32_t advDurationMs,
                                   bool restartOnDisc)
{
    static bool servicesStarted = false;

    const uint32_t minAdvIntIncrementsUs = 625; // microseconds [us]

    bool retval = true;

    (void)advDurationMs; // For now we don't implement advertising duration on ESP

    // Get advertising object
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

    // Set advertising interval (in milliseconds)
    pAdvertising->setMinInterval(advPeriodMs*1000/minAdvIntIncrementsUs);
    pAdvertising->setMaxInterval(advPeriodMs*1000/minAdvIntIncrementsUs);

    if( !servicesStarted )
    {
        for(uint8_t i = 0; i < servNum; i++)
        {
            services[i].serv->start();
        }
        servicesStarted = true;
    }

    restartAdvOnDisc = restartOnDisc;

    // Start advertising with the configured interval
    pAdvertising->start();

    return retval;
}

bool Esp32Backend::stopAdvertisement(void)
{
    bool retval = true;

    // Get advertising object
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

    // Stop advertising after the specified duration
    pAdvertising->stop();

    return retval;
}

bool Esp32Backend::setAdvPayload(AdvType type, uint8_t *data, uint32_t dataLen)
{
    bool retval = true;

    // Get advertising object
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();

    // Custom advertising data
    BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

    char cdata[2];
    cdata[0] = dataLen + 1;
    cdata[1] = (esp_ble_adv_data_type)type;
    oAdvertisementData.addData(String(cdata, 2) + String((char*)data, dataLen));
    //oAdvertisementData.setFlags(0x06); // General discoverable mode, BR/EDR not supported

    // Manufacturer specific data
    //std::string manufacturerData = "Hello";
    //oAdvertisementData.addManufacturerData(manufacturerData);

    // Setting the custom data as the advertisement payload
    pAdvertising->setAdvertisementData(oAdvertisementData);

    return retval;
}

bool Esp32Backend::setTxPower(TxPower dbm)
{
    bool retval = true;

    esp_power_level_t espDbm;
    switch(dbm)
    {
        case POW_N40DBM:
        case POW_N20DBM:
        case POW_N16DBM:
        case POW_N12DBM: espDbm = ESP_PWR_LVL_N12; break;
        case POW_N9DBM:
        case POW_N8DBM:  espDbm = ESP_PWR_LVL_N9; break;
        case POW_N6DBM:  espDbm = ESP_PWR_LVL_N6; break;
        case POW_N4DBM:
        case POW_N3DBM:  espDbm = ESP_PWR_LVL_N3; break;
        case POW_0DBM:   espDbm = ESP_PWR_LVL_N0; break;
        case POW_2DBM:
        case POW_3DBM:
        case POW_4DBM:   espDbm = ESP_PWR_LVL_P3; break;
        case POW_6DBM:   espDbm = ESP_PWR_LVL_P6; break;
        case POW_9DBM:   espDbm = ESP_PWR_LVL_P9; break;
        default:         espDbm = ESP_PWR_LVL_N0; break;
    }

    BLEDevice::setPower(espDbm);

    return retval;
}

int8_t Esp32Backend::addService(uint8_t servUuid)
{
    int8_t servIndex = INVALID_SERVICE_INDEX;

    if( servNum < MAX_NUM_SERVICES )
    {
        servIndex = servNum;
        servNum++;
        BLEUUID servUuidFull = baseUuid;
        servUuidFull.getNative()->uuid.uuid128[servUuidFull.getNative()->len - 3] = servUuid;
        services[servIndex].serv = pServer->createService(servUuidFull);
        services[servIndex].charNum = 0;
    }

    return servIndex;
}

int8_t Esp32Backend::addChar(uint8_t serviceIndex, uint32_t maxSize, CharPropFlags flags)
{
    int8_t charIndex = -1;

    // We don't use max size in ESP because it dinamicaly allocates required size.
    (void)maxSize;

    uint32_t espProps = 0;

    espProps |= (flags & CharPropFlags::BROADCAST) ? BLECharacteristic::PROPERTY_BROADCAST : 0 ;
    espProps |= (flags & CharPropFlags::READ) ? BLECharacteristic::PROPERTY_READ : 0 ;
    espProps |= (flags & CharPropFlags::WRITE_WITHOUT_RESPONSE) ? BLECharacteristic::PROPERTY_WRITE_NR : 0 ;
    espProps |= (flags & CharPropFlags::WRITE) ? BLECharacteristic::PROPERTY_WRITE : 0 ;
    espProps |= (flags & CharPropFlags::NOTIFY) ? BLECharacteristic::PROPERTY_NOTIFY : 0 ;
    espProps |= (flags & CharPropFlags::INDICATE) ? BLECharacteristic::PROPERTY_INDICATE : 0 ;

    bool notifies = (flags & CharPropFlags::NOTIFY) ? true : false ;

    if( serviceIndex < servNum )
    {
        charIndex = services[serviceIndex].charNum;
        services[serviceIndex].charNum++;
        BLEUUID charUuid = charUuidFromIndex(serviceIndex, charIndex);
        BLECharacteristic* newChar = services[serviceIndex].serv->createCharacteristic(charUuid, espProps);
        newChar->setCallbacks(new SimpleBLECharCallbacks(this));
        if( notifies )
        {
            newChar->addDescriptor(new BLEDescriptor(notifyDescUuid));
        }
    }

    return charIndex;
}

int32_t Esp32Backend::checkChar(uint8_t serviceIndex, uint8_t charIndex)
{
    return readChar(serviceIndex, charIndex, NULL, 0);
}

int32_t Esp32Backend::readChar(uint8_t serviceIndex, uint8_t charIndex,
                            uint8_t *buff, uint32_t buffSize)
{
    int32_t readBytes = 0;

    bool returnData = buff ? true : false ;

    BLECharacteristic* characteristic = getCharacteristic(serviceIndex, charIndex);

    if( characteristic )
    {
        if( returnData )
        {
            uint8_t* charData = characteristic->getData();

            // Read all characteristic bytes if buffer is large enough, otherwise
            // just fill the buffer.
            uint32_t toRead = 
                buffSize > characteristic->getLength() ?
                characteristic->getLength() :
                buffSize ;
            for(; readBytes < toRead; readBytes++)
            {
                buff[readBytes] = charData[readBytes];
            }

            receivedData[serviceIndex].rstFlag(charIndex);
        }
        else
        {
            readBytes = characteristic->getLength();

            // If there is no new data to be read, make bytes available to
            // read negative.
            if( !receivedData[serviceIndex].getFlag(charIndex) )
            {
                readBytes *= -1;
            }
        }
    }

    return readBytes;
}

bool Esp32Backend::writeChar(uint8_t serviceIndex, uint8_t charIndex,
                             const uint8_t *data, uint32_t dataSize)
{
    bool retval = false;

    BLECharacteristic* characteristic = getCharacteristic(serviceIndex, charIndex);

    // Data just gets coppied so it is safe to cast it from const here.
    characteristic->setValue((uint8_t*)data, dataSize);

    readData[serviceIndex].rstFlag(charIndex);

    // Check if notify descriptor present and if it is send a notification.
    if( characteristic->getDescriptorByUUID(notifyDescUuid) )
    {
        characteristic->notify();
    }

    return retval;
}

bool Esp32Backend::waitCharUpdate(uint8_t* serviceIndex, uint8_t* charIndex,
                                  uint32_t* dataSize, uint32_t timeout)
{
    bool retval = false;

    *dataSize = 0;

    Timeout waitCharUpdate(timeout);

    while( waitCharUpdate.notExpired() )
    {
        for( *serviceIndex = 0; *serviceIndex < servNum; (*serviceIndex)++ )
        {
            const uint8_t serviceNumberOfChars = services[*serviceIndex].charNum;
            for( *charIndex = 0; *charIndex < serviceNumberOfChars; (*charIndex)++ )
            {
                if( receivedData[*serviceIndex].getFlag(*charIndex) )
                {
                    BLECharacteristic* characteristic = getCharacteristic(*serviceIndex, *charIndex);
                    *dataSize = characteristic->getLength();
                    retval = true;
                    break;
                }
            }
            if( retval )
            {
                break;
            }
        }
        if( retval )
        {
            break;
        }
        ifc->delayMs(2);
    }

    return retval;
}


void Esp32Backend::debugPrint(const char *str)
{
    internalDebug(str);
    internalDebug("\r\n");
}

BLEUUID Esp32Backend::charUuidFromIndex(uint8_t servIndex, uint8_t charIndex)
{
    return charUuidFromIndex(services[servIndex].serv->getUUID(), charIndex);
}
BLEUUID Esp32Backend::charUuidFromIndex(BLEUUID servUuid, uint8_t charIndex)
{
    servUuid.getNative()->uuid.uuid128[servUuid.getNative()->len - 4] = charIndex + 1;
    return servUuid;
}

BLECharacteristic* Esp32Backend::getCharacteristic(uint8_t servIndex, uint8_t charIndex)
{
    BLEService* service = services[servIndex].serv;
    return service->getCharacteristic(charUuidFromIndex(servIndex, charIndex));
}

