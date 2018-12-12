#include <iostream>
#include <sys/signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define MAX_EVENT_NUMBER 1024
#define MAX_BUFFER_SIZE 1024

static int pipefd[2];//管道数组定义
static int epollfd;//epoll描述符


static void AddFd(int epoll_fd,int fd){
   //将fd添加到epoll_fd对应的内核事件表中
   epoll_event event;
   event.data.fd = fd;//填充需要监听的文件描述符
   event.events = EPOLLIN | EPOLLET;//监听输入，使用ET模式
   epoll_ctl(epoll_fd,EPOLL_CTL_ADD,fd,&event); //注册对应的事件
}
static void signal_handler(int signo){
	//信号处理函数，主要处理SIGINT,SIGTERM信号
  if(signo == SIGINT)
     printf("sigint signal!");
  else if(signo == SIGTERM)
     printf("sigterm signal!");
  int saveno = errno;
  int msg = signo;
  send(pipefd[1],(char*)&msg,1,0);//管道也可以使用send函数，注意第二个参数要转换为char*
  errno = saveno;
}
void Addsig(int signo){
    
}
int main(int argc,char* argv[]){
   if(argc <= 2){
   	  printf("usage:%s ip_address port_number\n",argv[0]);
   	  exit(0);
   }
   epoll_event events[MAX_EVENT_NUMBER];
   epollfd = epoll_create(5);//一个epollfd最多绑定多少个描述符
   if(epollfd < 0){
   	  printf("can not get the epollfd!");
   	  exit(-1);
   }
   AddFd(epollfd,listenfd);//将监听描述符加到epollfd对应的事件表上
   //注册信号处理函数
   AddSig(SIGINT);
   Addsig(SIGTERM);
   bool stop_server = false;//服务器是否工作的标志
   
   int ret;
   char buffer[MAX_BUFFER_SIZE];//数据缓冲区
   //事件循环
   while(!stop_server){
     int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);//等待事件触发返回
     if(number < 0 && (errno != EINTR)){
          printf("epoll error!");
          break;
     }
     //循环处理就绪事件，遍历events数组
     for(int i = 0;i < number;i++){
     	int sockfd = events[i].data.fd; //获取就绪事件的描述符
     	if(sockfd == listenfd){
     		//有新连接到来
     		struct sockaddress_in client_addr;
     		socklen_t client_len = sizeof(client_addr);//客户端地址长度
     		int conn_fd = accept(listenfd,(struct sockaddr*)client_addr,&client_len);
     		AddFd(epollfd,conn_fd); //将新连接的套接字描述符加到epoll内核事件表
     	}
     	else if(sockfd == pipefd[0] && (events[i].eventS & EPOLLIN)){
     		//处理信号处理程序传输过来的数据
     		int sig;
     		char signals[50];
     		ret = recv(sockfd,signals,sizeof(signals),0);//使用recv读取数据
     		if(recv == -1){
     			printf("receive error!");
     			continue;
     		}
     		else if(ret == 0){
     			printf("receive finished!");
     			continue;
     		}
     		else{ //处理传送给主循环的信息，遍历消息数组
     			  for(int i=0;i < ret;i++){
     				  switch(signals[i])
     				 {
     				    case SIGINT: 
     				    {    
     				       	 printf("server will be killed!");
                     stop_server = true;
                     break;
                }
                case SIGTERM:
                {
                     printf("server will be terminated!");
                     stop_server = true;
                     break;
                }
     				  }
     			   }
     		    }
     	    }
     	else if(events[i].events && EPOLLIN){
     		//处理客户连接传送过来的数据
            memset(buffer,0,sizeof(buffer));
            ret = recv(sockfd,buffer,sizeof(buffer)-1,0);//接收用户发送过来的数据，并存储到缓冲区
            //打印信息
            printf("get %d bytes of data from the client:%d\n",buffer,sockfd);
            //回射给客户端
            buffer[MAX_BUFFER_SIZE-1]='\0';//空字符结尾
            write(sockfd,buffer,sizeof(buffer));
     	}
     	else{
     	    printf("can not handle the event!");
     	    continue;
     	}
     }
   }
   //程序退出，释放资源
   close(listenfd);
   close(pipefd[0]);
   close(pipefd[1]);
   printf("The server is going to stop!");
   return 0;
}