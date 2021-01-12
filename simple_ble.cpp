#include "simple_ble.h"
#include "at_process.h"

#include <stdlib.h>
#include <string.h>


#define MODULE_RX_BLOCK_SIZE_B                                  (10)


/******************************PORTING INTERFACE*******************************/

/*
#include "Arduino.h"
#include "SoftwareSerial.h"

static int internalRxPin = -1;
static int internalTxPin = -1;
static int internalRxEnablePin = -1;
static int internalModuleResetPin = -1;

static Stream *serial = NULL;
static Stream *debug = NULL;
static SoftwareSerial softSerial(internalRxPin, internalRxPin);

static void internalPortingInit(void)
{
    // If using pins for hardware uart.
    if( internalRxPin == 0 && internalTxPin == 1 )
    {
        serial = &Serial;

        Serial.begin(9600);
    }
    else
    {
        softSerial = SoftwareSerial(internalRxPin, internalTxPin);

        softSerial.begin(9600);

        serial = &softSerial;
        debug = &Serial;
    }

    if( internalRxEnablePin >= 0 )
    {
        pinMode(internalRxEnablePin, OUTPUT);
    }
    if( internalModuleResetPin >= 0 )
    {
        pinMode(internalModuleResetPin, OUTPUT);
    }
}
// Timing ports
static uint32_t internalMillis(void)
{
    return millis();
}
static void internalDelay(uint32_t ms)
{
    delay(ms);
}
// Debug
static void internalDebug(const char *dbgPrint)
{
    if( debug )
    {
        debug->print(dbgPrint);
    }
}
// Serial ports
static bool internalSerPut(char c)
{
    return serial->write(c) > 0;
}
static bool internalSerGet(char *c)
{
    bool availableChars = serial->available() > 0;

    *c = availableChars ? serial->read() : *c ;

    return availableChars;
}
//GPIO ports
static void internalSetRxEnable(bool state)
{
    // NOTE: active high!!
    if( internalRxEnablePin >= 0 )
    {
        digitalWrite(internalRxEnablePin, state ? HIGH : LOW);
    }
}
static void internalSetModuleReset(bool state)
{
    // NOTE: active low!!
    if( internalModuleResetPin >= 0 )
    {
        digitalWrite(internalModuleResetPin, state ? HIGH : LOW);
    }
}
*/



/******************************PORTING INTERFACE*******************************/

/*
SimpleBLE::SimpleBLE(int rxPin, int txPin, int rxEnablePin, int moduleResetPin)
{
    internalRxPin = rxPin;
    internalTxPin = txPin;
    internalRxEnablePin = rxEnablePin;
    internalModuleResetPin = moduleResetPin;
    
    SerialUART uart =
    {
        .serPut = internalSerPut,
        .serGet = internalSerGet
    };

    at = AtProcess(&uart, internalDelay);
}
*/

/*
SimpleBLE::SimpleBLE(GenericGpioSetter *rxEnabledSetter,
                     GenericGpioSetter *moduleResetSetter,
                     SerialPut *serialPutter,
                     SerialGet *serialGetter,
                     MillisCounter *millisCounterGetter,
                     MillisecondDelay *delayer,
                     DebugPrint *debugPrinter
)
{
    SerialUART uart =
    {
        .serPut = serialPutter,
        .serGet = serialGetter
    };

    at = AtProcess(&uart, internalDelay);
}
*/

void SimpleBLE::activateModuleRx(void)
{
    internalSetRxEnable(true);
}
void SimpleBLE::deactivateModuleRx(void)
{
    //internalSetRxEnable(false);
    internalSetRxEnable(true);
}
void SimpleBLE::hardResetModule(void)
{
    internalSetModuleReset(false);
    internalDelay(1);
    internalSetModuleReset(true);
}

void SimpleBLE::begin(void)
{
    //Timeout::init(internalMillis);

    //internalPortingInit();

    at.init();

    deactivateModuleRx();
    hardResetModule();
}

AtProcess::Status SimpleBLE::sendReceiveCmd(const char *cmd,
                                            uint32_t timeout)
{
    return sendReceiveCmd(cmd, NULL, 0, false, timeout);
}

