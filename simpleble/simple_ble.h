#ifndef __SIMPLE_BLE_H__
#define __SIMPLE_BLE_H__

#ifdef ARDUINO
#define USING_ARDUINO_INTERFACE
#endif //ARDUINO

#ifdef ESP32
#define USING_ESP32_BACKEND
#endif //ESP32

#ifdef USING_ARDUINO_INTERFACE
#include "Arduino.h"
#ifndef USING_ESP32_BACKEND
#include "AltSoftSerial.h"
#endif //USING_ESP32_BACKEND
#endif //USING_ARDUINO_INTERFACE

#ifdef USING_ESP32_BACKEND
#include "esp32_backend.h"
#else
#include "simple_ble_backend.h"
#endif //USING_ESP32_BACKEND

#include <stdint.h>


#define SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION                   (0)

#define TANKS_SERVICE_UUID                                          (0xA0)


#ifndef USING_ESP32_BACKEND
typedef SimpleBLEBackendInterface SimpleBLEInterface;
#endif //USING_ESP32_BACKEND

class SimpleBLE
{
public:

    typedef int8_t TankId;

#ifdef USING_ARDUINO_INTERFACE
    class TankData
    {
    public:
        // Constructor, when allocating an array always leave room for one more
        // byte in case Tank holds text
        TankData(uint32_t size) :
            id(SimpleBLE::INVALID_TANK_ID),
            size(size),
            data(new uint8_t[size+1])
        {}

        TankData(SimpleBLE::TankId validId, uint32_t size) : TankData(size)
        {
            id = validId;
        }

        // Copy constructor
        TankData(const TankData& other) : TankData(other.id, other.size)
        {
            if( data ) memcpy(data, other.data, size);
        }

        // Destructor
        ~TankData() { delete[] data; }

        SimpleBLE::TankId getId() { return id; }

        // Function to get the size of the buffer
        uint32_t getSize() const { return size; }

        // Function to get a pointer to the data buffer
        uint8_t* getData() { return data; }

        // Function to get a const pointer to the data buffer
        const uint8_t* getData() const { return data; }

        bool getBool() { return !!data[0]; }

        int getInt(int size=1)
        {
            uint32_t retval = 0;
            if( size >= 1 ) retval |= (uint32_t)(data[0]) << 0;
            if( size >= 2 ) retval |= (uint32_t)(data[1]) << 8;
            if( size >= 3 ) retval |= (uint32_t)(data[2]) << 16;
            if( size >= 4 ) retval |= (uint32_t)(data[3]) << 24;

            return (int)retval;
        }

        // We can always add '\0' because we allocate one byte more than we need
        // for tank data.
        char* getCString() { data[size] = '\0'; return (char*)data; }
        String getString() { return String(getCString()); }

        // Indexing operator for non-const access
        uint8_t& operator[](size_t index) { return data[index]; }

        // Indexing operator for const access
        const uint8_t& operator[](size_t index) const { return data[index]; }

    private:
        SimpleBLE::TankId id;
        uint32_t size;
        uint8_t* data;
    };
#endif //USING_ARDUINO_INTERFACE

    enum TankType
    {
        READ,
        WRITE,
        WRITE_CONFIRMED
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
#ifdef USING_ARDUINO_INTERFACE
    SimpleBLE() : backend(&arduinoIf) {}
#else //USING_ARDUINO_INTERFACE
    SimpleBLE(const SimpleBLEInterface *ifc) : backend(ifc) {}
#endif //USING_ARDUINO_INTERFACE

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
    bool softRestart(void) { return backend.softRestart(); }

    /**
     * @brief Start advertising with previously constructed payload with setAdvPayload
     *        function.
     * 
     * @param advPeriodMs Period between advertisements in milliseconds. How often
     *                  to advertise.
     * @param advDurationMs Advertisement duration. How long to advertise after first
     *                    packet. If you want infinite advertisement set this to
     *                    SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION .
     * @param restartOnDisc Should advertisement restart if client disconnects.
     * @return true If advertisement started.
     * @return false If advertisement failed to start.
     */
    inline bool startAdvertisement(uint32_t advPeriodMs,
                                   int32_t advDurationMs = SIMPLEBLE_INFINITE_ADVERTISEMENT_DURATION,
                                   bool restartOnDisc = true)
    { return backend.startAdvertisement(advPeriodMs, advDurationMs, restartOnDisc); }
    /**
     * @brief Stop advertising.
     * 
     * @return true If advertising successfuly stoped.
     * @return false If failed to stop advertisement.
     */
    inline bool stopAdvertisement(void) { return backend.stopAdvertisement(); }

    /**
     * @brief Set the BLE transmission power.
     * 
     * @param dbm Transmission power in dBm.
     * @return true If transmission power was successfuly set.
     * @return false If an error occured during transmission power adjustment.
     */
    bool setTxPower(TxPower dbm);

    bool setDeviceName(const char* newName);

    bool waitUpdates(TankId* tank, uint32_t* updateSize=NULL, uint32_t timeout=1000);

    /**
     * @brief Read data from tank.
     * 
     * @param tank Id of a tank we want to read.
     * @param buff Buffer in which to save tank data.
     * @param buffSize Buffer size.
     * @param readLen If not NULL, length of data read from a tank
     * @return bool false if no new data is read from a tank, true if new data is
     *              read from a tank
     */
    bool readTank(TankId tank, uint8_t *buff, uint32_t buffSize, uint32_t* readLen=NULL);

    /**
     * @brief Write data to a tank.
     * 
     * @param tank Id of a tank we want to write.
     * @param data Buffer with data that should be transfered to desired tank.
     * @param dataSize Data length in buffer.
     * @return true If data was successfuly sent to Simple BLE module.
     * @return false If data transmission to module was unsuccessful.
     */
    bool writeTank(TankId tank, const uint8_t *data, uint32_t dataSize);
    /**
     * @brief Write C string to a tank.
     * 
     * @param tank Id of a tank we want to write.
     * @param str Buffer with C string that should be transfered to desired tank.
     * @return true If data was successfuly sent to Simple BLE module.
     * @return false If data transmission to module was unsuccessful.
     */
    bool writeTank(TankId tank, const char *str);

#ifdef USING_ARDUINO_INTERFACE
    TankData manageUpdates(uint32_t timeout=1000);
#endif //USING_ARDUINO_INTERFACE

#ifdef USING_ESP32_BACKEND
    typedef Esp32Backend BackendNs;
    Esp32Backend backend;
#else
    typedef SimpleBLEBackend BackendNs;
    SimpleBLEBackend backend;
#endif //USING_ESP32_BACKEND

    int8_t tanksServiceIndex;

#ifdef USING_ARDUINO_INTERFACE
private:
// AltSoft lib uses these RX and TX pins for communication but it doesn't
// realy nead them to be defined here. This is just for reference.
    static const int RX_PIN = 8;
    static const int TX_PIN = 9;
    static const int RX_ENABLE_PIN = 10;
    static const int MODULE_RESET_PIN = 11;

#ifdef USING_ESP32_BACKEND
    static const Esp32BackendInterface arduinoIf;
#else
    static AltSoftSerial altSerial;

    static const SimpleBLEBackendInterface arduinoIf;
#endif //USING_ESP32_BACKEND
public:
#endif //USING_ARDUINO_INTERFACE
};


#endif//__SIMPLE_BLE_H__
