#include "TimerId.h"
#include "ThreadPool.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <set>
#include <vector>
#include <string.h>

struct CompTime
{
    bool operator()(TimerId t1,TimerId t2)
    {
        if(t1.getTimer()->getExpiration().microSecondsSinceEpoch() != t2.getTimer()->getExpiration().microSecondsSinceEpoch())
        {
            return t1.getTimer()->getExpiration().microSecondsSinceEpoch() <=
                t2.getTimer()->getExpiration().microSecondsSinceEpoch() ;
        }
        else
        {
            return t1.getSequence() <= t2.getSequence();
        }
    }
};

class TimerQueue
{
public:
    typedef std::function<void()> Callback;
    TimerQueue()
        : maxTimerNum(1024),
          timerExecuteThreadNum(4),
          threadpoolname("timerqueue_threadpool"),
          exe_threads(threadpoolname),
          timerfd(createTimerfd()),
          stopped(false)
    {
        exe_threads.setQueueSize(maxTimerNum);
        exe_threads.start(timerExecuteThreadNum);
        loop_thread = new std::thread(std::bind(&TimerQueue::loop,this));
    }

    TimerQueue(int maxTimersNumber, int maxThreadPoolNum,const std::string &name="timerqueue_threadpool" )
        : maxTimerNum(maxTimersNumber),
          timerExecuteThreadNum(maxThreadPoolNum),
          threadpoolname(name),
          exe_threads(threadpoolname),
          timerfd(createTimerfd()),
          stopped(false)
    {
        exe_threads.setQueueSize(maxTimerNum);
        exe_threads.start(timerExecuteThreadNum);
        loop_thread = new std::thread(std::bind(&TimerQueue::loop,this));
    }

    TimerQueue(TimerQueue &) = delete;
    TimerQueue(TimerQueue &&) = delete;

    ~TimerQueue()
    {
        exe_threads.stop();
        for(auto it = timers_queue.begin(); it!=timers_queue.end(); ++it)
            delete it->getTimer();
        delete loop_thread;
    }
    TimerId add_timer(Callback cb, Timestamp when,double interval)
    {
        if(stopped)
            return NULL;
        std::unique_lock<std::mutex> lck(mtx);
        Timer *timer = new Timer(cb,when,interval);
        TimerId timerId(timer);
        
        if(timers_queue.size() == 0)
        {
            timers_queue.insert(timerId);
            resetTimerfd(timerfd,when);
        }
        else
        {
            if(when <= timers_queue.begin()->getTimer()->getExpiration())
            {
                resetTimerfd(timerfd,when);
            }
            timers_queue.insert(timerId);
        }
        return timerId;
    }

    void cancel_timer(TimerId *timerId)
    {
        std::unique_lock<std::mutex> lck(mtx);
        timerId->cancel();
    }
   

    TimerId add_timer(Callback cb,Timestamp when)
    {
        return add_timer(cb,when,0.0);
    }
    
    std::thread *getloopThread()
    {
        return loop_thread;
    }
    
    void stop()
    {
        stopped = true;
        exe_threads.stop();
    }

    void wait()
    {
        loop_thread->join();
    }
private:
    int maxTimerNum;
    int timerExecuteThreadNum;
    std::string threadpoolname = "threadpool";
    ThreadPool exe_threads; 

    std::set<TimerId,CompTime> timers_queue;

    bool stopped;
    const int timerfd ;

    fd_set reads;
    struct timeval timeout;

    std::mutex mtx;

    std::thread *loop_thread ;

    int createTimerfd()
    {
        int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                 TFD_NONBLOCK | TFD_CLOEXEC);
        assert(timerfd>=0);    
        return timerfd;
    }
    void loop()
    {
        int result = 0;
        fd_set tmps ;
        FD_ZERO(&reads);
        FD_SET(timerfd,&reads);
        
        while(!stopped)
        {
            tmps = reads;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            result = select(timerfd+1,&tmps,0,0,&timeout);
            if(result>0)
            {
                if(FD_ISSET(timerfd,&tmps))
                {
                    handleExpiredTimers();
                }
            }
            else if(result == 0)
            {
                ;
            }
        }
    }
    void handleExpiredTimers()
    {
        Timestamp now(Timestamp::now());

        uint64_t howmany;
        ssize_t n = ::read(timerfd, &howmany, sizeof howmany); 

        std::vector<TimerId> repeatedTimers;

        std::set<TimerId,CompTime>::iterator iter_up = timers_queue.begin();

        {
            std::unique_lock<std::mutex> lck(mtx);
            for(iter_up = timers_queue.begin(); iter_up!=timers_queue.end(); ++iter_up)
            {
                Timestamp Timestamp = iter_up->getTimer()->getExpiration();
                if(Timestamp.microSecondsSinceEpoch() <= now.microSecondsSinceEpoch())
                {
                    Callback cb = iter_up->getTimer()->getCallback();
                    if(iter_up->getTimer()->getCancelStatus()==false)
                        exe_threads.run(cb);
                }
                else
                {
                    break;
                }
            }

            for(auto first = timers_queue.begin();first != iter_up; ++first)
            {
                if(first!=timers_queue.end() && first->getTimer()->getCancelStatus()==false)
                {
                    if(first->getTimer()->isRepeated())
                    {
                        double interval = first->getTimer()->getInterval();
                        Timestamp newTimestamp = now + interval;
                        Timer *timer = new Timer(first->getTimer()->getCallback(),newTimestamp,interval);
                        TimerId timerId(timer);
                        repeatedTimers.push_back(timerId);
                    }
                    delete first->getTimer();
                }
            }
            if(iter_up!=timers_queue.begin())
                timers_queue.erase(timers_queue.begin(),iter_up);
            else
                timers_queue.erase(timers_queue.begin(),timers_queue.end());
            
            for(auto it=repeatedTimers.begin();it!=repeatedTimers.end();++it)
            {
                timers_queue.insert(*it);
            }
           
            // 重新设定时间
            if(timers_queue.size()>0)
            {
                Timestamp t = timers_queue.begin()->getTimer()->getExpiration();
                resetTimerfd(timerfd,t);
            }
            else
            {
                resetTimerfd(timerfd,now);
            }
        }
    }

    struct timespec howMuchTimeFromNow(Timestamp when) // 现在还有多长时间到when
    {
        int64_t microseconds = when.microSecondsSinceEpoch()
                                - Timestamp::now().microSecondsSinceEpoch();
        if (microseconds < 100)
        {
            microseconds = 100;
        }
        struct timespec ts;
        ts.tv_sec = static_cast<time_t>(
            microseconds / Timestamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(
            (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
        return ts;
    }

    void resetTimerfd(int timerfd,Timestamp expiration)
    {
        struct itimerspec newValue;
        struct itimerspec oldValue;
        memset(&newValue, 0,sizeof newValue);
        memset(&oldValue, 0,sizeof oldValue);
        newValue.it_value = howMuchTimeFromNow(expiration);
        int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    }

};
