#include "simple_ble.h"
#include "at_process.h"

#include <stdlib.h>
#include <string.h>


#define MODULE_RX_BLOCK_SIZE_B                                  (6)

static const char cmdEnding[] = "\r";
static const char cmdAck[] = "\nOK\r\n";
static const char cmdError[] = "ERROR\r\n";



#ifdef ARDUINO
static const SimpleBLEInterface SimpleBLE::arduinoIf = {
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
#endif //ARDUINO

bool SimpleBLE::begin()
{
    bool retval = false;

#ifdef ARDUINO
    pinMode(RX_ENABLE_PIN, OUTPUT);
    pinMode(MODULE_RESET_PIN, OUTPUT);

    altSerial.begin(9600);
#endif //ARDUINO

    backend.begin();

    exitUltraLowPower();

do{
    if( softRestart() )
    {
        break;
    }

    if( setTxPower(SimpleBLE::POW_0DBM) )
    {
        break;
    }

    tanksServiceIndex = backend.addService(TANKS_SERVICE_UUID);
    if( tanksServiceIndex == SimpleBLEBackend::INVALID_SERVICE_INDEX )
    {
        break;
    }

    retval = true;
}while(0);

    return retval;
}

SimpleBLE::TankId SimpleBLE::addTank(SimpleBLE::TankType type, uint32_t maxSizeBytes)
{
    SimpleBLEBackend::CharPropFlags charFlags = SimpleBLEBackend::NONE;

    switch (type)
    {
        case SimpleBLE::READ :
            charFlags = SimpleBLEBackend::READ | SimpleBLEBackend::NOTIFY;
            break;
        case SimpleBLE::WRITE :
            charFlags = SimpleBLEBackend::WRITE_WITHOUT_RESPONSE;
            break;
        case SimpleBLE::WRITE_WITH_RESPONSE :
            charFlags = SimpleBLEBackend::WRITE;
            break;

        default:
            break;
    }

    SimpleBLE::TankId newTankId = INVALID_TANK_ID;

    if( charFlags != SimpleBLEBackend::NONE )
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
    SimpleBLEBackend::TxPower bkdDbm = SimpleBLEBackend::POW_0DBM;

    switch (dbm)
    {
        case SimpleBLE::POW_N40DBM : bkdDbm = SimpleBLEBackend::POW_N40DBM; break;
        case SimpleBLE::POW_N20DBM : bkdDbm = SimpleBLEBackend::POW_N20DBM; break;
        case SimpleBLE::POW_N16DBM : bkdDbm = SimpleBLEBackend::POW_N16DBM; break;
        case SimpleBLE::POW_N12DBM : bkdDbm = SimpleBLEBackend::POW_N12DBM; break;
        case SimpleBLE::POW_N8DBM : bkdDbm = SimpleBLEBackend::POW_N8DBM; break;
        case SimpleBLE::POW_N4DBM : bkdDbm = SimpleBLEBackend::POW_N4DBM; break;
        case SimpleBLE::POW_0DBM : bkdDbm = SimpleBLEBackend::POW_0DBM; break;
        case SimpleBLE::POW_2DBM : bkdDbm = SimpleBLEBackend::POW_2DBM; break;
        case SimpleBLE::POW_3DBM : bkdDbm = SimpleBLEBackend::POW_3DBM; break;
        case SimpleBLE::POW_4DBM : bkdDbm = SimpleBLEBackend::POW_4DBM; break;

        default :
            bkdDbm = SimpleBLEBackend::POW_0DBM;
            break;
    }

    return backend.setTxPower(bkdDbm);
}

bool SimpleBLE::setDeviceName(const char* newName)
{
    return backend.setAdvPayload(
            SimpleBLEBackend::COMPLETE_LOCAL_NAME,
            (uint8_t*)newName,
            strlen(newName));
}
