#include "TimerQueue.h"
#include <iostream>

using namespace std;

void fun(int i)
{
    cout << i << endl;
}

int main()
{
    TimerQueue tq;
    Timestamp now(Timestamp::now());

    for(int i=0;i<10;++i)
    {
        tq.add_timer(std::bind(fun,i),now + i);
    }
    
    tq.wait();
}

// 编译: g++ -o test -lpthread  test.cpp