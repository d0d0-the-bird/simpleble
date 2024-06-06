#include "simple_ble.h"
#include "at_process.h"
#include "timeout.h"

#include <stdlib.h>
#include <string.h>


#define MODULE_RX_BLOCK_SIZE_B                                  (6)

static const char cmdEnding[] = "\r";
static const char cmdAck[] = "\nOK\r\n";
static const char cmdError[] = "ERROR\r\n";


SimpleBLEBackend::SimpleBLEBackend(const SimpleBLEInterface *ifc) :
    ifc(ifc),
    at(ifc->serialPut, ifc->serialGet, ifc->delayMs, ifc->millis, NULL)
{
    Timeout::init(ifc->millis);
}

void SimpleBLEBackend::activateModuleRx(void)
{
    ifc->rxEnabledSet(true);
    ifc->delayMs(15;)
}
void SimpleBLEBackend::deactivateModuleRx(void)
{
    ifc->rxEnabledSet(false);
}
void SimpleBLEBackend::hardResetModule(void)
{
    ifc->moduleResetSet(false);
    ifc->delayMs(10);
    ifc->moduleResetSet(true);
}

void SimpleBLEBackend::begin()
{
    AtProcessInit s = {
      cmdEnding,
      cmdAck,
      cmdError,
    };
    at.init(&s);
    deactivateModuleRx();
    hardResetModule();
}

AtProcess::Status SimpleBLEBackend::sendReceiveCmd(const char *cmd,
                                            uint32_t timeout,
                                            char *response)
{
    return sendReceiveCmd(cmd, NULL, 0, false, timeout, response);
}

AtProcess::Status SimpleBLEBackend::sendReadReceiveCmd(const char *cmd,
                                                uint8_t *buff,
                                                uint32_t buffSize,
                                                uint32_t timeout)
{
    return sendReceiveCmd(cmd, buff, buffSize, true, timeout);
}

AtProcess::Status SimpleBLEBackend::sendWriteReceiveCmd(const char *cmd,
                                                 uint8_t *data,
                                                 uint32_t dataSize,
                                                 uint32_t timeout)
{
    return sendReceiveCmd(cmd, data, dataSize, false, timeout);
}


AtProcess::Status SimpleBLEBackend::sendReceiveCmd(const char *cmd,
                                            uint8_t *buff,
                                            uint32_t size,
                                            bool readNWrite,
                                            uint32_t timeout,
                                            char *response)
{
    AtProcess::Status cmdStatus = AtProcess::GEN_ERROR;

    if( response )
    {
        response[0] = '\0';
    }

    activateModuleRx();

    // We will get an echo of this command uninterrupted with URCs because we
    // send it quickly.
    uint32_t sent = at.sendCommand(cmd);

    if( !readNWrite && buff )
    {
        // We are writing.
        sent += at.write(buff, size);
    }
    if( sent > 0 )
    {
        // Now is the time to start checking for read data.
        if( readNWrite && buff )
        {
            // We are reading.
            //AtProcess::Status lineStatus;

            char lineBuff[80+1];
            uint32_t lineLen = 0;

            // Protect for later string operations.
            lineBuff[sizeof(lineBuff)-1] = '\0';

            do
            {
                //lineStatus = at.recvResponse(1000, AtProcess::URC);
                lineLen = at.getLine(lineBuff, sizeof(lineBuff)-1, 1000);
                internalDebug(lineBuff);

            }while(lineLen && strncmp(cmd, lineBuff, strlen(cmd)) != 0);

            if( lineLen )
            {
                // Read one line because it is still not the data.
                //lineStatus = at.recvResponse(1000, AtProcess::URC);
                lineLen = at.getLine(lineBuff, sizeof(lineBuff)-1, 1000);
                internalDebug(lineBuff);

                at.readBytesBlocking(buff, size);
            }
        }

        cmdStatus = at.recvResponseWaitOk(timeout, response, 100);
    }

    deactivateModuleRx();

    return cmdStatus;
}


bool SimpleBLEBackend::softRestart(void)
{
    bool retval = true;

    char cmdStr[20] = "AT+RESTART";

    if( sendReceiveCmd(cmdStr) != AtProcess::SUCCESS )
    {
        retval = false;
    }
    else
    {
        at.waitURC("^START", NULL, 0, 5000);
    }

    return retval;
}

