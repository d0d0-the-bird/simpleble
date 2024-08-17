#include "simple_ble_backend.h"
#include "at_process.h"
#include "timeout.h"

#include <string.h>


#define MODULE_RX_BLOCK_SIZE_B                                  (6)

static const char cmdEnding[] = "\r";
static const char cmdAck[] = "\nOK\r\n";
static const char cmdError[] = "ERROR\r\n";


SimpleBLEBackend::SimpleBLEBackend(const SimpleBLEBackendInterface *ifc) :
    ifc(ifc),
    at(ifc->serialPut, ifc->serialGet, ifc->delayMs, ifc->millis, NULL)
{
    Timeout::init(ifc->millis);
    unprocessedUrc[0] = '\0';
}

void SimpleBLEBackend::activateModuleRx(void)
{
    ifc->rxEnabledSet(true);
    ifc->delayMs(15);
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

    // Check if there are some unprocessed URCs before we execute a new command
    {
        char lineBuff[80+1];
        uint32_t lineLen = 0;
        do
        {
            lineLen = at.getLine(lineBuff, sizeof(lineBuff)-1, 5);

            if( lineBuff[0] >= ' ' )
            {
                strncpy(unprocessedUrc, lineBuff, sizeof(unprocessedUrc));
                unprocessedUrc[sizeof(unprocessedUrc)-1] = '\0';
            }

        }while(lineLen);
    }

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
            char lineBuff[80+1];
            uint32_t lineLen = 0;

            // Protect for later string operations.
            lineBuff[0] = '\0';
            lineBuff[sizeof(lineBuff)-1] = '\0';

            do
            {
                lineLen = at.getLine(lineBuff, sizeof(lineBuff)-1, 1000);
                internalDebug(lineBuff);

            }while(lineLen && strncmp(cmd, lineBuff, strlen(cmd)) != 0);

            if( lineLen )
            {
                // Read one line because it is still not the data.
                lineLen = at.getLine(lineBuff, sizeof(lineBuff)-1, 1000);
                internalDebug(lineBuff);

                at.readBytesBlocking(buff, size);
            }
        }

        cmdStatus = at.recvResponseWaitOk(timeout, response, 100);
    }

    return cmdStatus;
}


bool SimpleBLEBackend::softRestart(void)
{
    bool retval = true;

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

    return retval;
}

bool SimpleBLEBackend::startAdvertisement(uint32_t advPeriod,
                                   int32_t advDuration,
                                   bool restartOnDisc)
{
    bool retval = true;

    char cmdStr[40]; cmdStr[0] = '\0';
    strcat(cmdStr, "AT+ADVSTART=");
    char helpStr[20];

    utilityItoa(advPeriod, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);
    strcat(cmdStr, ",");
    utilityItoa(advDuration, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);
    strcat(cmdStr, ",");
    utilityItoa(restartOnDisc ? 1 : 0, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);

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

    char cmdStr[40]; cmdStr[0] = '\0';
    strcat(cmdStr, "AT+ADVPAYLOAD=");
    char helpStr[20];

    utilityItoa(type, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);
    strcat(cmdStr, ",");
    utilityItoa(dataLen, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);

    if( sendWriteReceiveCmd(cmdStr, data, dataLen) != AtProcess::SUCCESS )
    {
        retval = false;
    }

    return retval;
}

bool SimpleBLEBackend::setTxPower(TxPower dbm)
{
    bool retval = true;

    char cmdStr[30]; cmdStr[0] = '\0';
    strcat(cmdStr, "AT+TXPOWER=");
    char helpStr[20];

    utilityItoa(dbm, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);

    if( sendReceiveCmd(cmdStr) != AtProcess::SUCCESS )
    {
        retval = false;
    }

    return retval;
}

int8_t SimpleBLEBackend::addService(uint8_t servUuid)
{
    int8_t srvIndex = INVALID_SERVICE_INDEX;

    char cmdStr[50]; cmdStr[0] = '\0';
    strcat(cmdStr, "AT+ADDSRV=");
    char helpStr[20];

    char response[100];

    utilityItoa(servUuid, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);

    if( sendReceiveCmd(cmdStr, 3000, response) == AtProcess::SUCCESS )
    {
        const char *retStatus = findCmdReturnStatus(response, "^ADDSRV:");

        if( retStatus )
        {
            srvIndex = utilityAtoi(retStatus);
        }
    }
    
    return srvIndex;
}

