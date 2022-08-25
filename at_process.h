#ifndef __AT_PROCESS_H__
#define __AT_PROCESS_H__

#include <stdio.h>
#include <stdint.h>


#define MAX_LINE_LEN_B                      (80+1)


typedef void (CharHandler)(char, void*);


struct AtProcessInit
{
    const char *cmdEnding;
    const char *cmdAck;
    const char *cmdError;
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
    AtProcess(bool (*pSerPut)(char),
              bool (*pSerGet)(char*),
              void (*pDelay)(uint32_t),
              uint32_t (*pMillis)(void),
              void (*pOutput)(const char *) = NULL) :
                                                    pSerPut(pSerPut),
                                                    pSerGet(pSerGet),
                                                    pDelay(pDelay),
                                                    pMillis(pMillis),
                                                    pOutput(pOutput)
    {}

    /**
     * @brief Initialize AT processor to a known state.
     */
    void init(AtProcessInit *s);

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
    uint32_t waitURC(const char *urc, char *lineBuff, uint32_t lineBuffSize, uint32_t timeout = 1000);

    /**
     * @brief Get one line from communication interface. If timeout occurs before
     *        we get the full line, function returns 0. '\n' is used as a line
     *        termination character.
     * 
     * @param line Pointer to a line buffer where the line will be saved. It can
     *             be NULL if line is not needed.
     * @param maxLineLen Maximum line length that a buffer can store including a
     *                   terminating '\0' character.
     * @param timeout Maximum time that function should wait for a line before
     *                returning.
     * @param cHandler Character handler function that can be passed by the caller.
     *                 If this parameter is not NULL it will be called on each
     *                 received character, with the character passed to it along
     *                 with the @ref handlerContext .
     * @param handlerContext Context pointer that is passed to @ref cHandler .
     * @return uint32_t Returns the number of characters received if no line buffer
     *                  is given, and number of characters written to the buffer
     *                  if buffer was passed.
     */
    uint32_t getLine(
        char *line,
        uint32_t maxLineLen,
        uint32_t timeout,
        CharHandler *cHandler = NULL, void *handlerContext = NULL
    );

    /**
     * @brief AT response parser. It can detect the end of response as well as
     *        its status. It returns with timeout status if timeout expires
     *        before valid response is received.
     * 
     * @param timeout Timeout in milliseconds to wait for response.
     * @param responseBuff Buffer to save the response in. It can be NULL if not
     *                     used.
     * @param responseBuffSize Size if the response buffer if it is used or don't
     *                         care if not.
     * @param cHandler Character handler function that can be passed by the caller.
     *                 If this parameter is not NULL it will be called on each
     *                 received character, with the character passed to it along
     *                 with the @ref handlerContext .
     * @param handlerContext Context pointer that is passed to @ref cHandler .
     * @return SUCCESS Requested response type was received and in the case of 
     *                 whole response type it was received with OK.
     * @return GEN_ERROR Some ERROR was received as a response.
     * @return TIMEOUT Set timeout was reached before response was detected.
     */
    Status recvResponse(
        uint32_t timeout = 3000,
        char *responseBuff = NULL, uint32_t responseBuffSize = 0,
        CharHandler *cHandler = NULL, void *handlerContext = NULL
    );

    Status recvResponseWaitOk(
        uint32_t timeout = 3000,
        char *responseBuff = NULL, uint32_t responseBuffSize = 0,
        CharHandler *cHandler = NULL, void *handlerContext = NULL
    );

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

private:
    const char *cmdEnding;
    const char *cmdAck;
    const char *cmdError;

    bool (*pSerPut)(char);
    bool (*pSerGet)(char*);
    void (*pDelay)(uint32_t);
    uint32_t (*pMillis)(void);
    void (*pOutput)(const char *);

    inline bool serPut(char c) { if( pSerPut ) return pSerPut(c); else return false; }
    inline bool serGet(char *c) { if( pSerGet ) return pSerGet(c); else return false; }
    inline void delay(uint32_t ms) { if( pDelay ) pDelay(ms); }
    inline uint32_t millis(void) { if( pMillis ) return pMillis(); else return 0; }
    inline void output(const char *str) { if( pOutput ) pOutput(str); }
};


#endif//__AT_PROCESS_H__
