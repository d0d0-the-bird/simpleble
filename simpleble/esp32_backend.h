#ifndef __ESP32_BACKEND_H__
#define __ESP32_BACKEND_H__

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <stdint.h>


#define CONST_CEIL(div1, div2)      (((div1) +                                  \
                                      !!((div1)%(div2))*(div2) -                \
                                      (div1)%(div2)                             \
                                     )                                          \
                                     /                                          \
                                     (div2)                                     \
                                    )


/**
 * @brief Simple BLE interface structure
 * 
 * @param millis Function pointer to a function that gets total
 *               elapsed milliseconds from start of the program.
 * @param delayMs Function pointer to a function that delays further execution
 *                by specified number of milliseconds.
 * @param debugPrint Optional Function pointer to a function that prints
 *                   various debug information to desired output.
 */
struct Esp32BackendInterface
{
    uint32_t (*const millis)(void);
    void (*const delayMs)(uint32_t);
    void (*const debugPrint)(const char*);
};



class Esp32Backend
{
public:
    static const int8_t INVALID_SERVICE_INDEX = -1;

    static const uint8_t MAX_NUM_SERVICES = 15;
    static const uint8_t MAX_NUM_CHARS = 10;

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
        POW_N9DBM = -9,
        POW_N8DBM = -8,
        POW_N6DBM = -6,
        POW_N4DBM = -4,
        POW_N3DBM = -3,
        POW_0DBM = 0,
        POW_2DBM = 2,
        POW_3DBM = 3,
        POW_4DBM = 4,
        POW_6DBM = 6,
        POW_9DBM = 9,
    };

    /**
     * @brief Construct a new Simple BLE object
     * 
     * @param ifc Complete SimpleBLE interface, with all external dependancies.
     */
    Esp32Backend(const Esp32BackendInterface *ifc);

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
                   const uint8_t *data, uint32_t dataSize);

    bool waitCharUpdate(uint8_t* serviceIndex, uint8_t* charIndex,
                        uint32_t* dataSize, uint32_t timeout=1000);

    const Esp32BackendInterface *ifc;

    bool restartAdvOnDisc;

    struct
    {
        BLEService* serv;
        uint8_t charNum;
    } services[MAX_NUM_SERVICES];
    uint8_t servNum;

    struct UpdatedDataFlags
    {
        UpdatedDataFlags() { memset(charFlags, 0x00, sizeof(charFlags)); }

        inline bool isValidIndex(uint32_t charIndex)
        {
            return charIndex < MAX_NUM_CHARS;
        }
        inline bool setFlag(uint32_t charIndex)
        {
            if( isValidIndex(charIndex) )
            {
                charFlags[charIndex/bitsPerByte] |= 1 << (charIndex%bitsPerByte);
            }

            return isValidIndex(charIndex);
        }
        inline bool rstFlag(uint32_t charIndex)
        {
            if( isValidIndex(charIndex) )
            {
                charFlags[charIndex/bitsPerByte] &= ~(1 << (charIndex%bitsPerByte));
            }

            return isValidIndex(charIndex);
        }
        inline bool getFlag(uint32_t charIndex)
        {
            if( isValidIndex(charIndex) )
            {
                return charFlags[charIndex/bitsPerByte] & 1 << (charIndex%bitsPerByte);
            }
            else
            {
                return false;
            }
        }

        static const int bitsPerByte = 8;

        uint8_t charFlags[CONST_CEIL(MAX_NUM_CHARS, bitsPerByte)];
    };

    UpdatedDataFlags receivedData[sizeof(services)/sizeof(services[0])];
    UpdatedDataFlags readData[sizeof(services)/sizeof(services[0])];

private:

    BLEServer* pServer;

    inline void internalDebug(const char *dbgPrint)
    {
        if( ifc->debugPrint )
        {
            ifc->debugPrint(dbgPrint);
        }
    }

    BLEUUID charUuidFromIndex(uint8_t servIndex, uint8_t charIndex);
    BLEUUID charUuidFromIndex(BLEUUID servUuid, uint8_t charIndex);

    BLECharacteristic* getCharacteristic(uint8_t servIndex, uint8_t charIndex);

    int32_t utilityAtoi(const char* asciiInt);

    void debugPrint(const char *str);

};


#endif//__ESP32_BACKEND_H__
