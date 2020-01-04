# 结合线程池的定时器设计

* 在`timerqueue`中有一个`loop`线程和线程池
   * `loop`线程负责设置定时器,更新`timerfd`的时间,将过期的定时器从`timerqueue`中移除,以及取消`timer`
   * 当定时器到期后, `loop`线程会将`timer`的回调添加到线程池的`task`队列,线程池负责定时器回调函数的执行
 
 * 更新`timerfd`时间的场合
   * 新添加定时器时, 这个定时器更早到期
   * 一个定时器到期后,`loop`线程将会移除`timerqueue`中的所有已经到期的`timer`
     * 如果`timerqueue`中还有`timer`,则用最早到期的`timer`的时间设置`timerfd`的时间
     * 如果`timerqueue`中没有`timer`,设置`timerfd`的时间为从现在开始的100ms后
     
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


