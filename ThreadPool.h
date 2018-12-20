#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <pthread.h>
#include <mutex>
#include <cstdio>
#include <exception>
#include "locker.h"

template<class T>
class ThreadPool{
private:
	size_t ThreadNumber;//线程池中线程数量

	//线程池允许的最大请求数
	size_t MaxRequestNumber;

	pthread_t* Threads;//描述所有线程的数组，数组大小为ThreadNumber

	//工作队列，使用STL的List容器实现
    std::list<T*> WorkQueue;
    //mutex mux;
    //线程池是否停止
    bool IsStopped; 
    //信号量
    Sem QueueStat;
    //请求队列的锁
    Locker QueueLocker;
public:
	//构造函数
    ThreadPool(size_t thread_number = 8,size_t max_request_number = 10000);
    //析构函数
    ~ThreadPool();
    //往请求队列中添加任务
    bool append(T* request);
private:
	//任务处理函数，每个线程都会跑这个函数，不断从工作队列中取出任务并执行
	static void* worker(void* arg);
	//线程池启动
	void run();
};

#endif