#ifndef __SIMPLE_BLE_H__
#define __SIMPLE_BLE_H__

#ifdef ARDUINO
#include "Arduino.h"
#include "AltSoftSerial.h"
#endif //ARDUINO

#include "simple_ble_backend.h"

#include <stdint.h>


#define SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION                   (0)

#define TANKS_SERVICE_UUID                                          (0xA0)


class SimpleBLE
{
public:

    typedef int8_t TankId;

    enum TankType
    {
        READ,
        WRITE,
        WRITE_WITH_RESPONSE
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

    static const TankId INVALID_TANK_ID = -1;

    /**
     * @brief Construct a new Simple BLE object
     * 
     * @param ifc Complete SimpleBLE interface, with all external dependancies.
     */
#ifdef ARDUINO
    SimpleBLE() : backend(&arduinoIf) {}
#else //ARDUINO
    SimpleBLE(const SimpleBLEInterface *ifc) : backend(ifc) {}
#endif //ARDUINO

    /**
     * @brief Exit ULTRA LOW POWER mode on the module. Module UART interface
     * becomes active again and we can communicate with it.
     * 
     */
    inline void exitUltraLowPower(void) { backend.activateModuleRx(); }
    /**
     * @brief Enter ULTRA LOW POWER mode on the module. This will disable the UART
     * interface and all communication to the module will be disabled. All other
     * module functionality remains as is and module keeps advertising if
     * advertisement was started.
     * 
     * @note Even in ULP mode module will still send URC messages over UART.
     * @note ULP power consumption depends on the module configuration. If
     *       advertisement interval is very short power consumption will be high.
     * 
     */
    inline void enterUltraLowPower(void) { backend.deactivateModuleRx(); }
    /**
     * @brief Reset the module via reset pin. Do this only if software reset
     *        doesn't work.
     * 
     */
    inline void hardResetModule(void) { backend.hardResetModule(); }

    /**
     * @brief Initialise pins to initial values and put module to known state.
     * 
     */
    bool begin();

    TankId addTank(TankType type, uint32_t maxSizeBytes);

    /**
     * @brief Restart Simple BLE module via builtin command.
     * 
     * @return true If module successfuly restarted.
     * @return false If and error occured during module restart.
     */
    bool softRestart(void) { backend.softRestart(); }

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
    inline bool startAdvertisement(uint32_t advPeriod,
                                   int32_t advDuration,
                                   bool restartOnDisc = true)
    { backend.startAdvertisement(advPeriod, advDuration, restartOnDisc); }
    /**
     * @brief Stop advertising.
     * 
     * @return true If advertising successfuly stoped.
     * @return false If failed to stop advertisement.
     */
    inline bool stopAdvertisement(void) { backend.stopAdvertisement(); }

    /**
     * @brief Set the BLE transmission power.
     * 
     * @param dbm Transmission power in dBm.
     * @return true If transmission power was successfuly set.
     * @return false If an error occured during transmission power adjustment.
     */
    bool setTxPower(TxPower dbm);

    bool setDeviceName(const char* newName);

    SimpleBLEBackend backend;

    int8_t tanksServiceIndex;

#ifdef ARDUINO
private:
// AltSoft lib uses these RX and TX pins for communication but it doesn't
// realy nead them to be defined here. This is just for reference.
    static const int RX_PIN = 8;
    static const int TX_PIN = 9;
    static const int RX_ENABLE_PIN = 10;
    static const int MODULE_RESET_PIN = 11;

    static AltSoftSerial altSerial;

    static const SimpleBLEInterface arduinoIf;
public:
#endif //ARDUINO
};


#endif//__SIMPLE_BLE_H__
