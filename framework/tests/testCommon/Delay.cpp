//
// Created by shugan.li on 18-3-27.
//

#include "Delay.h"
TimeSpan Delay::meanTime( void )
{
    TimeSpan usedTime;
    int32_t from = 0;
    for(int32_t run = 0; run < nextRunIdx; ++run)
    {
        auto runTime = runTimes[run];
        for(int32_t raise = from; raise <= nextRaiseIdx; ++raise)
        {
            if(raiseTimes[raise] > runTime)
            {
                from = raise;
                usedTime +=  runTime - raiseTimes[raise-1];
                break;
            }
        }
    }

    return usedTime / nextRaiseIdx;
}