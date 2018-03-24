#include "TimeRangeScheduler.h"
#include "clock/Clock.h"
TimeRangeScheduler::TimeRangeScheduler(TimeRangePhase& ranges)
    :mRanges(ranges)
{
    FindNextCheckTime(Clock::Instance().GetExchangeTime());
}
    
void TimeRangeScheduler::FindNextCheckTime(DateTime now)
{
    static const auto ONE_MILLISECONDS = boost::posix_time::milliseconds(1);
    static const auto computeInterval = [](TimeSpan t1, TimeSpan t2)
    {
        if(t1 < t2)
        {
            return t1 + boost::posix_time::hours(24) - t2;
        }
        return t1 - t2;
    };

    auto itCurrentRange = mRanges.GetCurrent(now.time_of_day());
    auto curTime = now.time_of_day();
    while(computeInterval(itCurrentRange->first.GetEnd(), curTime) <= ONE_MILLISECONDS)
    {
        itCurrentRange = mRanges.next(itCurrentRange);
    }

    auto time = DateTime(now.date(), itCurrentRange->first.GetEnd());
    if(time < now)
    {
	    time += boost::posix_time::hours(24);
    }

    mNextCheckTime = time;
    mCurrentRange = itCurrentRange;
}

void TimeRangeScheduler::Check(DateTime exchangeTime, OnTimeRangeChange onTimeRangeChange)
{
    if(exchangeTime >= mNextCheckTime )
    {
        onTimeRangeChange(mCurrentRange, mRanges.next(mCurrentRange));
        FindNextCheckTime(exchangeTime);
    }
}

