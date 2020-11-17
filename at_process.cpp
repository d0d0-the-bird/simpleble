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
    charsPrinted += print("\r");

    return charsPrinted;
}

bool AtProcess::waitURC(const char *urc, uint32_t timeout)
{
    bool retval = false;

    size_t urcLen = strlen(urc);

    while(recvResponse(timeout, WHOLE) != TIMEOUT)
    {
        const char *status = getLastStatus();

        // First condition ensures string is URC, second checks
        // if it is wanted URC.
        if(status[0] == '^' && strncmp(urc, status, urcLen) == 0 )
        {
            retval = true;
            break;
        }
    }

    return retval;
}

AtProcess::Status AtProcess::recvResponse(uint32_t timeout, RespType response)
{
    AtProcess::Status retval = TIMEOUT;

    char *status = lastStatus;

    *status = '\0';
    char *lastStatusChar = &lastStatus[sizeof(lastStatus)-1];

    Timeout respTimeout(timeout);

    while(respTimeout.notExpired())
    {
        char lastChar = '\0';

        while(read(&lastChar))
        {
            if( status != lastStatusChar && lastChar )
            {
                *status = lastChar;

                status++;
            }

            if( response == URC && lastChar == '\n' )
            {
                break;
            }
        }

        if( response == URC )
        {
            if( lastChar == '\n' )
            {
                retval = SUCCESS;
                break;
            }
        }
        else
        {
            if( strstr(lastStatus, "OK") )
            {
                retval = SUCCESS;
                break;
            }
            else if( strstr(lastStatus, "ERROR") )
            {
                retval = GEN_ERROR;
                break;
            }
        }
    }

    *status = '\0';

    if( response == WHOLE && retval == SUCCESS )
    {
        delay(10);
    }

    return retval;
}


const char *AtProcess::sendReceiveResponse(const char *command, uint32_t timeout)
{
    sendCommand(command);

    recvResponse(timeout);

    return getLastStatus();
}

AtProcess::Status AtProcess::sendReceive(const char *command, uint32_t timeout)
{
    sendCommand(command);
    return recvResponse(timeout);
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

