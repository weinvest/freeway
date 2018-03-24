#include <mutex>
#include <thread>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "common/DTimer.h"
#include "DSpinLock.h"

#ifdef _USING_DFC_TIMER_ENGINE
class ChapterRing
{
public:
    ChapterRing(int32_t nChapterCount, int64_t timeDuration)
    :mCapterCount(nChapterCount)
    ,mCurrentHand(0)
    ,mTimeDuration(timeDuration)
    ,mRing(new DTimer*[nChapterCount])
    ,mNextRing(nullptr)
    {
        memset(mRing, 0, sizeof(DTimer**) * nChapterCount);
    }

#if 0
    std::string Name;
#endif

    ~ChapterRing()
    {
        delete[] mRing;
    }

    void Initialize(int64_t nowTime)
    {
        auto unitDuration = mTimeDuration / mCapterCount;
        auto slotIndex = nowTime % mTimeDuration;
        mStartTime = (nowTime / mTimeDuration) * mTimeDuration;
        mEndTime = mStartTime + mTimeDuration;
        mCurrentHand = slotIndex / unitDuration;
    }

    bool Insert(DTimer* pTimer)
    {
        auto expireTime = pTimer->GetExpireTime();
        //std::cout << this << "insert:" << expireTime <<"\n";
        if(expireTime < mEndTime)
        {
            auto unitDuration = mTimeDuration / mCapterCount;
            auto slotIndex = expireTime % mTimeDuration;
            slotIndex /= unitDuration;
            if(slotIndex <= mCurrentHand)
            {
#if 0
                std::cout << "insert timer(" << expireTime << ") to ring-" << Name << " faied\n";
#endif
                return false;
            }

#if 0
            std::cout << "insert timer(" << expireTime << ") to ring-" << Name << " at:" << slotIndex << "\n";
#endif
            Insert2(pTimer, slotIndex);
            return true;
        }
        return false;
    }

    void Insert2(DTimer* pTimer, int32_t slotIndex)
    {
        auto pNextTimer = mRing[slotIndex];
        mRing[slotIndex] = pTimer;
        pTimer->SetNext(pNextTimer);
    }

    DTimer* Tick( void )
    {
        if((mCurrentHand == (mCapterCount-1)))
        {
            mCurrentHand = -1;
            mStartTime = mEndTime;
            mEndTime += mTimeDuration;

            if(nullptr != mNextRing)
            {
#if 0
                auto nextHand = mNextRing->mCurrentHand;
                std::cout << Name << " tick to " << mNextRing->Name << " at " << nextHand << "\n";
#endif
                auto pTimer = mNextRing->Tick();
                while(nullptr != pTimer)
                {
#if 0
                    std::cout << Name << " tick to " << mNextRing->Name << " at " << nextHand << " and get timer:" << pTimer->GetExpireTime() <<"\n";
#endif
                    auto pNextTimer = pTimer->GetNext();
                    auto insertResult = Insert(pTimer);
                    assert(insertResult);
                    pTimer = pNextTimer;
                }
            }
        }

        ++mCurrentHand;

        auto pCurrentTimerList = mRing[mCurrentHand];
        mRing[mCurrentHand] = nullptr;
        return pCurrentTimerList;
    }

    void SetNext(ChapterRing* pNextRing) { mNextRing = pNextRing; }
    auto GetNext( void ) { return mNextRing; }

    auto GetCurrentTime( void ) { return mStartTime + mCurrentHand * mTimeDuration / mCapterCount; }

    auto GetCurrentHand( void ) const { return mCurrentHand; }

    auto GetCapterCount( void ) const { return mCapterCount; }
private:
    int32_t mCapterCount;
    int32_t mCurrentHand;
    int64_t mTimeDuration;
    int64_t mStartTime;
    int64_t mEndTime;
    DTimer** mRing;
    ChapterRing* mNextRing;

};

