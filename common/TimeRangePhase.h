#ifndef ARAGO_TIMERANGEPHASE_H
#define ARAGO_TIMERANGEPHASE_H
#include "common/TimeRange.h"
#include "json.hpp"
using json = nlohmann::json;
class TimeRangePhase
{
public:
    TimeRangePhase();
    typedef void* DataT;
    typedef dmap<TimeRange, DataT> TimeRangeMap;

    void Add(const TimeRange& range,DataT pData);
    void Remove(const TimeRange& range);
    void Clear();

    bool Complete(DataT pDefaultData);

    TimeRangeMap::iterator begin() { return mTimeRanges.begin(); }
    TimeRangeMap::iterator end() { return mTimeRanges.end(); }
    TimeRangeMap::const_iterator begin() const { return mTimeRanges.begin(); }
    TimeRangeMap::const_iterator end() const { return mTimeRanges.end(); }
    TimeRangeMap::const_iterator cbegin() const { return mTimeRanges.cbegin(); }
    TimeRangeMap::const_iterator cend() const { return mTimeRanges.cend(); }
    TimeRangeMap::const_iterator GetCurrent(TimeSpan t);
    TimeRangeMap::const_iterator GetRecently(void) const;
    TimeRangeMap::const_iterator prev(TimeRangeMap::const_iterator it);
    TimeRangeMap::const_iterator next(TimeRangeMap::const_iterator it);
    TimeRangeMap::iterator prev(TimeRangeMap::iterator it);
    TimeRangeMap::iterator next(TimeRangeMap::iterator it);

    size_t Count() { return mTimeRanges.size(); }

    void Load(const json& conf
            , const std::string& phaseName
            , std::function<DataT (const TimeRange& range, const std::string&)> actionParser
            , DataT defaultValue);
private:
    void ResetCurrentIt();
    bool CheckValid(TimeRangeMap::const_iterator curItem
                    ,TimeRangeMap::const_iterator nextItem
                    ,TimeRangeMap& unknowns
                    ,DataT pDefaultData);

    TimeRangeMap mTimeRanges;
    std::atomic<TimeRangeMap::const_iterator> mCurrentIt;
};
typedef TimeRangePhase::TimeRangeMap TimeRangeMap;
#endif // TIMERANGEPHASE_H
