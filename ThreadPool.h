#include <functional>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <string>
#include <assert.h>


class ThreadPool
{
    public:
        typedef std::function<void()> Task;
        ThreadPool(const std::string &namearg="threadpool")
        : name(namearg),
        maxqueueSize(256),
        running(false)
        { }
        void setQueueSize(const int queuesize)
        {
            std::unique_lock<std::mutex> lck(mtx);
            maxqueueSize = queuesize;
        }
        int getmaxQueueSize()
        {
            std::unique_lock<std::mutex> lck(mtx);
            return maxqueueSize;
        }
        int getThreadNum()
        {
            if(running)
            {
                std::unique_lock<std::mutex> lck(mtx);
                return threads.size();
            }
        }
        ~ThreadPool()
        {
            if(running)
                stop();
        }
        void stop() {
            {
                std::unique_lock<std::mutex> lck(mtx);
                running = false;
                notEmpty.notify_all();
            }
            for (auto &thr : threads)
                thr->join();
            int n=threads.size();
            for(int i=0;i<n;++i)
                delete threads[i];
        }
        void start(int numThreads)
        {
            assert(threads.empty());
            running = true;
            threads.reserve(numThreads);
            for(int i=0;i<numThreads;++i)
            {
                std::thread *thr_ptr = new std::thread(std::bind(&ThreadPool::runInThread,this));
                threads.push_back(thr_ptr);
            }
        }
        void run(Task task)
        {
            assert(running);
            {
                std::unique_lock<std::mutex> lck(mtx);
                while(queue.size()>=maxqueueSize)
                {
                    notFull.wait(lck);
                }
                assert(queue.size()<maxqueueSize);
                queue.push_back(task);
                notEmpty.notify_all();
            }
        }
        std::thread *getFirstThread()
        {
            std::unique_lock<std::mutex> lck(mtx);
            if(!threads.empty())
                return threads[0];
            else
                return nullptr;
        }
    private:
        std::mutex mtx;
        std::condition_variable notEmpty;
        std::condition_variable notFull;
        std::string name;
        int maxqueueSize;
        bool running;
        void runInThread()
        {
            while (running)
            {
                Task task(take());
                if(task)
                    task();
            }
        }
        Task take()
        {
            std::unique_lock<std::mutex> lck(mtx);
            while (queue.empty() && running)
                notEmpty.wait(lck);
            Task task=NULL;
            if(!queue.empty())
            {
                task=queue.front();
                queue.pop_front();
                if(maxqueueSize>0)
                    notFull.notify_all();
            }
            return task;
        }
        std::vector<std::thread *> threads;
        std::deque<Task> queue;
};

// 关于线程池的使用
/*
    ThreadPool threads;//("threadpool");
    threads.setQueueSize(1024); // 设置线程池中最多可以容纳的任务数为1024
    threads.start(10); // 在线程池中开启10个线程
    threads.run(cb); // 将cb放到线程池的任务队列
*/