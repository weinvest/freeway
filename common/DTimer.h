#ifndef _DTIMER_H
#define _DTIMER_H
#include <functional>
#include "utils/DMacros.h"
#include "common/Types.h"
PRE_DECLARE_CLASS(io_service, (boost)(asio))
class DTimer;
class DTimerEngine
{
public:
    virtual ~DTimerEngine( void ) {}
    virtual void Start( void ) = 0;
    virtual void Stop( void ) = 0;
    virtual bool AddTimer(DTimer* pTimer) = 0;
    virtual void SetNow(DateTime now) {};
    virtual int64_t GetCurrentTime( void ) = 0;
};

class DTimer
{
public:
    typedef std::function<void()> Action;
    DTimer( void );
    ~DTimer( void );

    void SetAction(Action act) { mAction = act; }
    void ExpireFromNow(TimeSpan span);
    void ExpireAt(DateTime t);
    void RepeatFrom(DateTime first, TimeSpan span); //调用这个函数以后不应该再调用ExpireFromNow, ExpireAt

    void Cancel( void );

#ifdef _USING_DFC_TIMER_ENGINE
    void SetNext(DTimer* pNext) { mNextTimer = pNext; }
    DTimer* GetNext( void ) { return mNextTimer; }
    int64_t GetExpireTime( void ) { return mExpireTime; }

    static DTimerEngine& GetRealTimerEngine( void );
    static DTimerEngine& GetSimuTimerEngine( void );
    static DTimerEngine& GetCurrTimerEngine( void );
#endif
    static boost::asio::io_service& GetIOService( void );
    static void RunNow(Action act);
    static std::shared_ptr<DTimer> ExpireFromNow(TimeSpan span, Action act);
    static std::shared_ptr<DTimer> ExpireAt(DateTime t, Action act);
    static std::shared_ptr<DTimer> RepeatFrom(DateTime first, TimeSpan span, Action act);
protected:
    Action mAction;
#ifdef _USING_DFC_TIMER_ENGINE
    friend class DTimerEngineBase;
    int64_t mExpireTime;
    int64_t mInterval;
    DTimer* mNextTimer;
    bool mCanceled;
#else
    void* mTimer;
#endif
};

#endif

