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
    
    tq.wait();
}

// 编译: g++ -o test -lpthread  test.cpp
