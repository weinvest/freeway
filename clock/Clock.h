#ifndef _MY_CLOCK_H
#define _MY_CLOCK_H
#include "common/Types.h"
#include "common/DEnum.h"
#include "utils/Singleton.h"
class Clock
{
    DECLARE_AS_SINGLETON(Clock)
public:
#define CLOCK_MODE_VALUES ((Real, 0x0001))((BackTest, 0x0002))
    enum type
    {
        DENUM_VALUE(CLOCK_MODE_VALUES)
    };
    static const char* ToString(type v);
    static bool Parse(const std::string& value,type& v);

    static DateTime Align(DateTime time, TimeSpan align);

    DateTime Now( void ) const;
    Date Today( void ) const;
    TimeSpan TimeOfDay( void ) const;

    int64_t MicroSeconds1970(DateTime t) const;
    DateTime MicroSeconds1970(int64_t t) const;

    int64_t MilliSeconds1970(DateTime t) const;
    DateTime MilliSeconds1970(int64_t t) const;

    int64_t Seconds1970(DateTime t) const;
    DateTime Seconds1970(int64_t t) const;

    void SetMode(type t) { mMode = t; }
    void SetNow(DateTime t); //only used in BackTest mode

    void SetExchangeTime(DateTime t);
    DateTime GetExchangeTime( void ) const;
    DateTime GetAdjustExchangeTime( void ) const;
private:
    type mMode;
    DateTime mNow; //only used in BackTest mode
    DateTime mExchangeTime;
    int32_t mAvgDeviation;
};

#endif

