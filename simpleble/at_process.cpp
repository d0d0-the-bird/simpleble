#include "at_process.h"
#include "timeout.h"

#include <stdint.h>
#include <string.h>


static int8_t substringInCharStream(char c,
                                    uint8_t numSubstrings,
                                    const char **substrings,
                                    uint8_t *foundLen
)
{
    int8_t foundSubstring = -1;

    for(uint32_t i = 0; i < numSubstrings; i++)
    {
        if( c == substrings[i][foundLen[i]] )
        {
            foundLen[i]++;
        }
        else
        {
            foundLen[i] = 0;
        }

        if( substrings[i][foundLen[i]] == '\0' )
        {
            foundSubstring = i;
            break;
        }
    }

    return foundSubstring;
}


void AtProcess::init(AtProcessInit *s)
{
    Timeout::init(pMillis);

    cmdEnding = s->cmdEnding;
    cmdAck = s->cmdAck;
    cmdError = s->cmdError;
}

uint32_t AtProcess::sendCommand(const char *cmd)
{
    uint32_t charsPrinted = print(cmd);
    charsPrinted += print(cmdEnding);

    return charsPrinted;
}

uint32_t AtProcess::waitURC(const char *urc, char *lineBuff, uint32_t lineBuffSize, uint32_t timeout)
{
    uint32_t recvLen = 0;

    struct URCContext
    {
        const char *urc;
        uint8_t urcFoundLen;
        int8_t foundUrc;
    } context = { urc,
                  0,
                  -1
    };

    CharHandler *responseChecker = [](char c, void* handlerContext)
    {
        URCContext *pContext = (URCContext*)handlerContext;

        // Check if we have already found one of substrings.
        if( pContext->foundUrc < 0 )
        {
            pContext->foundUrc =
                substringInCharStream(c,
                                      1,
                                      &pContext->urc,
                                      &pContext->urcFoundLen);
        }
    };

    do
    {
        recvLen = getLine(lineBuff, lineBuffSize-1, timeout, responseChecker, &context);

        if( context.foundUrc == 0 )
        {
            break;
        }

    }while(recvLen);

    return recvLen;
}

uint32_t AtProcess::getLine(
    char *line,
    uint32_t maxLineLen,
    uint32_t timeout,
    CharHandler *cHandler, void *handlerContext
)
{
    uint32_t lineLen = 0;
    bool timeoutExpired = false;

    Timeout respTimeout(timeout);
    if(!line)
    {
        maxLineLen = MAX_LINE_LEN_B;
    }

    do
    {
        char lastChar = '\0';

        if( read(&lastChar) )
        {
            if( line && lineLen < maxLineLen )
            {
                line[lineLen++] = lastChar;
            }
            else
            {
                lineLen++;
            }

            char lastCharOutput[2] = {lastChar, '\0'};
            output(lastCharOutput);

            if( cHandler )
            {
                cHandler(lastChar, handlerContext);
            }

            if( lastChar == '\n' )
            {
                break;
            }

            respTimeout.restart();
        }
        else
        {
            delay(5);
        }

        timeoutExpired = respTimeout.expired();
    }while(!timeoutExpired);

    if( line )
    {
        if( lineLen < maxLineLen )
        {
            line[lineLen] = '\0';
        }
        else
        {
           line[maxLineLen - 1] = '\0';
        }
    }

    return timeoutExpired ? 0 : lineLen ;
}


AtProcess::Status AtProcess::recvResponse(
    uint32_t timeout,
    char *responseBuff, uint32_t responseBuffSize,
    CharHandler *cHandler, void *handlerContext
)
{
    const uint8_t numSubstrings = 2;
    AtProcess::Status retval = TIMEOUT;
    uint32_t responseLen = 0;

    struct ResponseContext
    {
        const char *substrings[numSubstrings];
        CharHandler *higherHandler;
        void *higherContext;
        uint8_t substringFoundLen[numSubstrings];
        int8_t foundSubstring;
    } context = { {cmdAck ? cmdAck : "", cmdError ? cmdError : ""},
                  cHandler, handlerContext,
                  {0, 0},
                  -1
    };

    CharHandler *responseChecker = [](char c, void* handlerContext)
    {
        ResponseContext *pContext = (ResponseContext*)handlerContext;

        const uint8_t numSubstrings = sizeof(pContext->substrings)/sizeof(char*);

        // Check if we have already found one of substrings.
        if( pContext->foundSubstring < 0 )
        {
            pContext->foundSubstring =
                substringInCharStream(c,
                                      numSubstrings,
                                      pContext->substrings,
                                      pContext->substringFoundLen);
        }

        if( pContext->higherHandler )
        {
            pContext->higherHandler(c, pContext->higherContext);
        }
    };

    uint32_t recvLineLen;
    do{
        recvLineLen =
            getLine(responseBuff ? &responseBuff[responseLen] : NULL,
                    responseBuffSize - responseLen,
                    timeout,
                    responseChecker, &context);

    
        responseLen += recvLineLen;

        if( context.foundSubstring == 0 )
        {
            retval = AtProcess::SUCCESS;
            break;
        }

        if( context.foundSubstring == 1 )
        {
            retval = AtProcess::GEN_ERROR;
            break;
        }

    }while(recvLineLen);

    if( responseBuff )
    {
        if( responseLen < responseBuffSize )
        {
            responseBuff[responseLen] = '\0';
        }
        else
        {
            responseBuff[--responseLen] = '\0';
        }
    }

    return retval;
}

AtProcess::Status AtProcess::recvResponseWaitOk(
    uint32_t timeout,
    char *responseBuff, uint32_t responseBuffSize,
    CharHandler *cHandler, void *handlerContext
)
{
    Status err = recvResponse(timeout, responseBuff, responseBuffSize, cHandler, handlerContext);
    
    
    if( err == GEN_ERROR )
    {
        char *additionalResp = NULL;

        if( responseBuff )
        {
            uint32_t responseLen = strlen(responseBuff);

            additionalResp = &responseBuff[responseLen];

            responseBuffSize -= responseLen;
        }

        if( recvResponse(timeout, additionalResp, responseBuffSize, cHandler, handlerContext) == TIMEOUT )
        {
            err = TIMEOUT;
        }
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

    for(printed = 0; str[printed] && serPut(str[printed]); printed++);

    return printed;
}

uint32_t AtProcess::write(uint8_t data)
{
    return write(&data, 1);
}
uint32_t AtProcess::write(uint8_t *data, uint32_t dataLen)
{
    uint32_t printed;

    for(printed = 0; printed < dataLen && serPut(data[printed]); printed++);

    return printed;
}
uint32_t AtProcess::readBytes(uint8_t *buff, uint32_t readAmount)
{
    uint32_t readed;

    for(readed = 0; readed < readAmount && serGet((char*)&buff[readed]); readed++);

    return readed;
}
uint32_t AtProcess::readBytesBlocking(uint8_t *buff, uint32_t readAmount)
{
    uint32_t readed;

    for(readed = 0; readed < readAmount; readed += serGet((char*)&buff[readed]) ? 1 : 0);

    return readed;
}
bool AtProcess::read(char *c)
{
    return serGet(c);
}
