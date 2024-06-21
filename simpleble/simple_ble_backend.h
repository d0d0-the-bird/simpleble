#ifndef __SIMPLE_BLE_BACKEND_H__
#define __SIMPLE_BLE_BACKEND_H__

#include "at_process.h"

#include <stdint.h>


/**
 * @brief Simple BLE interface structure
 * 
 * @param rxEnabledSet Function pointer to a function that sets and clears
 *                     RX enabled pin.
 * @param moduleResetSet Function pointer to a function that sets and clears
 *                       module reset pin.
 * @param serialPut Function pointer to a function that puts one char to
 *                  serial interface.
 * @param serialGet Function pointer to a function that receives one character
 *                  from serial interface.
 * @param millis Function pointer to a function that gets total
 *               elapsed milliseconds from start of the program.
 * @param delayMs Function pointer to a function that delays further execution
 *                by specified number of milliseconds.
 * @param debugPrint Optional Function pointer to a function that prints
 *                   various debug information to desired output.
 */
struct SimpleBLEInterface
{
    void (*const rxEnabledSet)(bool);
    void (*const moduleResetSet)(bool);
    bool (*const serialPut)(char);
    bool (*const serialGet)(char*);
    uint32_t (*const millis)(void);
    void (*const delayMs)(uint32_t);
    void (*const debugPrint)(const char*);
};



class SimpleBLEBackend
{
public:
    static const int8_t INVALID_SERVICE_INDEX = -1;

    enum AdvType
    {
        INVALID_TYPE = 0x00,
        FLAGS = 0x01,
        N16BIT_SERVICE_UUID_MORE_AVAILABLE,
        N16BIT_SERVICE_UUID_COMPLETE,
        N32BIT_SERVICE_UUID_MORE_AVAILABLE,
        N32BIT_SERVICE_UUID_COMPLETE,
        N128BIT_SERVICE_UUID_MORE_AVAILABLE,
        N128BIT_SERVICE_UUID_COMPLETE,
        SHORT_LOCAL_NAME,
        COMPLETE_LOCAL_NAME,
        TX_POWER_LEVEL,
        CLASS_OF_DEVICE = 0x0D,
        SIMPLE_PAIRING_HASH_C,
        SIMPLE_PAIRING_RANDOMIZER_R,
        SECURITY_MANAGER_TK_VALUE,
        SECURITY_MANAGER_OOB_FLAGS,
        SLAVE_CONNECTION_INTERVAL_RANGE,
        SOLICITED_SERVICE_UUIDS_16BIT = 0x14,
        SOLICITED_SERVICE_UUIDS_128BIT,
        SERVICE_DATA,
        PUBLIC_TARGET_ADDRESS,
        RANDOM_TARGET_ADDRESS,
        APPEARANCE,
        ADVERTISING_INTERVAL,
        LE_BLUETOOTH_DEVICE_ADDRESS,
        LE_ROLE,
        SIMPLE_PAIRING_HASH_C256,
        SIMPLE_PAIRING_RANDOMIZER_R256,
        SERVICE_DATA_32BIT_UUID = 0x20,
        SERVICE_DATA_128BIT_UUID,
        LESC_CONFIRMATION_VALUE,
        LESC_RANDOM_VALUE,
        URI,
        N3D_INFORMATION_DATA = 0x3D,
        MANUFACTURER_SPECIFIC_DATA = 0xFF
    };

    enum CharPropFlags
    {
        NONE = 0x00,
        BROADCAST = 0x01,
        READ = 0x02,
        WRITE_WITHOUT_RESPONSE = 0x04,
        WRITE = 0x08,
        NOTIFY = 0x10,
        INDICATE = 0x20,
        AUTHENTICATED_SIGNED_WRITES = 0x40,
        EXTENDED_PROPERTIES = 0x80,
        READ_AND_NOTIFY = 0x12
    };

    enum TxPower
    {
        POW_N40DBM = -40,
        POW_N20DBM = -20,
        POW_N16DBM = -16,
        POW_N12DBM = -12,
        POW_N8DBM = -8,
        POW_N4DBM = -4,
        POW_0DBM = 0,
        POW_2DBM = 2,
        POW_3DBM = 3,
        POW_4DBM = 4
    };

