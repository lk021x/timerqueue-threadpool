# 结合线程池的定时器设计

* 定时器到期后, 将timer从timerqueue中移除,并重新设置时间. 将timer的回调函数添加到线程池的task队列

* 重新设置时间的两个场合
  * 新添加timer时, 这个timer更早到期
  * 一个定时器到期
 