AtProcess::Status SimpleBLE::sendReadReceiveCmd(const char *cmd,
                                                uint8_t *buff,
                                                uint32_t buffSize,
                                                uint32_t timeout)
{
    return sendReceiveCmd(cmd, buff, buffSize, true, timeout);
}

AtProcess::Status SimpleBLE::sendWriteReceiveCmd(const char *cmd,
                                                 uint8_t *data,
                                                 uint32_t dataSize,
                                                 uint32_t timeout)
{
    return sendReceiveCmd(cmd, data, dataSize, false, timeout);
}


AtProcess::Status SimpleBLE::sendReceiveCmd(const char *cmd,
                                            uint8_t *buff,
                                            uint32_t size,
                                            bool readNWrite,
                                            uint32_t timeout)
{
    AtProcess::Status cmdStatus = AtProcess::GEN_ERROR;

    activateModuleRx();

    // We will get an echo of this command uninterrupted with URCs because we
    // send it quick.
    uint32_t sent = at.sendCommand(cmd);

    if( buff )
    {
        if( !readNWrite )
        {
            // We are writing.
            sent += at.write(buff, size);
        }
    }

    if( sent > 0 )
    {
        // Calculate block fill chars.
        sent--;
        while(sent++ % MODULE_RX_BLOCK_SIZE_B)
        {
            at.write((uint8_t)'\r');
        }

        // Now is the time to start checking for read data.
        if( buff )
        {
            if( readNWrite )
            {
                // We are reading.
                AtProcess::Status lineStatus;

                do
                {
                    lineStatus = at.recvResponse(1000, AtProcess::URC);

                }while(strncmp(cmd, at.getLastStatus(), strlen(cmd)) != 0 ||
                    lineStatus != AtProcess::TIMEOUT
                );

                if( lineStatus != AtProcess::TIMEOUT )
                {
                    // Read one line because it is still not the data.
                    lineStatus = at.recvResponse(1000, AtProcess::URC);

                    at.readBytesBlocking(buff, size);
                }
            }
        }

        cmdStatus = at.recvResponse(timeout);
    }

    deactivateModuleRx();

    return cmdStatus;
}


bool SimpleBLE::softRestart(void)
{
    bool retval = true;

    char cmdStr[20] = "AT+RESTART";

    if( sendReceiveCmd(cmdStr) != AtProcess::SUCCESS )
    {
        debugPrint(at.getLastStatus());
        retval = false;
    }
    else
    {
        at.waitURC("^START", 5000);
    }

    return retval;
}

