#ifndef __SIMPLE_BLE_H__
#define __SIMPLE_BLE_H__

#include "simple_ble_backend.h"

#include <stdint.h>


#define SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION                   (0)


class SimpleBLE
{
public:

    /**
     * @brief Construct a new Simple BLE object
     * 
     * @param ifc Complete SimpleBLE interface, with all external dependancies.
     */
    SimpleBLE(const SimpleBLEInterface *ifc) : backend(ifc)
    {}

    /**
     * @brief Activate module serial reception of data.
     * 
     */
    inline void activateModuleRx(void) { backend.activateModuleRx(); }
    /**
     * @brief Deactivate module serial reception of data to save power.
     * 
     */
    inline void deactivateModuleRx(void) { backend.deactivateModuleRx(); }
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
    void begin();

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
                                   bool restartOnDisc)
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
    inline bool setTxPower(TxPower dbm) { backend.setTxPower(dbm); }


    SimpleBLEBackend backend;
};


#endif//__SIMPLE_BLE_H__