int8_t SimpleBLEBackend::addChar(uint8_t serviceIndex, uint32_t maxSize, CharPropFlags flags)
{
    int8_t charIndex = -1;

    char cmdStr[50]; cmdStr[0] = '\0';
    strcat(cmdStr, "AT+ADDCHAR=");
    char helpStr[20];

    char response[50];

    utilityItoa(serviceIndex, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);
    strcat(cmdStr, ",");
    utilityItoa(maxSize, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);
    strcat(cmdStr, ",");
    utilityItoa(flags, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);

    if( sendReceiveCmd(cmdStr, 3000, response) == AtProcess::SUCCESS )
    {
        const char *retStatus = findCmdReturnStatus(response, "^ADDCHAR:");

        if( retStatus )
        {
            charIndex = utilityAtoi(retStatus);
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
    char cmdStr[50]; cmdStr[0] = '\0';
    strcat(cmdStr, "AT+READCHAR=");
    char helpStr[20];

    int32_t readBytes = buffSize;

    bool returnData = buff ? true : false ;

    utilityItoa(serviceIndex, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);
    strcat(cmdStr, ",");
    utilityItoa(charIndex, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);
    strcat(cmdStr, ",");
    utilityItoa(returnData, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);


    if( returnData )
    {
        if( sendReadReceiveCmd(cmdStr, buff, buffSize) == AtProcess::SUCCESS )
        {
        }
    }
    else
    {
        char response[100];

        if( sendReceiveCmd(cmdStr, 3000, response) == AtProcess::SUCCESS )
        {
            const char *retStatus = findCmdReturnStatus(response, "^READCHAR:");

            if( retStatus )
            {
                readBytes = utilityAtoi(retStatus);

                bool newData = utilityAtoi(strpbrk(retStatus, ",")+1);

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
                                 const uint8_t *data, uint32_t dataSize)
{
    bool retval = false;

    char cmdStr[50]; cmdStr[0] = '\0';
    strcat(cmdStr, "AT+WRITECHAR=");
    char helpStr[20];

    utilityItoa(serviceIndex, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);
    strcat(cmdStr, ",");
    utilityItoa(charIndex, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);
    strcat(cmdStr, ",");
    utilityItoa(dataSize, helpStr, sizeof(helpStr));
    strcat(cmdStr, helpStr);


    if( sendWriteReceiveCmd(cmdStr, (uint8_t*)data, dataSize) == AtProcess::SUCCESS )
    {
        retval = true;
    }

    return retval;
}

bool SimpleBLEBackend::waitCharUpdate(uint8_t* serviceIndex, uint8_t* charIndex,
                                  uint32_t* dataSize, uint32_t timeout)
{
    const char charwriteUrc[] = "^CHARWRITE";

    bool retval = false;

    char urcBuff[40];

do{
    if( unprocessedUrc[0] && strncmp(unprocessedUrc, charwriteUrc, strlen(charwriteUrc)) == 0 )
    {
        strncpy(urcBuff, unprocessedUrc, sizeof(urcBuff));
        unprocessedUrc[0] = '\0';
    }
    else if( at.waitURC(charwriteUrc, urcBuff, sizeof(urcBuff), timeout) == 0 )
    {
        break;
    }

    char *urcStart = urcBuff; while(*urcStart != '^') urcStart++;
    char *infoParse = &urcStart[12];

    *serviceIndex = utilityAtoi(infoParse);
    infoParse = strpbrk(infoParse, ",") + 1;
    *charIndex = utilityAtoi(infoParse);
    infoParse = strpbrk(infoParse, ",") + 1;
    *dataSize = utilityAtoi(infoParse);

    retval = true;

}while(0);

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

uint32_t SimpleBLEBackend::utilityItoa(int32_t value, char *strBuff, uint32_t strBuffSize)
{
    const uint32_t base = 10;
    // Store information about sign.
    bool isPositive = value >= 0 ? true : false;
    uint32_t writtenDigits = 0;

    // We want to work with positive number.
    if( !isPositive )
    {
        value *= -1;
    }

    // To store all digits of a number we will use this union.
    union
    {
        uint32_t raw[2];
        struct
        {
            uint8_t downer : 4 ;
            uint8_t upper : 4 ;
        } bytePair[8];
    } digits;

    memset(digits.raw, 0x00, sizeof(digits.raw));

    // Extract digits from a number.
    uint32_t numDigits = value == 0 ? 1 : 0 ;
    for(; value && numDigits < sizeof(digits.bytePair)*2; numDigits++)
    {
        uint8_t digit = value%base;

        value /= base;

        if( numDigits%2 == 0 )
        {
            digits.bytePair[numDigits/2].downer = digit;
        }
        else
        {
            digits.bytePair[numDigits/2].upper = digit;
        }
    }

    // Add sign.
    if( !isPositive )
    {
        strBuff[0] = '-';
        // Increment pointer by one for convinience in for loop.
        strBuff++;
        writtenDigits++;
    }

    // Write digits to a string array.
    for(int i = 0; i < numDigits && writtenDigits < strBuffSize; i++, writtenDigits++)
    {
        uint32_t digitIterator = numDigits - i - 1;
        if( digitIterator%2 == 0 )
        {
            strBuff[i] = digits.bytePair[digitIterator/2].downer + '0';
        }
        else
        {
            strBuff[i] = digits.bytePair[digitIterator/2].upper + '0';
        }
    }
    // Now that we are done with for loop return buffer pointer to original value.
    isPositive ? 0 : strBuff--;

    // Finish string with a null terminator.
    if( writtenDigits < strBuffSize )
    {
        strBuff[writtenDigits] = '\0';
    }
    else
    {
        strBuff[0] = '\0';
        writtenDigits = 0;
    }

    return writtenDigits;
}

int32_t SimpleBLEBackend::utilityAtoi(const char* asciiInt)
{
    int32_t retval = 0;

    while( *asciiInt == ' ' ) asciiInt++;

    int32_t sign = 1;

    if( *asciiInt == '-' )
    {
        sign = -1;
        asciiInt++;
    }
    else if( *asciiInt == '+' )
    {
        asciiInt++;
    }

    while( *asciiInt >= '0' && *asciiInt <= '9' )
    {
        retval *= 10;
        retval += *asciiInt - '0';
        asciiInt++;
    }

    return sign*retval;
}
