#include "common/TimeRange.h"

const TimeSpan TimeRange::GetSpan() const
{
    if(IsCrossDay())
    {
        return mEnd - mStart + TimeSpan(24, 0, 0);
    }

    return mEnd - mStart;
}

void TimeRange::CloseStart(TimeSpan start)
{
    mStart = start;
    mIsStartOpen = false;
}

void TimeRange::OpenStart(TimeSpan start)
{
    mStart = start;
    mIsStartOpen = true;
}

void TimeRange::CloseEnd(TimeSpan end)
{
    mEnd = end;
    mIsEndOpen = false;
}

void TimeRange::OpenEnd(TimeSpan end)
{
    mEnd = end;
    mIsEndOpen = true;
}

bool TimeRange::IsValid() const
{
    return IsValid(mStart) && IsValid(mEnd) && (mStart != mEnd || (!mIsStartOpen && !mIsEndOpen));
}

bool TimeRange::IsValid(const TimeSpan t) const
{
    static TimeSpan midNight(24,0,0,0);
    return t <= midNight;
}


bool TimeRange::IsInRange(const TimeSpan t) const
{
    if(IsCrossDay())
    {
        if(t > mStart || (IsStartClose() && t == mStart))
        {
            return true;
        }
        else if(t < mEnd || (IsEndClose() && t == mEnd))
        {
            return true;
        }

        return false;
    }
    else
    {
        if(t > mStart && t < mEnd)
        {
            return true;
        }
        else if(IsStartClose() && t == mStart)
        {
            return true;
        }
        else if(IsEndClose() && t == mEnd)
        {
            return true;
        }
        return false;
    }
}

bool TimeRange::HasIntersection(const TimeRange& other) const
{
    bool r1 = IsInRange(other.mStart);
    if(r1)
    {
        if(other.mStart != this->mEnd)
        {
            return true;
        }
        return other.IsStartClose();
    }

    bool r2 = IsInRange(other.mEnd);
    if(r2)
    {
        if(other.mEnd != this->mStart)
        {
            return true;
        }
        return other.IsEndClose();
    }

    return false;
}

bool TimeRange::IsNeedComplete(const TimeRange& other) const
{
    return mEnd != other.mStart || (IsEndOpen() && other.IsStartOpen());
}

const TimeRange TimeRange::Intersection(const TimeRange& other) const
{
    TimeRange intersection;
    if(IsInRange(other.GetStart()))
    {
        if(other.IsStartOpen() || (other.GetStart() == GetStart() && IsStartOpen()))
        {
            intersection.OpenStart(other.GetStart());
        }
        else
        {
            intersection.CloseStart(other.GetStart());
        }

        if(IsEndOpen() || (other.GetEnd() == GetEnd() && other.IsEndOpen()))
        {
            intersection.OpenEnd(GetEnd());
        }
        else
        {
            intersection.CloseEnd(GetEnd());
        }
    }
    else
    {
        return other.Intersection(*this);
    }
    return intersection;
}

const TimeRange TimeRange::Complete(const TimeRange& other) const
{
    TimeRange complete;
    complete.mStart = this->mEnd;
    complete.mIsStartOpen = !this->mIsEndOpen;
    complete.mEnd = other.mStart;
    complete.mIsEndOpen = !other.mIsStartOpen;

    return complete;
}

int32_t TimeRange::Complete(TimeRange& range1,TimeRange& range2) const
{
    if(IsCrossDay())
    {
        range1.mStart = this->mEnd;
        range1.mIsStartOpen = !this->mIsEndOpen;
        range1.mEnd = this->mStart;
        range1.mIsEndOpen = !this->mIsStartOpen;
        return 1;
    }

    int32_t ret = 0;
    if(mStart != TimeSpan(0,0,0,0))
    {
        range1.mStart = TimeSpan(0,0,0,0);
        range1.mIsStartOpen = true;
        range1.mEnd = this->mStart;
        range1.mIsEndOpen = !this->mIsStartOpen;
        ret = 1;
    }

    if(mEnd != TimeSpan(24,0,0,0))
    {
        range2.mStart = this->mEnd;
        range2.mIsStartOpen = !this->mIsEndOpen;
        range2.mEnd = TimeSpan(24,0,0,0);
        range2.mIsEndOpen = true;
        ret |= 2;
    }
    return ret;
}

int64_t TimeRange::GetEnd2StartMilliSeconds()
{
    if(IsCrossDay() )
    {
        return mEnd.total_milliseconds()+mOneDayMilliSeconds - mStart.total_milliseconds();
    }
    else
    {
        return mEnd.total_milliseconds() - mStart.total_milliseconds();;
    }
}

bool operator < (const TimeRange lhs,const TimeRange rhs)
{
    return lhs.GetStart() < rhs.GetStart();
}
