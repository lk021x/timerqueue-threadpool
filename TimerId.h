#include "Timer.h"

// TimerId.h用于标识每个Timer
// TimerId还被用来取消定时器

class TimerId
{
public:
    TimerId() : 
        timer_(nullptr),
        sequence_(0)
    { }
    
    TimerId(Timer *timer) 
        : timer_(timer),
          sequence_(timer->getSequence())
    { }
        
    bool isValid()
    {
        return timer_!=NULL ;
    }
    Timer *getTimer() const
    {
        return timer_;
    }
    int64_t getSequence()
    {
        return sequence_;
    }
    
    void cancel()
    {
        timer_->cancelTimer();
    }
private:
    Timer *timer_;
    int64_t sequence_; // timer的序号,这个序号是唯一的,用来标识每个timer
};