    /**
     * @brief Construct a new Simple BLE object
     * 
     * @param ifc Complete SimpleBLE interface, with all external dependancies.
     */
    SimpleBLEBackend(const SimpleBLEInterface *ifc);

    /**
     * @brief Activate module serial reception of data.
     * 
     */
    void activateModuleRx(void);
    /**
     * @brief Deactivate module serial reception of data to save power.
     * 
     */
    void deactivateModuleRx(void);
    /**
     * @brief Reset the module via reset pin. Do this only if software reset
     *        doesn't work.
     * 
     */
    void hardResetModule(void);

    /**
     * @brief Initialise pins to initial values and put module to known state.
     * 
     */
    void begin();

    /**
     * @brief Send a command that doesn't need to receive any data.
     * 
     * @param cmd Command string that you want to send.
     * @param timeout How long to wait for module response in milliseconds.
     * @param response Optional buffer to store module response, make sure it is
     *                 of sufficient size!
     * @return AtProcess::Status Returns SUCCESS if response was received and no
     *                           error was reported by module.
     *                           Returns TIMEOUT if no command was received in
     *                           specified time.
     *                           Returns GEN_ERROR if module reported and error.
     */
    AtProcess::Status sendReceiveCmd(const char *cmd, uint32_t timeout = 3000, char *response = NULL);
    /**
     * @brief Send a command that has to read data from serial.
     * 
     * @param cmd Command string that you want to send.
     * @param buff Data buffer in which to store received data.
     * @param buffSize Data buffer size.
     * @param timeout How long to wait for module response in milliseconds.
     * @return AtProcess::Status Returns SUCCESS if response was received and no
     *                           error was reported by module.
     *                           Returns TIMEOUT if no command was received in
     *                           specified time.
     *                           Returns GEN_ERROR if module reported and error.
     */
    AtProcess::Status sendReadReceiveCmd(const char *cmd,
                                         uint8_t *buff,
                                         uint32_t buffSize,
                                         uint32_t timeout = 3000);
    /**
     * @brief Send a command that also has to write data to serial.
     * 
     * @param cmd Command string that you want to send.
     * @param data Data buffer which contains data for Simple BLE module.
     * @param dataSize Length of data in data buffer.
     * @param timeout How long to wait for module response in milliseconds.
     * @return AtProcess::Status Returns SUCCESS if response was received and no
     *                           error was reported by module.
     *                           Returns TIMEOUT if no command was received in
     *                           specified time.
     *                           Returns GEN_ERROR if module reported and error.
     */
    AtProcess::Status sendWriteReceiveCmd(const char *cmd,
                                          uint8_t *data,
                                          uint32_t dataSize,
                                          uint32_t timeout = 3000);
    /**
     * @brief Command that joins all of the above sendReceiveCmd into one function.
     * 
     * @param cmd Command string that you want to send.
     * @param buff Read buffer or write data depending on readNWrite parameter. Set
     *             to NULL if neither is needed.
     * @param size Size of the buffer or data length, depending on readNWrite parameter.
     * @param readNWrite Boolean which tells if data should be read or written.
     * @param timeout How long to wait for module response in milliseconds.
     * @param response Optional buffer to store module response, make sure it is
     *                 of sufficient size!
     * @return AtProcess::Status Returns SUCCESS if response was received and no
     *                           error was reported by module.
     *                           Returns TIMEOUT if no command was received in
     *                           specified time.
     *                           Returns GEN_ERROR if module reported and error.
     */
    AtProcess::Status sendReceiveCmd(const char *cmd,
                                    uint8_t *buff,
                                    uint32_t size,
                                    bool readNWrite,
                                    uint32_t timeout = 3000,
                                    char *response = NULL);

    /**
     * @brief Restart Simple BLE module via builtin command.
     * 
     * @return true If module successfuly restarted.
     * @return false If and error occured during module restart.
     */
    bool softRestart(void);

