#include "simple_ble.h"
#include "at_process.h"

#include <string.h>


static const char cmdEnding[] = "\r";
static const char cmdAck[] = "\nOK\r\n";
static const char cmdError[] = "ERROR\r\n";


#ifdef USING_ARDUINO_INTRFACE
#ifdef USING_ESP32_BACKEND
const Esp32BackendInterface SimpleBLE::arduinoIf = {
    [](void) { return (uint32_t)millis(); },
    [](uint32_t ms) { delay(ms); },
    //[](const char *dbg) { Serial.print(dbg); }
    NULL
};
#else
static const SimpleBLEBackendInterface SimpleBLE::arduinoIf = {
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
    //[](const char *dbg) { Serial.print(dbg); }
    NULL
};
#endif //USING_ESP32_BACKEND
#endif //USING_ARDUINO_INTRFACE

bool SimpleBLE::begin()
{
    bool retval = false;

#ifdef USING_ARDUINO_INTRFACE
#ifndef USING_ESP32_BACKEND
    pinMode(RX_ENABLE_PIN, OUTPUT);
    pinMode(MODULE_RESET_PIN, OUTPUT);

    arduinoIf.rxEnabledSet(true);
    arduinoIf.moduleResetSet(true);
    altSerial.begin(9600);
#endif //USING_ESP32_BACKEND

    arduinoIf.delayMs(500);
#endif //USING_ARDUINO_INTRFACE

    backend.begin();

    exitUltraLowPower();

do{
    if( !softRestart() )
    {
        break;
    }

    if( !setTxPower(SimpleBLE::POW_0DBM) )
    {
        break;
    }

    tanksServiceIndex = backend.addService(TANKS_SERVICE_UUID);
    if( tanksServiceIndex == BackendNs::INVALID_SERVICE_INDEX )
    {
        break;
    }

    retval = true;
}while(0);

    return retval;
}

SimpleBLE::TankId SimpleBLE::addTank(SimpleBLE::TankType type, uint32_t maxSizeBytes)
{
    BackendNs::CharPropFlags charFlags = BackendNs::NONE;

    switch (type)
    {
        case SimpleBLE::READ :
            charFlags = BackendNs::READ_AND_NOTIFY;
            break;
        case SimpleBLE::WRITE :
            charFlags = BackendNs::WRITE_WITHOUT_RESPONSE;
            break;
        case SimpleBLE::WRITE_CONFIRMED :
            charFlags = BackendNs::WRITE;
            break;

        default:
            break;
    }

    SimpleBLE::TankId newTankId = INVALID_TANK_ID;

    if( charFlags != BackendNs::NONE )
    {
        newTankId = backend.addChar(
            tanksServiceIndex,
            maxSizeBytes,
            charFlags);
    }

    return newTankId;
}

bool SimpleBLE::setTxPower(SimpleBLE::TxPower dbm)
{
    BackendNs::TxPower bkdDbm = BackendNs::POW_0DBM;

    switch (dbm)
    {
        case SimpleBLE::POW_N40DBM : bkdDbm = BackendNs::POW_N40DBM; break;
        case SimpleBLE::POW_N20DBM : bkdDbm = BackendNs::POW_N20DBM; break;
        case SimpleBLE::POW_N16DBM : bkdDbm = BackendNs::POW_N16DBM; break;
        case SimpleBLE::POW_N12DBM : bkdDbm = BackendNs::POW_N12DBM; break;
        case SimpleBLE::POW_N8DBM : bkdDbm = BackendNs::POW_N8DBM; break;
        case SimpleBLE::POW_N4DBM : bkdDbm = BackendNs::POW_N4DBM; break;
        case SimpleBLE::POW_0DBM : bkdDbm = BackendNs::POW_0DBM; break;
        case SimpleBLE::POW_2DBM : bkdDbm = BackendNs::POW_2DBM; break;
        case SimpleBLE::POW_3DBM : bkdDbm = BackendNs::POW_3DBM; break;
        case SimpleBLE::POW_4DBM : bkdDbm = BackendNs::POW_4DBM; break;

        default :
            bkdDbm = BackendNs::POW_0DBM;
            break;
    }

    return backend.setTxPower(bkdDbm);
}

bool SimpleBLE::setDeviceName(const char* newName)
{
    return backend.setAdvPayload(
            BackendNs::COMPLETE_LOCAL_NAME,
            (uint8_t*)newName,
            strlen(newName));
}

bool SimpleBLE::waitUpdates(TankId* tank, uint32_t* updateSize, uint32_t timeout)
{
    uint8_t serviceIndex; uint8_t charIndex; uint32_t dataSize;
    bool retval = backend.waitCharUpdate(&serviceIndex, &charIndex, &dataSize, timeout);

    if( retval && serviceIndex == tanksServiceIndex )
    {
        *tank = charIndex;

        if( updateSize ) *updateSize = dataSize;
    }

    return retval;
}

bool SimpleBLE::readTank(TankId tank, uint8_t *buff, uint32_t buffSize, uint32_t* readLen)
{
    int32_t internalReadLen = backend.readChar(tanksServiceIndex, (uint8_t)tank, buff, buffSize);

    bool retval = internalReadLen <= buffSize;

    if( readLen )
    {
        *readLen = internalReadLen;
    }

    return retval;
}

bool SimpleBLE::writeTank(TankId tank, const uint8_t *data, uint32_t dataSize)
{
    return backend.writeChar(tanksServiceIndex, (uint8_t)tank, data, dataSize);
}

bool SimpleBLE::writeTank(TankId tank, const char *str)
{
    return writeTank(tank, (const uint8_t*)str, strlen(str));
}

#ifdef USING_ARDUINO_INTRFACE
SimpleBLE::TankData SimpleBLE::manageUpdates(uint32_t timeout)
{
    SimpleBLE::TankId updatedTankId;
    uint32_t updatedSize;

    if( waitUpdates(&updatedTankId, &updatedSize, timeout) )
    {
        SimpleBLE::TankData updatedTank(updatedTankId, updatedSize);

        uint32_t dummyReadLen; // We know how much we will read since waitUpdates() gave the size
        if( readTank(updatedTank.getId(), updatedTank.getData(), updatedTank.getSize(), &dummyReadLen) )
        {
            return updatedTank;
        }
    }

    return SimpleBLE::TankData(0);
}
#endif //USING_ARDUINO_INTRFACE
