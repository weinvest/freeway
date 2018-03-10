#include <boost/date_time/posix_time/time_parsers.hpp>
#include "common/TimeRangePhase.h"

TimeRangePhase::TimeRangePhase()
    :mCurrentIt(mTimeRanges.begin())
{
}

void TimeRangePhase::Add(const TimeRange& range,DataT pData)
{
    mTimeRanges.insert(std::make_pair(range,pData));
    ResetCurrentIt();
}

void TimeRangePhase::ResetCurrentIt()
{
    mCurrentIt.store(mTimeRanges.begin());
}

void TimeRangePhase::Remove(const TimeRange& range)
{
    mTimeRanges.erase(range);
    ResetCurrentIt();
}

void TimeRangePhase::Clear()
{
    mTimeRanges.clear();
    ResetCurrentIt();
}

bool TimeRangePhase::Complete(DataT pDefaultData)
{
    TimeRangeMap unknowns;
    if(mTimeRanges.size() > 1)
    {
        auto curItem = mTimeRanges.begin();
        auto nextItem = mTimeRanges.begin();

        for(++nextItem; nextItem != mTimeRanges.end(); ++curItem,++nextItem)
        {
            if(!CheckValid(curItem,nextItem,unknowns,pDefaultData))
            {
                return false;
            }
        }

        if(!CheckValid(curItem,mTimeRanges.begin(),unknowns,pDefaultData))
        {
            return false;
        }
    }
    else if(1 == mTimeRanges.size())
    {
        auto& item = mTimeRanges.begin()->first;
        unknowns.insert(std::make_pair(item.Complete(item),pDefaultData));
    }
    mTimeRanges.insert(unknowns.begin(),unknowns.end());
    ResetCurrentIt();
    return true;
}

bool TimeRangePhase::CheckValid(TimeRangeMap::const_iterator curItem
                                ,TimeRangeMap::const_iterator nextItem
                                ,TimeRangeMap& unknowns
                                ,DataT pDefaultData)
{
    if (!curItem->first.IsValid() || !nextItem->first.IsValid() || curItem->first.HasIntersection(nextItem->first))
    {
        return false;
    }
    else if(curItem->first.IsNeedComplete(nextItem->first))
    {
        unknowns.insert(std::make_pair(curItem->first.Complete(nextItem->first),pDefaultData));
    }

    return true;
}

TimeRangePhase::TimeRangeMap::const_iterator TimeRangePhase::GetCurrent(TimeSpan t)
{
    auto curIt = mCurrentIt.load();
    if(curIt != mTimeRanges.cend())
    {
        if(curIt->first.IsInRange(t))
        {
            return curIt;
        }
        else if(curIt->first.GetStart() >= t)
        {
            bool found = false;
            while(curIt != mTimeRanges.cbegin())
            {
                --curIt;
                if(curIt->first.IsInRange(t))
                {
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                curIt = mTimeRanges.cend();
                --curIt;
            }
        }
        else
        {
            bool found = false;
            for(++curIt;curIt != mTimeRanges.cend();++curIt)
            {
                if(curIt->first.IsInRange(t))
                {
                    found = true;
                    break;
                }
            }

            if(!found)
            {
                curIt = mTimeRanges.cbegin();
            }
        }

        mCurrentIt.store(curIt);
        return curIt;
    }

    return mTimeRanges.end();
}

TimeRangePhase::TimeRangeMap::const_iterator TimeRangePhase::GetRecently( void ) const
{
    return mCurrentIt.load();
}

void TimeRangePhase::Load(const json& conf
        , const std::string& phaseName
        , std::function<DataT (const TimeRange& range, const std::string&)> actionParser
        , DataT defaultValue)
{
    for(auto& rangeNode : conf)
    {
        TimeRange range;
        std::string strrange;
        if(1 == rangeNode.count("START"))
        {
            range.CloseStart(boost::posix_time::duration_from_string(rangeNode["START"].get<std::string>()));
        }
        else if(1 == rangeNode.count("start"))
        {
            range.OpenStart(boost::posix_time::duration_from_string(rangeNode["start"].get<std::string>()));
        }
        else
        {
            throw std::logic_error(std::string("can't find value for the start time of ") + strrange);
        }

        if(1 == rangeNode.count("END"))
        {
            range.CloseEnd(boost::posix_time::duration_from_string(rangeNode["END"].get<std::string>()));
        }
        else if(1 == rangeNode.count("end"))
        {
            range.OpenEnd(boost::posix_time::duration_from_string(rangeNode["end"].get<std::string>()));
        }
        else
        {
            throw std::logic_error(std::string("can't find value for the end time of ") + strrange);
        }

        DataT pAction = nullptr;
        if(0 != rangeNode.count(phaseName))
        {
            pAction = actionParser(range, rangeNode[phaseName].get<std::string>());
        }
        Add(range, pAction);
    }//for

    if(!Complete(defaultValue))
    {
        throw std::logic_error(std::string(" complete ranges failed, maybe there are overlap ranges."));
    }
}

TimeRangeMap::const_iterator TimeRangePhase::prev(TimeRangeMap::const_iterator it)
{
    if(it == mTimeRanges.cbegin())
    {
       it = mTimeRanges.cend();
    }
    --it;
    return it;
}

TimeRangeMap::const_iterator TimeRangePhase::next(TimeRangeMap::const_iterator it)
{
    ++it;
    if(it == mTimeRanges.cend())
    {
        return mTimeRanges.cbegin();
    }

    return it;
}

TimeRangeMap::iterator TimeRangePhase::prev(TimeRangeMap::iterator it)
{
    if(it == mTimeRanges.begin())
    {
        it = mTimeRanges.end();
    }
    --it;
    return it;
}

TimeRangeMap::iterator TimeRangePhase::next(TimeRangeMap::iterator it)
{
    ++it;
    if(it == mTimeRanges.end())
    {
        return mTimeRanges.begin();
    }
    return it;
}