bool SimpleBLEBackend::startAdvertisement(uint32_t advPeriod,
                                   int32_t advDuration,
                                   bool restartOnDisc)
{
    bool retval = true;

    char cmdStr[40] = "AT+ADVSTART=";
    char helpStr[20];

    strcat(cmdStr, itoa(advPeriod, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(advDuration, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(restartOnDisc ? 1 : 0, helpStr, 10));

    if( sendReceiveCmd(cmdStr) != AtProcess::SUCCESS )
    {
        retval = false;
    }

    return retval;
}

bool SimpleBLEBackend::stopAdvertisement(void)
{
    bool retval = true;

    const char* cmdStr = "AT+ADVSTOP";

    if( sendReceiveCmd(cmdStr) != AtProcess::SUCCESS )
    {
        retval = false;
    }

    return retval;
}

bool SimpleBLEBackend::setAdvPayload(AdvType type, uint8_t *data, uint32_t dataLen)
{
    bool retval = true;

    char cmdStr[40] = "AT+ADVPAYLOAD=";
    char helpStr[20];

    strcat(cmdStr, itoa(type, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(dataLen, helpStr, 10));

    if( sendWriteReceiveCmd(cmdStr, data, dataLen) != AtProcess::SUCCESS )
    {
        retval = false;
    }

    return retval;
}

bool SimpleBLEBackend::setTxPower(TxPower dbm)
{
    bool retval = true;

    char cmdStr[30] = "AT+TXPOWER=";
    char helpStr[20];

    strcat(cmdStr, itoa(dbm, helpStr, 10));

    if( sendReceiveCmd(cmdStr) != AtProcess::SUCCESS )
    {
        retval = false;
    }

    return retval;
}

int8_t SimpleBLEBackend::addService(uint8_t servUuid)
{
    int8_t srvIndex = -1;

    char cmdStr[50] = "AT+ADDSRV=";
    char helpStr[20];

    char response[100];

    strcat(cmdStr, itoa(servUuid, helpStr, 10));

    if( sendReceiveCmd(cmdStr, 3000, response) == AtProcess::SUCCESS )
    {
        const char *retStatus = findCmdReturnStatus(response, "^ADDSRV:");

        if( retStatus )
        {
            srvIndex = atoi(retStatus);
        }
    }
    
    return srvIndex;
}

int8_t SimpleBLEBackend::addChar(uint8_t serviceIndex, uint32_t maxSize, CharPropFlags flags)
{
    int8_t charIndex = -1;

    char cmdStr[50] = "AT+ADDCHAR=";
    char helpStr[20];

    char response[100];

    strcat(cmdStr, itoa(serviceIndex, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(maxSize, helpStr, 10));
    strcat(cmdStr, ",");
    strcat(cmdStr, itoa(flags, helpStr, 10));

    if( sendReceiveCmd(cmdStr, 3000, response) == AtProcess::SUCCESS )
    {
        const char *retStatus = findCmdReturnStatus(response, "^ADDCHAR:");

        if( retStatus )
        {
            charIndex = atoi(retStatus);
        }
    }

    return charIndex;
}

int32_t SimpleBLEBackend::checkChar(uint8_t serviceIndex, uint8_t charIndex)
{
    return readChar(serviceIndex, charIndex, NULL, 0);
}

int32_t SimpleBLEBackend::readChar(uint8_t serviceIndex, uint8_t charIndex,
                            uint8_t *buff, uint32_t buffSize)
{
    char cmdStr[50] = "AT+READCHAR=";
    char helpStr[20];

    char response[100];

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
        }
    }
    else
    {
        if( sendReceiveCmd(cmdStr, 3000, response) == AtProcess::SUCCESS )
        {
            const char *retStatus = findCmdReturnStatus(response, "^READCHAR:");

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

bool SimpleBLEBackend::writeChar(uint8_t serviceIndex, uint8_t charIndex,
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
        retval = true;
    }

    return retval;
}


const char *SimpleBLEBackend::findCmdReturnStatus(const char *cmdRet, const char *statStart)
{
    const char *retStatus = strstr(cmdRet, statStart);

    if( retStatus )
    {
        retStatus += strlen(statStart);
    }

    return retStatus;
}

void SimpleBLEBackend::debugPrint(const char *str)
{
    internalDebug(str);
    internalDebug("\r\n");
}
