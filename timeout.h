#ifndef __TIMEOUT_H__
#define __TIMEOUT_H__


#include <stdio.h>
#include <stdint.h>


typedef uint32_t (MillisecondC)(void);


/**
 * @brief A very handy timeout class to handle timings in blocking parts of code.
 *        It is completely inlined for better performance.
 */
class Timeout
{
public:

    /**
     * @brief Construct a new Timeout object. It checks a static member for
     *        milliseconds counter function.
     * 
     * @param timeout Timeout to check for in milliseconds.
     */
    Timeout(uint32_t timeout) : timeout(timeout)
    {
        _millis = init(NULL);

        restart();
    }

    // Call this before any other calls.
    /**
     * @brief Static initializer for any instance of this class. It takes
     *        a milliseconds counter function as an argument, which serves for
     *        checking if timeout passed.
     * 
     * @param millisF Arduino like milliseconds counter. If NULL internal pointer
     *                won't be updated.
     * @return MillisecondC* Returns internal millis pointer value.
     */
    static MillisecondC *init(MillisecondC *millisF)
    {
        static MillisecondC *intMillis = NULL;
        
        if( millisF != NULL )
        {
            intMillis = millisF;
        }

        return intMillis;
    }

    /**
     * @brief Restarts the timeout counter.
     */
    inline void restart() { startTime = privMillis(); }

    inline uint32_t passed(void) { return privMillis() - startTime; }

    /**
     * @brief Checks if timeout value expired.
     * 
     * @return true Timeout expired.
     * @return false Timeout still not reached.
     */
    inline bool expired(void)
    {
        return passed() >= timeout ;
    }

    /**
     * @brief Checks if there is still time before timeout.
     * 
     * @return true Timeout still not reached.
     * @return false Timeout expired.
     */
    inline bool notExpired(void) { return !expired(); }

private:

    /**
     * @brief Wrapper for internal millis pointer. It provides security from
     *        unalowed memory access.
     * 
     * @return uint32_t Number of milliseconds from some point in code execution,
     *                  or 0 if no millis function provided.
     */
    inline uint32_t privMillis(void)
    {
        return _millis ? _millis() : 0 ;
    }

    MillisecondC *_millis;

    uint32_t startTime;
    uint32_t timeout;
};


#endif//__TIMEOUT_H__