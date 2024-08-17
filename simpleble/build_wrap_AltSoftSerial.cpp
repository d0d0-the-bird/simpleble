#include "simple_ble.h"

#if defined(USING_ARDUINO_INTERFACE) && !defined(USING_ESP32_BACKEND)
#include "AltSoftSerial.cpp"
#endif //defined(USING_ARDUINO_INTERFACE) && !defined(USING_ESP32_BACKEND)