    /**
     * @brief Start advertising with previously constructed payload with setAdvPayload
     *        function.
     * 
     * @param advPeriod Period between advertisements in milliseconds. How often
     *                  to advertise.
     * @param advDuration Advertisement duration. How long to advertise after first
     *                    packet. If you want infinite advertisement set this to
     *                    SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION .
     * @param restartOnDisc Should advertisement restart if client disconnects.
     * @return true If advertisement started.
     * @return false If advertisement failed to start.
     */
    bool startAdvertisement(uint32_t advPeriod,
                            int32_t advDuration,
                            bool restartOnDisc);

    /**
     * @brief Stop advertising.
     * 
     * @return true If advertising successfuly stoped.
     * @return false If failed to stop advertisement.
     */
    bool stopAdvertisement(void);

    /**
     * @brief Set the advertisement payload section. Advertisement payload is
     *        composed of multiple sections differentiated by type. In order to
     *        set multiple sections call this function for each one of them.
     * 
     * @param type One of the AdvType values. More decsription on payload types
     *             can be found on official Bluetooth site.
     * @param data Data to be set under the desired type.
     * @param dataLen Data length.
     * @return true If new data was added to the payload.
     * @return false If an error occured during data adding.
     */
    bool setAdvPayload(AdvType type, uint8_t *data, uint32_t dataLen);

    /**
     * @brief Set the BLE transmission power.
     * 
     * @param dbm Transmission power in dBm.
     * @return true If transmission power was successfuly set.
     * @return false If an error occured during transmission power adjustment.
     */
    bool setTxPower(TxPower dbm);

    /**
     * @brief Add new service to Simple BLE module. If we compare BLE to a filesystem,
     *        services are like folders.
     * 
     * @param servUuid Unique service ID.
     * @return int8_t If successful returns positive service index, if an error
     *                occured returns negative number.
     */
    int8_t addService(uint8_t servUuid);

    /**
     * @brief Add new characteristic to Simple BLE module under desired service.
     *        If we compare BLE to a filesystem then characteristics are like files.
     * 
     * @param serviceIndex Index of a service under which to add new characteristic.
     * @param maxSize Maximum size of a characteristic.
     * @param flags Characteristic flag properties. Arithmetically OR values from
     *              CharPropFlags enum to get your desired combination of properties.
     *              Currently tested indicate, notify, read and write.
     * @return int8_t If successful returns characteristic index by which it can
     *                be referenced. This index is not unique since multiple characteristics
     *                from different services can have same indexes. If this function
     *                fails it returns negative number.
     */
    int8_t addChar(uint8_t serviceIndex, uint32_t maxSize, CharPropFlags flags);

    /**
     * @brief Check if characteristic has any new unread data.
     * 
     * @param serviceIndex Service under which is your desired characteristic.
     * @param charIndex Desired characteristic index.
     * @return int32_t Positive number if characteristic has some new data to read.
     *                 Negative if characteristic has no new data to read.
     */
    int32_t checkChar(uint8_t serviceIndex, uint8_t charIndex);

    /**
     * @brief Read data from characteristic.
     * 
     * @param serviceIndex Service under which is your desired characteristic.
     * @param charIndex Desired characteristic index.
     * @param buff Buffer in which to save characteristic data.
     * @param buffSize Buffer size.
     * @return int32_t Positive number if characteristic has some new data to read.
     *                 Negative if characteristic has no new data to read.
     */
    int32_t readChar(uint8_t serviceIndex, uint8_t charIndex,
                      uint8_t *buff, uint32_t buffSize);

    /**
     * @brief Write data to a characteristic.
     * 
     * @param serviceIndex Service under which is your desired characteristic.
     * @param charIndex Desired characteristic index.
     * @param data Buffer with data that should be transfered to desired characteristic.
     * @param dataSize Data length in buffer.
     * @return true If data was successfuly sent to Simple BLE module.
     * @return false If data transmission to module was unsuccessful.
     */
    bool writeChar(uint8_t serviceIndex, uint8_t charIndex,
                   uint8_t *data, uint32_t dataSize);

    const SimpleBLEInterface *ifc;

    AtProcess at;

private:

    inline void internalDebug(const char *dbgPrint)
    {
        if( ifc->debugPrint )
        {
            ifc->debugPrint(dbgPrint);
        }
    }

    const char *findCmdReturnStatus(const char *cmdRet, const char *statStart);
    void debugPrint(const char *str);

};


#endif//__SIMPLE_BLE_BACKEND_H__