bool SimpleBLE::startAdvertisement(uint32_t advPeriod,
                                   int32_t advDuration,
                                   bool restartOnDisc)
{
    bool retval = true;

    char cmdStr[50] = "AT+ADVSTART=";
    char helpStr[20];

    strcat(cmdStr, itoa(advPeriod, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(advDuration, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa((unsigned)restartOnDisc, helpStr, 10));

    if( sendReceiveCmd(cmdStr) != AtProcess::SUCCESS )
    {
        debugPrint(at.getLastStatus());
        retval = false;
    }

    return retval;
}

bool SimpleBLE::stopAdvertisement(void)
{
    bool retval = true;

    char cmdStr[20] = "AT+ADVSTOP";

    if( sendReceiveCmd(cmdStr) != AtProcess::SUCCESS )
    {
        debugPrint(at.getLastStatus());
        retval = false;
    }

    return retval;
}

bool SimpleBLE::setAdvPayload(AdvType type, uint8_t *data, uint32_t dataLen)
{
    bool retval = true;

    char cmdStr[50] = "AT+ADVPAYLOAD=";
    char helpStr[20];

    strcat(cmdStr, itoa(type, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(dataLen, helpStr, 10));

    if( sendWriteReceiveCmd(cmdStr, data, dataLen) != AtProcess::SUCCESS )
    {
        debugPrint(at.getLastStatus());
        retval = false;
    }

    return retval;
}

bool SimpleBLE::setTxPower(TxPower dbm)
{
    bool retval = true;

    char cmdStr[50] = "AT+TXPOWER=";
    char helpStr[20];

    strcat(cmdStr, itoa(dbm, helpStr, 10));

    if( sendReceiveCmd(cmdStr) != AtProcess::SUCCESS )
    {
        debugPrint(at.getLastStatus());
        retval = false;
    }

    return retval;
}

int SimpleBLE::addService(uint8_t servUuid)
{
    int srvIndex = -1;

    char cmdStr[50] = "AT+ADDSRV=";
    char helpStr[20];

    strcat(cmdStr, itoa(servUuid, helpStr, 10));

    if( sendReceiveCmd(cmdStr) == AtProcess::SUCCESS )
    {
        debugPrint(at.getLastStatus());

        const char *retStatus = findCmdReturnStatus(at.getLastStatus(), "^ADDSRV:");

        if( retStatus )
        {
            srvIndex = atoi(retStatus);
        }
    }

    return srvIndex;
}

int SimpleBLE::addChar(uint8_t serviceIndex, uint32_t maxSize, CharPropFlags flags)
{
    int charIndex = -1;

    char cmdStr[50] = "AT+ADDCHAR=";
    char helpStr[20];

    strcat(cmdStr, itoa(serviceIndex, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(maxSize, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(flags, helpStr, 10));

    if( sendReceiveCmd(cmdStr) == AtProcess::SUCCESS )
    {
        debugPrint(at.getLastStatus());

        const char *retStatus = findCmdReturnStatus(at.getLastStatus(), "^ADDCHAR:");

        if( retStatus )
        {
            charIndex = atoi(retStatus);
        }
    }

    return charIndex;
}

int32_t SimpleBLE::checkChar(uint8_t serviceIndex, uint8_t charIndex)
{
    return readChar(serviceIndex, charIndex, NULL, 0);
}

int32_t SimpleBLE::readChar(uint8_t serviceIndex, uint8_t charIndex,
                            uint8_t *buff, uint32_t buffSize)
{
    char cmdStr[50] = "AT+READCHAR=";
    char helpStr[20];

    uint32_t readBytes = buffSize;

    bool returnData = buff ? true : false ;

    strcat(cmdStr, itoa(serviceIndex, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(charIndex, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(returnData, helpStr, 10));


    if( returnData )
    {
        if( sendReadReceiveCmd(cmdStr, buff, buffSize) == AtProcess::SUCCESS )
        {
            debugPrint(at.getLastStatus());
        }
    }
    else
    {
        if( sendReceiveCmd(cmdStr) == AtProcess::SUCCESS )
        {
            const char *retStatus = findCmdReturnStatus(at.getLastStatus(), "^READCHAR:");

            if( retStatus )
            {
                readBytes = atoi(retStatus);

                bool newData = atoi(strpbrk(retStatus, ",")+1);

                // If there is no new data to be read, make bytes available to
                // read negative.
                if( !newData )
                {
                    readBytes *= -1;
                }
            }
        }
    }

    return readBytes;
}

bool SimpleBLE::writeChar(uint8_t serviceIndex, uint8_t charIndex,
                          uint8_t *data, uint32_t dataSize)
{
    bool retval = false;

    char cmdStr[50] = "AT+WRITECHAR=";
    char helpStr[20];

    strcat(cmdStr, itoa(serviceIndex, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(charIndex, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(dataSize, helpStr, 10));


    if( sendWriteReceiveCmd(cmdStr, data, dataSize) == AtProcess::SUCCESS )
    {
        debugPrint(at.getLastStatus());

        retval = true;
    }

    return retval;
}


const char *SimpleBLE::findCmdReturnStatus(const char *cmdRet, const char *statStart)
{
    const char *retStatus = strstr(cmdRet, statStart);

    if( retStatus )
    {
        retStatus = strpbrk(retStatus, statStart[strlen(statStart)-1]) + 1;
    }

    return retStatus;
}

void SimpleBLE::debugPrint(const char *str)
{
    internalDebug(str);
    internalDebug("\r\n");
}
