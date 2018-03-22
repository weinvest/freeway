#include "clock/Clock.h"
IMPLEMENT_SINGLETON(Clock)
DENUM_IMPL_PARSE(Clock, CLOCK_MODE_VALUES)
DENUM_IMPL_TO_STRING(Clock, CLOCK_MODE_VALUES)

DateTime START_TIME(Date(1970,1,1));
Clock::Clock( void )
    :mMode(Clock::Real)
    ,mNow(Now())
    ,mExchangeTime(mNow)
    ,mAvgDeviation(0)
{

}

DateTime Clock::Now( void ) const
{
    if(Clock::Real == mMode)
    {
        //return boost::posix_time::microsec_clock::universal_time();
        return boost::posix_time::microsec_clock::local_time();
    }
    return mNow;
}

Date Clock::Today( void ) const
{
    return Now().date();
}

TimeSpan Clock::TimeOfDay( void ) const
{
    return Now().time_of_day();
}

int64_t Clock::MicroSeconds1970(DateTime t) const
{
    return (t - START_TIME).total_microseconds();
}

DateTime Clock::MicroSeconds1970(int64_t t) const
{
    return DateTime(START_TIME.date(), boost::posix_time::microseconds(t));
}

int64_t Clock::MilliSeconds1970(DateTime t) const
{
    return (t - START_TIME).total_milliseconds();
}

DateTime Clock::MilliSeconds1970(int64_t t) const
{
    return DateTime(START_TIME.date(), boost::posix_time::milliseconds(t));
}

int64_t Clock::Seconds1970(DateTime t) const
{
    return (t - START_TIME).total_seconds();
}

DateTime Clock::Seconds1970(int64_t t) const
{
    return DateTime(START_TIME.date(), boost::posix_time::seconds(t));
}

DateTime Clock::Align(DateTime time, TimeSpan align)
{
    auto date = time.date();
    int64_t count = time.time_of_day().total_microseconds() / align.total_microseconds();
    return DateTime(date, boost::posix_time::microseconds((count + 1) * align.total_microseconds()));
}

void Clock::SetExchangeTime(DateTime t)
{
    static const int32_t DEVIATION_THRESHOLD = 30 * 1e6;
    if(t > mExchangeTime)
    {
        mExchangeTime = t;
        auto deviation = (t - Now()).total_microseconds();
        if(deviation < DEVIATION_THRESHOLD && deviation > -DEVIATION_THRESHOLD)
        {
            mAvgDeviation = 0.8 * mAvgDeviation + 0.2 * deviation;
        }
    }
}

DateTime Clock::GetExchangeTime( void ) const
{
    return mExchangeTime;
}

DateTime Clock::GetAdjustExchangeTime( void ) const
{
    return Now() + boost::posix_time::microseconds(mAvgDeviation);
}