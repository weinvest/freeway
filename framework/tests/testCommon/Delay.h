//
// Created by shugan.li on 18-3-27.
//

#ifndef FREEWAY_DELAY_H
#define FREEWAY_DELAY_H
#include "clock/Clock.h"

struct Delay
{
    Delay(int32_t maxTimes)
            :raiseTimes(new DateTime[maxTimes])
            ,runTimes(new DateTime[maxTimes])
    {
    }

    auto setRaisedTime(DateTime t) { raiseTimes[nextRaiseIdx++] = t; }
    auto setRunTime(DateTime t) { runTimes[nextRunIdx++] = t; }
    TimeSpan meanTime( void );

    DateTime *raiseTimes;
    DateTime *runTimes;
    int32_t nextRaiseIdx{0};
    int32_t nextRunIdx{0};
};


#endif //FREEWAY_DELAY_H
