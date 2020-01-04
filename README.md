# 结合线程池的定时器设计

* 在timerqueue中有一个loop线程和线程池
   * loop线程负责设置定时器,更新timerfd的时间,将过期的定时器从timerqueue中移除,以及取消timer
   * 当定时器到期后, loop线程会将timer的回调添加到线程池的task队列,线程池负责定时器回调函数的执行
 
 * 更新timerfd时间的场合
   * 新添加定时器时, 这个定时器更早到期
   * 一个定时器到期后,loop线程将会移除timerqueue中的所有已经到期的timer
     * 如果timerqueue中还有timer,则用最早到期的timer的时间设置timerfd的时间
     * 如果timerqueue中没有timer,设置timerfd的时间为从现在开始的100ms后
     
 # 使用
 ```cpp
 #include "TimerQueue.h"
 #include <iostream>

 using namespace std;
 
 void print()
 {
     cout << "hello world" << endl;
 }
 int main()
 {
   TimerQueue tq;
   Timestamp now(Timestamp::now());
   Timestamp t = now +1.0; // 从现在开始的1s后
   TimerId timerId1 = tq.add_timer(print,t);
   
   TimerId timerId2 = tq.add_timer(print,t + 1.0);
   
   tq.cancel_timer(&timerId2); // 关于定时器的取消
   tq.wait();
 }
 ```


