#ifndef __SIMPLE_BLE_H__
#define __SIMPLE_BLE_H__

#include "at_process.h"

#include <stdint.h>


typedef void (GenericGpioSetter)(bool);
typedef bool (SerialPut)(char);
typedef bool (SerialGet)(char*);
typedef uint32_t (MillisCounter)(void);
typedef void (MillisecondDelay)(uint32_t);
typedef void (DebugPrint)(const char*);


class SimpleBLE
{
public:
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
        EXTENDED_PROPERTIES = 0x80
    };

    //SimpleBLE(int rxPin, int txPin, int rxEnablePin, int moduleResetPin = -1);
    SimpleBLE(GenericGpioSetter *rxEnabledSetter,
              GenericGpioSetter *moduleResetSetter,
              SerialPut *serialPutter,
              SerialGet *serialGetter,
              MillisCounter *millisCounterGetter,
              MillisecondDelay *delayer,
              DebugPrint *debugPrinter = NULL
    ) :
              rxEnabledSetter(rxEnabledSetter),
              moduleResetSetter(moduleResetSetter),
              at((SerialUART*)&(SerialUART){serialPutter, serialGetter}, delayer, debugPrinter),
              millisCounterGetter(millisCounterGetter),
              delayer(delayer),
              debugPrinter(debugPrinter)
    {
        Timeout::init(millisCounterGetter);

        /*
        SerialUART uart =
        {
            .serPut = serialPutter,
            .serGet = serialGetter
        };

        at = AtProcess(&uart, internalDelay);
        */
    }

    void activateModuleRx(void);
    void deactivateModuleRx(void);
    void hardResetModule(void);

    void begin(void);
    
    AtProcess::Status sendReceiveCmd(const char *cmd, uint32_t timeout = 3000, char *response = NULL);
    AtProcess::Status sendReadReceiveCmd(const char *cmd,
                                         uint8_t *buff,
                                         uint32_t buffSize,
                                         uint32_t timeout = 3000);
    AtProcess::Status sendWriteReceiveCmd(const char *cmd,
                                          uint8_t *data,
                                          uint32_t dataSize,
                                          uint32_t timeout = 3000);
    AtProcess::Status sendReceiveCmd(const char *cmd,
                                    uint8_t *buff,
                                    uint32_t size,
                                    bool readNWrite,
                                    uint32_t timeout = 3000,
                                    char *response = NULL);

    bool softRestart(void);
    bool startAdvertisement(uint32_t advPeriod,
                            int32_t advDuration,
                            bool restartOnDisc);
    bool stopAdvertisement(void);
    bool setAdvPayload(AdvType type, uint8_t *data, uint32_t dataLen);

    bool setTxPower(TxPower dbm);

    int addService(uint8_t servUuid);
    int addChar(uint8_t serviceIndex, uint32_t maxSize, CharPropFlags flags);

    int32_t checkChar(uint8_t serviceIndex, uint8_t charIndex);

    int32_t readChar(uint8_t serviceIndex, uint8_t charIndex,
                      uint8_t *buff, uint32_t buffSize);
    
    bool writeChar(uint8_t serviceIndex, uint8_t charIndex,
                   uint8_t *data, uint32_t dataSize);

private:

    inline void internalSetRxEnable(bool state)
    {
        rxEnabledSetter(state);
    }
    inline void internalSetModuleReset(bool state)
    {
        moduleResetSetter(state);
    }
    inline uint32_t internalMillis(void)
    {
        return millisCounterGetter();
    }
    inline void internalDelay(uint32_t ms)
    {
        delayer(ms);
    }
    inline void internalDebug(const char *dbgPrint)
    {
        if( debugPrinter )
        {
            debugPrinter(dbgPrint);
        }
    }

    const char *findCmdReturnStatus(const char *cmdRet, const char *statStart);
    void debugPrint(const char *str);

    GenericGpioSetter *rxEnabledSetter;
    GenericGpioSetter *moduleResetSetter;
    MillisCounter *millisCounterGetter;
    MillisecondDelay *delayer;
    DebugPrint *debugPrinter;

    AtProcess at;

};


#endif//__SIMPLE_BLE_H__
