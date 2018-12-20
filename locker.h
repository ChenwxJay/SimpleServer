#ifndef LOCKER_H
#define LOCKER_H
#include <exception>
#include <iostream>
#include <cstio>

class Sem{
public:
	Sem();//构造函数
	~Sem(){
		//销毁信号量
		sem_destroy(&semp);
	}
	bool wait()//等待信号量
	{   //直接使用sem信号量的api
		return sem_wait(&semp) == 0;//注意返回值是一个bool型
	}
	bool post()//发布信号量
	{   //使用post的api
		return sem_post(&semp) == 0;//如果发送成功，就返回true
	}
private:
   sem_t semp;//指向信号量的IPC描述符
};
class Locker{
public:
	Locker(){
		//创建并初始化互斥锁
		if(pthread_mutex_init(&mux,NULL) != 0){
			printf("Initialize the mutex failed!");
			throw std::exception();
		}
	}
	//析构函数，销毁互斥锁
	~Locker(){
		pthread_mutex_destroy(&mux);
	}
	//获取锁
	bool lock(){
		return pthread_mutex_lock(&mux) == 0;//加锁，通过返回值来判断是否加锁成功
	}
	bool unlock(){
		return pthread_mutex_unlock(&mux) == 0;//解锁，使用底层API
	}
private:
	pthread_mutex_t mux;
};
class Cond{
public:
	Cond(){
		if(pthread_mutex_init(&mux,NULL) != 0){
			printf("Initialize the mutex failed!");
			throw std::exception();
		}
		if(pthread_cond_init(&cond,NULL) != 0){
			//一旦初始化条件变量出错，就应该释放已经分配的资源
			pthread_mutex_destroy(&mux);
			throw std::exception();
		}
	}
	~Cond(){
		
	}
}