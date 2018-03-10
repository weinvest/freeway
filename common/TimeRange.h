#ifndef ARAGO_TIMERANGE_H
#define ARAGO_TIMERANGE_H
#include "common/Types.h"
class TimeRange
{
    TimeSpan mStart;
    TimeSpan mEnd;
    bool mIsStartOpen;
    bool mIsEndOpen;
    const int64_t mOneDayMilliSeconds;

public:
    TimeRange():mOneDayMilliSeconds(boost::posix_time::time_duration(24,0,0).total_milliseconds()){}
    void CloseStart(TimeSpan start);
    void OpenStart(TimeSpan start);

    void CloseEnd(TimeSpan end);
    void OpenEnd(TimeSpan end);

    const TimeSpan GetStart() const { return mStart; }
    const TimeSpan GetEnd() const { return mEnd; }
    const TimeSpan GetSpan() const;

    bool IsValid() const;
    bool IsStartOpen() const { return mIsStartOpen; }
    bool IsStartClose() const { return !IsStartOpen();}
    bool IsEndOpen() const { return mIsEndOpen; }
    bool IsEndClose() const { return !IsEndOpen(); }

    bool IsCrossDay() const { return mEnd < mStart; }
    bool IsInRange(const TimeSpan t) const;
    bool HasIntersection(const TimeRange& other) const;
    bool IsNeedComplete(const TimeRange& other) const;

    const TimeRange Complete(const TimeRange& other) const;
    const TimeRange Intersection(const TimeRange& other) const;
    int32_t Complete(TimeRange& range1,TimeRange& range2) const;

    int64_t GetEnd2StartMilliSeconds();
private:
    bool IsValid(const TimeSpan t) const;
};

bool operator < (const TimeRange lhs,const TimeRange rhs);
#endif // TIMERANGE_H
