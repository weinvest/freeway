#ifndef _TIME_RANGE_SCHEDULER_H
#define _TIME_RANGE_SCHEDULER_H
#include "common/Types.h"
#include "common/TimeRangePhase.h"
class TimerAction;
class TimeRangePhase;
class TimeRangeScheduler
{
public:
    typedef std::function<void(TimeRangeMap::const_iterator, TimeRangeMap::const_iterator)> OnTimeRangeChange;
    TimeRangeScheduler(TimeRangePhase& ranges);
    TimeRangeMap::const_iterator GetCurrentRange( void ) { return mCurrentRange; }
    void Check(DateTime exchangeTime, OnTimeRangeChange onTimeRangeChange);
private:
    void FindNextCheckTime(DateTime now);

    TimeRangePhase& mRanges;
    DateTime mNextCheckTime;
    TimeRangeMap::const_iterator mCurrentRange;
};
#endif