static const DateTime DFC_EPOCH_TIME(Date(1970,1,1));
class DTimerEngineBase: public DTimerEngine
{
public:
    DTimerEngineBase()
    :mMillisecondRing(100, 1000)
    ,mSecondRing(60, 60*1000)
    ,mMinuteRing(60, 60*60*1000)
    ,mHourRing(24, 24*60*60*1000)
    ,mDayRing(30, 30l*24*60*60*1000)
    {
        mMillisecondRing.SetNext(&mSecondRing);
        mSecondRing.SetNext(&mMinuteRing);
        mMinuteRing.SetNext(&mHourRing);
        mHourRing.SetNext(&mDayRing);
#if 0
        mMillisecondRing.Name = "Millis ";
        mSecondRing.Name = "Second ";
        mMinuteRing.Name = "Minute ";
        mHourRing.Name = "Hours  ";
        mDayRing.Name = "Day   ";
#endif
    }

    ~DTimerEngineBase()
    {
        Stop();
    }

    bool AddTimer(DTimer* pTimer) override
    {
        if(nullptr == pTimer)
        {
            return false;
        }

        auto pRing = &mMillisecondRing;
        do
        {
            SPINLOCK(mRingMutex);
            if(pRing->Insert(pTimer))
            {
                return true;
            }
            pRing = pRing->GetNext();
        }
        while(nullptr != pRing);

        auto diff2Current = pTimer->GetExpireTime() - mMillisecondRing.GetCurrentTime();
        if(diff2Current < 0 && diff2Current > -CHECKING_INTERVAL_IN_MILLISECONDS)
        {
            SPINLOCK(mRingMutex);
            mMillisecondRing.Insert2(pTimer, (mMillisecondRing.GetCurrentHand() + 1) % mMillisecondRing.GetCapterCount());
            return true;
        }
        return false;
    }

    int64_t GetCurrentTime( void ) override
    {
        auto now = boost::posix_time::microsec_clock::local_time();
        auto duraMilliseconds = (now - DFC_EPOCH_TIME).total_milliseconds();
        return duraMilliseconds;
    }

