#include "TimerQueue.h"
#include <iostream>

using namespace std;

void fun()
{
    Timestamp now(Timestamp::now());
    cout << now.toFormattedString() << endl;
}

int main()
{
    TimerQueue tq;
    Timestamp now(Timestamp::now());

    for(int i=0;i<60;++i)
    {
        tq.add_timer(fun,now + i);
    }
    tq.add_timer(fun, 2020, 1, 6, 19, 20, 20,20); // 在2019-1-6 19:20:20:20执行fun, 这个时间是本机时间而不是GMT时间
    tq.wait();
}

// 编译: g++ -o test -lpthread  test.cpp
