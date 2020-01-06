#include <sys/time.h>
#include <string>
#include <ctime>
#include <inttypes.h> // PRId64
#include <time.h>



    class Timestamp
    {
    public:
        Timestamp() : microSecondsSinceEpoch_(0) { }

        explicit Timestamp(int64_t microSecondsSinceEpochArg) : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
        { }
        
        Timestamp(int year, int month, int day , int hour, int minute, int sec, int mirSec)
            : microSecondsSinceEpoch_(secondsSinceEpoch(year,month , day , hour , minute, sec)*kMicroSecondsPerSecond + mirSec*1000)
        {

        }
        void swap(Timestamp &that)
        {
            std::swap(microSecondsSinceEpoch_,that.microSecondsSinceEpoch_);
        }

        std::string toString() const 
        {
            char buf[32] = {0};
            int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
            int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
            snprintf(buf, sizeof(buf)-1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
            return buf;
        }

        bool isvalid()
        { return microSecondsSinceEpoch_ > 0; }

        int64_t microSecondsSinceEpoch()
        {
            return microSecondsSinceEpoch_;
        }

        time_t secondsSinceEpoch() const 
        {
            return static_cast<time_t> (microSecondsSinceEpoch_ / kMicroSecondsPerSecond );
        }

        static Timestamp now() 
        {
            struct timeval tv;
            gettimeofday(&tv,NULL);
            int64_t seconds = tv.tv_sec;
            return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
        }

        static Timestamp invalidTimestamp() 
        {
            return Timestamp();
        }

        static Timestamp fromUnixTime(time_t t) 
        {
            return fromUnixTime(t,0);
        }

        static Timestamp fromUnixTime(time_t t, int microSec)
        {
            return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microSec ); 
        }

        static const int kMicroSecondsPerSecond = 1000 * 1000; 


        std::string toFormattedString(bool showMicroseconds=false) const
        {
            // 时间显示格式为: 年月日:时:分:秒:微秒
            char buf[64] ={0};
            time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); 
            struct tm tm_time;
            gmtime_r(&seconds,&tm_time); 

            if(showMicroseconds)
            {
                int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
                snprintf(buf,sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                    tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                    tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                    microseconds);
            }
            else
            {
                snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
                tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
            }
            return buf;
        }

        std::string getDate()
        {
            if(!isvalid())
                return "";
            else
            {
                std::string formatedStr=toFormattedString();
                return std::string(formatedStr.begin(),formatedStr.begin()+8);
            }
        }

        
    private:
          int64_t  microSecondsSinceEpoch_;  
          int64_t secondsSinceEpoch(int year, int month, int day , int hour, int minute, int sec)
        {
            struct tm t;
            char format[32] = "%H:%M:%S %d %m %Y" ; 
            char buf[32] ;
            sprintf(buf,"%d:%d:%d %d %d %d", hour,minute,sec,day,month,year);
            strptime(buf,format, &t);
            
            return mktime(&t);
        }
    };

    bool operator<=(Timestamp t1,Timestamp t2)
    {
        return t1.microSecondsSinceEpoch() <= t2.microSecondsSinceEpoch();
    }

    bool operator==(Timestamp t1,Timestamp t2)
    {
        return t1.microSecondsSinceEpoch() == t2.microSecondsSinceEpoch();
    }
    
    Timestamp operator+(Timestamp ts,double seconds)
    {
        int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond); 
        return Timestamp(ts.microSecondsSinceEpoch()+delta);
    }

