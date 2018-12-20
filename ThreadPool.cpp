#include "ThreadPool.h"

template<class T> 
ThreadPool<T>::ThreadPool(size_t thread_number,size_t max_request_number):ThreadNumber(thread_number),
                         MaxRequestNumber(max_request_number),IsStopped(false),Threads(nullptr)
{   
	if(ThreadNumber < 0 || MaxRequestNumber < 0){
		perror("The arguments are invalid!\n");
		throw std::exception();//抛出异常，注意抛出的是异常对象
	}
	//创建一个线程数组，并初始化Threads指针
	this->Threads = new pthread_t[ThreadNumber];
	if(this->Threads == nullptr){
		perror("Failed to create pthread_t array!\n");
		throw std::exception();//抛出异常，注意抛出的是异常对象
	}
	for(int i = 0;i < ThreadNumber;i++){
		printf("create thread %d \n",i);
		if(pthread_create(Threads[i],NULL,worker,this) != 0){ //创建失败则销毁所有线程元素
			delete []Threads;
			throw std::exception();
		} 
		if(pthread_detach(Threads[i])){ //将每个线程都设置为脱离状态
            delete []Threads;
            throw std::exception();
		}
	}
}              
template<class T>
ThreadPool<T>::~ThreadPool(){
	delete []Threads;//销毁所有线程
	IsStopped = true;//置标志位为true
}

template<class T>
ThreadPool<T>::append(T* request){
   QueueLocker.lock();//多线程访问，需要上锁
   if(WorkQueue.size() > MaxRequestNumber){
   	  //超过线程池限制，直接返回false,在退出前需要先解锁，避免死锁
   	  QueueLocker.unlock();
   	  return false;
   }
   WorkQueue.push_back(request);//插入工作队列
   QueueLocker.unlock();//解锁
   //发出信号量
   QueueStat.post();
   return true;
}
//线程函数，每个线程都会运行这个函数
template<class T>
void* ThreadPool::worker(void* arg){
	ThreadPool* thread_pool = (ThreadPool*)arg;//获取参数中的线程池
    thread_pool->run();
    return thread_pool;//返回线程池对象指针
}
template<class T>
void ThreadPool<T>::run(){
	 while(!IsStopped) { //只要线程池还在运行
        QueueStat.wait(); //信号量，等待
        //操作任务队列，需要加锁
        QueueLocker.lock();
        if(WorkQueue.empty()){
        	QueueLocker.unlock();
        	continue;
        }
        T* request = WorkQueue.front();//获取双向链表的链表头，获取任务请求
        WorkQueue.pop_front();//删除请求
        QueueLocker.unlock();
        //处理请求
        if(request == nullptr){
        	printf("The current request is invalid!It will be abondoned!");
        	continue;
        }
        request->process();//调用请求的处理方法
	 }
}
