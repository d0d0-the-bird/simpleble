#include "timeout.h"

#include <stdio.h>
#include <stdint.h>


Timeout::Timeout(uint32_t timeout) : timeout(timeout)
{
    restart();
}

MillisType *Timeout::_millis = NULL;

void Timeout::init(MillisType *millisF)
{
    if( millisF )
        _millis = millisF;
}

int32_t Timeout::remaining(void)
{
    uint32_t passedTime = passed();
    int32_t retval = timeout - passedTime ;

    if( timeout >= passedTime )
    {
        retval = retval < 0 ? INT32_MAX : retval ;
    }
    else
    {
        retval = retval > 0 ? INT32_MIN : retval ;
    }

    return retval;
}
