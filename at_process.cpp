#include "at_process.h"
#include "timeout.h"


#include <stdint.h>
#include <string.h>


void AtProcess::init()
{
}


uint32_t AtProcess::sendCommand(const char *cmd)
{
    uint32_t charsPrinted = print(cmd);
    charsPrinted += write('\r');

    return charsPrinted;
}

bool AtProcess::waitURC(const char *urc, uint32_t timeout)
{
    bool retval = false;

    size_t urcLen = strlen(urc);

    char lineBuff[MAX_LINE_LEN_B];

    // Protect for later string operations.
    lineBuff[sizeof(lineBuff)-1] = '\0';

    while(getLine(lineBuff, sizeof(lineBuff)-1, timeout))
    {
        // First condition ensures string is URC, second checks
        // if it is wanted URC.
        if(lineBuff[0] == '^' && strncmp(urc, lineBuff, urcLen) == 0 )
        {
            retval = true;
            break;
        }
    }

    return retval;
}

uint32_t AtProcess::getLine(char *line, uint32_t maxLineLen, uint32_t timeout)
{
    uint32_t lineLen = 0;

    Timeout respTimeout(timeout);

    while(lineLen < maxLineLen && respTimeout.notExpired())
    {
        char lastChar = '\0';
        bool readChar = false;

        if( read(&lastChar) )
        {
            line[lineLen++] = lastChar;
            readChar = true;
        }

        if( lastChar == '\n' )
        {
            break;
        }

        if( !readChar && delay )
        {
            delay(10);
        }
    }

    if( lineLen < maxLineLen )
    {
        line[lineLen] = '\0';
    }

    return lineLen;
}

AtProcess::Status AtProcess::recvResponse(uint32_t timeout, char *responseBuff)
{
    AtProcess::Status retval = TIMEOUT;

    char lineBuff[MAX_LINE_LEN_B];

    // Protect for later string operations.
    lineBuff[sizeof(lineBuff)-1] = '\0';

    if( responseBuff )
    {
        *responseBuff = '\0';
    }

    while(getLine(lineBuff, sizeof(lineBuff)-1, timeout))
    {
        if( responseBuff )
        {
            strcat(responseBuff, lineBuff);
        }

        output(lineBuff);

        if( strstr(lineBuff, "OK") )
        {
            retval = SUCCESS;
            break;
        }
        else if( strstr(lineBuff, "ERROR") )
        {
            retval = GEN_ERROR;
            break;
        }
    }

    return retval;
}

AtProcess::Status AtProcess::recvResponseWaitOk(uint32_t timeout, char *responseBuff)
{
    Status err = recvResponse(timeout, responseBuff);

    if( err == GEN_ERROR )
    {
        char *additionalResp = NULL;

        if( responseBuff )
        {
            additionalResp = &responseBuff[strlen(responseBuff)];
        }

        recvResponse(timeout, additionalResp);
    }

    return err;
}

AtProcess::Status AtProcess::sendReceive(const char *command, uint32_t timeout, char *responseBuff)
{
    sendCommand(command);
    return recvResponse(timeout, responseBuff);
}

uint32_t AtProcess::print(const char *str)
{
    uint32_t printed;

    for(printed = 0; str[printed] && uart.serPut(str[printed]); printed++);

    return printed;
}

uint32_t AtProcess::write(uint8_t data)
{
    return write(&data, 1);
}
uint32_t AtProcess::write(uint8_t *data, uint32_t dataLen)
{
    uint32_t printed;

    for(printed = 0; printed < dataLen && uart.serPut(data[printed]); printed++);

    return printed;
}
uint32_t AtProcess::readBytes(uint8_t *buff, uint32_t readAmount)
{
    uint32_t readed;

    for(readed = 0; readed < readAmount && uart.serGet((char*)&buff[readed]); readed++);

    return readed;
}
uint32_t AtProcess::readBytesBlocking(uint8_t *buff, uint32_t readAmount)
{
    uint32_t readed;

    for(readed = 0; readed < readAmount; readed += uart.serGet((char*)&buff[readed]) ? 1 : 0);

    return readed;
}
bool AtProcess::read(char *c)
{
    return uart.serGet(c);
}
