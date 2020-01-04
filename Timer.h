#include <atomic>
#include <functional>
#include "Timestamp.h"

class Timer
{
public:
    typedef std::function<void()> Callback;

    Timer(Callback cb,Timestamp when,double interval)
       : cb_(cb),
         expiration_(when),
         interval_(interval),
         sequence_(setTimerSequence()),
         repeat_(interval>0.0),
         hasCanceled(false)
    {

    }

    Timer()
      : cb_(NULL),
        expiration_(Timestamp::invalidTimestamp()),
        interval_(0.0),
        sequence_(0),
        repeat_(false),
        hasCanceled(false)
    {

    }
    Timer(Timer &) = delete;
    Timer(Timer &&) = delete;
    
    void run() const
    {
        cb_();
    }

    Timestamp getExpiration() const
    {
        return expiration_;
    }

    bool isRepeated() const
    {
        return repeat_;
    }

    int64_t getSequence() const
    {
        return sequence_;
    }

    void restart(Timestamp now)
    {
        if(repeat_)
        {
            expiration_ = now + interval_;
        }
        else
        {
            expiration_ = Timestamp::invalidTimestamp();
        }
    }
    double getInterval()
    {
        return interval_;
    }
    Callback getCallback()
    {
        return cb_;
    }

    bool getCancelStatus()
    {
        return hasCanceled;
    }
    void cancelTimer()
    {
        hasCanceled = true;
    }

private:
    const Callback cb_;
    Timestamp expiration_;
    const double interval_;
    bool repeat_ = false;
    const int64_t sequence_ ; //定时器的序号

    bool hasCanceled;
    static std::atomic_long s_numCreated ; // 用于设置定时器的序号

    static int64_t setTimerSequence()
    {
        s_numCreated.fetch_add(1);
        int64_t res = s_numCreated.load(std::memory_order_relaxed);
        return res;
    }

};
std::atomic_long Timer::s_numCreated ;