    void MainLoop( void )
    {
        try
        {
            while (nullptr != mCheckingThread)
            {
                auto nowTime = GetCurrentTime();
                auto waitTime = 10 - nowTime % CHECKING_INTERVAL_IN_MILLISECONDS;
                std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));

                while (mMillisecondRing.GetCurrentTime() <= (nowTime - CHECKING_INTERVAL_IN_MILLISECONDS / 2))
                {
                    DTimer *pTriggeredTimer = nullptr;
                    {
                        SPINLOCK(mRingMutex);
                        pTriggeredTimer = mMillisecondRing.Tick();
                    }

                    while (nullptr != pTriggeredTimer)
                    {
                        auto pNextTimer = pTriggeredTimer->GetNext();
                        pTriggeredTimer->SetNext(nullptr);

                        if (!pTriggeredTimer->mCanceled)
                        {
                            pTriggeredTimer->mAction();
                            if (0 != pTriggeredTimer->mInterval)
                            {
                                pTriggeredTimer->mExpireTime += pTriggeredTimer->mInterval;
                                AddTimer(pTriggeredTimer);
                            }
                        }
                        pTriggeredTimer = pNextTimer;
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
#if 0
            std::cout << e.what() << "\n";
#endif
        }
#if 0
        std::cout << "time exit:\n";
#endif
    }

    void Start( void ) override
    {
        if(mCheckingThread)
        {
            return;
        }

        auto nowTime = GetCurrentTime();
        auto pRing = &mMillisecondRing;
        do
        {
            pRing->Initialize(nowTime);
            pRing = pRing->GetNext();
        }
        while(nullptr != pRing);
        mCheckingThread.reset(new std::thread([this](){ MainLoop();}));
    }

    void Stop( void ) override
    {
        if(mCheckingThread)
        {
            auto pStopedThread = mCheckingThread;
            mCheckingThread = nullptr;
            pStopedThread->join();
        }
    }

private:
    static const int32_t CHECKING_INTERVAL_IN_MILLISECONDS = 10;
    ChapterRing mMillisecondRing;
    ChapterRing mSecondRing;
    ChapterRing mMinuteRing;
    ChapterRing mHourRing;
    ChapterRing mDayRing;
    DSpinLock mRingMutex;

    std::shared_ptr<std::thread> mCheckingThread;
};

DTimer::DTimer( void )
:mExpireTime(0)
,mInterval(0)
,mNextTimer(nullptr)
,mCanceled(false)
{}

DTimer::~DTimer( void )
{
}

void DTimer::ExpireFromNow(TimeSpan span)
{
    mExpireTime = GetCurrTimerEngine().GetCurrentTime() + span.total_milliseconds();
    mInterval = 0;
    GetCurrTimerEngine().AddTimer(this);
}

void DTimer::ExpireAt(DateTime t)
{
    mExpireTime = (t - DFC_EPOCH_TIME).total_milliseconds();
    mInterval = 0;
    GetCurrTimerEngine().AddTimer(this);
}

void DTimer::RepeatFrom(DateTime first, TimeSpan span)
{
    mExpireTime = (first - DFC_EPOCH_TIME).total_milliseconds();
    mInterval = span.total_milliseconds();
    GetCurrTimerEngine().AddTimer(this);
}

void DTimer::Cancel( void )
{
    mCanceled = true;
}

DTimerEngine& DTimer::GetRealTimerEngine( void )
{
    static DTimerEngineBase b;
    return b;
}

DTimerEngine& DTimer::GetSimuTimerEngine( void )
{
    return GetRealTimerEngine();
}

DTimerEngine& DTimer::GetCurrTimerEngine( void )
{
    return GetRealTimerEngine();
}
#else

#define GET_TIMER() ((boost::asio::deadline_timer*)mTimer)
DTimer::DTimer( void )
    :mTimer(new boost::asio::deadline_timer(GetIOService()))
{}

DTimer::~DTimer( void )
{
    delete GET_TIMER();
}

void DTimer::ExpireFromNow(TimeSpan span)
{
    auto pTimer = GET_TIMER();
    pTimer->expires_from_now(span);
    pTimer->async_wait([this](const boost::system::error_code& err)
    {
        if(!err)
        {
            mAction();
        }
    });
}

void DTimer::ExpireAt(DateTime t)
{
    auto pTimer = GET_TIMER();
    pTimer->expires_at(t - TimeSpan(8, 0, 0));
    pTimer->async_wait([this](const boost::system::error_code& err)
    {
        if(!err)
        {
            mAction();
        }
    });
}

void DTimer::RepeatFrom(DateTime first, TimeSpan span)
{
    auto action = mAction;
    SetAction([action, span, this]
    {
        action();
        ExpireFromNow(span);
    });
    ExpireAt(first);
}

void DTimer::Cancel( void )
{
    GET_TIMER()->cancel();
}
#endif
boost::asio::io_service& DTimer::GetIOService( void )
{
    static boost::asio::io_service* globalService = nullptr;
    static boost::asio::io_service::work* globalWork = nullptr;
    static std::thread* globalTimerThread = nullptr;
    if(nullptr != globalService)
    {
        return *globalService;
    }

    static std::once_flag init_instance;
    std::call_once(init_instance
    ,[]()
    {
        globalService = new boost::asio::io_service();
        globalWork = new boost::asio::io_service::work(*globalService);
        globalTimerThread = new std::thread([]() { globalService->run(); });
    });

    return *globalService;
}

void DTimer::RunNow(Action act)
{
    GetIOService().post(act);
}

std::shared_ptr<DTimer> DTimer::ExpireFromNow(TimeSpan span, Action act)
{
    auto pTimer = std::make_shared<DTimer>();
    pTimer->SetAction([act, pTimer]()
    {
        act();
    });
    pTimer->ExpireFromNow(span);

    return pTimer;
}

std::shared_ptr<DTimer> DTimer::ExpireAt(DateTime t, Action act)
{
    auto pTimer = std::make_shared<DTimer>();
    pTimer->SetAction([act, pTimer]()
    {
        act();
    });
    pTimer->ExpireAt(t);

    return pTimer;
}

std::shared_ptr<DTimer> DTimer::RepeatFrom(DateTime first, TimeSpan span, Action act)
{
    auto pTimer = std::make_shared<DTimer>();
    pTimer->SetAction([act, pTimer]()
    {
        act();
    });
    pTimer->RepeatFrom(first, span);

    return pTimer;
}
