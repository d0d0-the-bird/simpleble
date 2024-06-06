#include "simple_ble.h"
#include "at_process.h"

#include <stdlib.h>
#include <string.h>


#define MODULE_RX_BLOCK_SIZE_B                                  (6)

static const char cmdEnding[] = "\r";
static const char cmdAck[] = "\nOK\r\n";
static const char cmdError[] = "ERROR\r\n";


void SimpleBLE::begin()
{

    backend.begin();
}


