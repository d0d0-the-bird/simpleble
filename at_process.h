#ifndef __AT_PROCESS_H__
#define __AT_PROCESS_H__

#include "timeout.h"

#include <stdio.h>
#include <stdint.h>


#define MAX_LINE_LEN_B                      (80+1)



struct SerialUART
{
    bool (*serPut)(char);
    bool (*serGet)(char*);
};



/**
 * @brief Class for handling of AT style UART communication. It handles some of
 *        the most common tasks present in this style of communication like
 *        sending a command, receiving a response etc.
 */
class AtProcess
{
public:

    /**
     * @brief AT response status codes.
     */
    enum Status
    {
        SUCCESS,
        GEN_ERROR,
        TIMEOUT
    };

    /**
     * @brief Type of response that @ref receiveResponse should wait for.
     */
    enum RespType
    {
        WHOLE, /*!< Whole response, up to OK or ERROR. */
        URC    /*!< Unsolicated Response Code (wait for one line). */
    };

    /**
     * @brief Construct a new At Process object.
     * 
     * @param uart Reference to previously initialized SerialUART instance.
     * @param delay Pointer to the delay function to use.
     * @param debug Pointer to the debug printout function to use.
     */
    AtProcess(SerialUART *uart = NULL,
              void (*delay)(uint32_t) = NULL,
              void (*_output)(const char *) = NULL) :
                                                    uart(*uart),
                                                    delay(delay),
                                                    _output(_output)
    {}

    /**
     * @brief Initialize AT processor to a known state.
     */
    void init();

    /**
     * @brief Send an AT command. It automaticaly appends CR ('\r') at the end.
     * 
     * @param cmd AT command string to send without AT termination character.
     * @return uint32_t Number of characters successfuly sent.
     */
    uint32_t sendCommand(const char *cmd);

    /**
     * @brief Wait for a specific URC.
     * 
     * @param urc URC string to wait for. It can be only a part of expected URC
     *            for dynamic URCs.
     * @param timeout Timeout in milliseconds to wait for requested URC.
     * @return true URC was received.
     * @return false URC wasn't received and timeout was reached.
     */
    bool waitURC(const char *urc, uint32_t timeout = 1000);
    
    uint32_t getLine(char *line, uint32_t maxLineLen, uint32_t timeout);

    /**
     * @brief AT response parser. It can detect The end of response as well as
     *        its status. It can also wait for single line responses like URCs.
     * 
     * @param timeout Timeout in milliseconds to wait for response.
     * @param response Type of response to wait for.
     * @return SUCCESS Requested response type was received and in the case of 
     *                 whole response type it was received with OK.
     * @return GEN_ERROR Some ERROR was received as a response.
     * @return TIMEOUT Set timeout was reached before response was detected.
     */
    Status recvResponse(uint32_t timeout = 3000, char *responseBuff = NULL);
    
    Status recvResponseWaitOk(uint32_t timeout = 3000, char *responseBuff = NULL);

    /**
     * @brief Sends provided command and returns one of the status codes.
     * 
     * @param command AT command string to send.
     * @param timeout Time in milliseconds to wait for response.
     * @return Status one of @ref Status codes.
     */
    Status sendReceive(const char *command, uint32_t timeout = 3000, char *responseBuff = NULL);

    /**
     * @brief Print a null terminated string on output communication interface.
     * 
     * @param str String to print out.
     * @return uint32_t Number of characters successfuly printed.
     */
    uint32_t print(const char *str);

    /**
     * @brief Write a data byte to output communication interface.
     * 
     * @param data Data byte to write out.
     * @return 1 Byte was successfuly sent.
     * @return 0 Failed to send data byte.
     */
    uint32_t write(uint8_t data);

    /**
     * @brief Write arbitrary number of bytes to output communication interface.
     * 
     * @param data Pointer to data buffer.
     * @param dataLen Amount of data in data buffer.
     * @return uint32_t Amount of bytes successfuly sent out.
     */
    uint32_t write(uint8_t *data, uint32_t dataLen);

    /**
     * @brief Request arbitrary amount of data from input communication interface,
     *        without blocking.
     * 
     * @param buff Pointer to buffer where input data should be stored.
     * @param readAmount Amount of data requested.
     * @return uint32_t Amount of data actually received into buffer.
     */
    uint32_t readBytes(uint8_t *buff, uint32_t readAmount);
    
    /**
     * @brief Request arbitrary amount of data from input communication interface,
     *        and block until we receive it.
     * 
     * @param buff Pointer to buffer where input data should be stored.
     * @param readAmount Amount of data requested.
     * @return uint32_t Amount of data actually received into buffer.
     */
    uint32_t readBytesBlocking(uint8_t *buff, uint32_t readAmount);

    /**
     * @brief Read one character from input communication interface without
     *        blocking.
     * 
     * @param c Pointer to storage for received character.
     * @return true Character was available.
     * @return false Character wasn't available.
     */
    bool read(char *c);

    /**
     * @brief Get the last status buffer.
     * 
     * @return const char* Pointer to last status buffer. It always holds status
     *                     received for last AT command sent.
     */
    //const char *getLastStatus(void) { return lastStatus; }

private:
    SerialUART uart;

    void (*delay)(uint32_t);

    void (*_output)(const char *);

    void output(const char *str)
    {
        if( _output )
        {
            _output(str);
        }
    }

    //char lastStatus[64];
};


#endif//__AT_PROCESS_H__
