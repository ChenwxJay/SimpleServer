#ifndef TIMER_H
#define TIMER_H

#include <iostream>
#include <time.h>
#include <netinet/in.h>
#include <functional>

using namespace std;

//前向声明,定时器节点类
class TimerNode{
public:
   TimerNode(int delay,function<void(ClientData*)> callback){
   	 this->expire_time = time(NULL) + delay;//在当前时间基础上叠加需要的延时
     this->callback = callback;//设置定时器回调函数
   }
   ~TimerNode(){}
private:
   time_t expire_time;//定时器定时绝对时间
   function<void(ClientData*)> callback;//定时器内置回调函数
};
//定时器类，使用最小堆实现
class Timer{
public:
	//构造函数，初始化定时器的容量，创建一个定时器节点数组
	Timer(int capacity)throw(std::exception):capacity(cap),CurrentSize(0)
	{   
		TimerNodeList = new vector<TimerNode*>(10,nullptr);//创建定时器节点指针数组
		if(TimerNodeList == nullptr){
			throw std::exception();//抛出异常对象
		}
		cout << "TImer create!" << endl;
	}
	Timer(Timer* timer,int size,int capacity);//使用已有的timer来构造
	//析构函数定义
	~Timer();
	//定时器的基本操作

	//添加定时器节点
	bool AddTimerNode(TimerNode* node)throw(std::exception);
	//删除定时器节点
	bool DelTimerNode(TimerNode* node)throw(std::exception);

private:

}