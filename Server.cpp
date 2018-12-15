#include <iostream>
#include <sys/signal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define MAX_EVENT_NUMBER 1024
#define MAX_BUFFER_SIZE 1024
#define MAX_CONNECTION_SIZE 50

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

void AddSig(int signo){
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));//内存块清零
    sa.sa_handler = signal_handler;//填充信号处理器
    sa.sa_flags |= SA_RESTART;//重新启动
    assert(sigaction(signo,&sa,NULL)!=-1);//判断返回值
}

static int SetNoBlocking(int fd){
   int old_option = fcntl(fd,F_GETFL);
   int new_option = old_option | O_NONBLOCK;
   fcntl(fd,F_SETFL,new_option);
   return old_option;//返回原来的状态
}

int main(int argc,char* argv[]){
   if(argc <= 2){
   	  printf("usage:%s ip_address port_number\n",argv[0]);
   	  exit(0);
   }

   const char* server_ip = argv[1];
   int port = atoi(argv[2]);//获取端口号

   int AcceptFd = 0;//监听套接字描述符定义
   struct sockaddr_in ServerAddr;
   int len = sizeof(ServerAddr);

   //创建套接字
   AcceptFd = socket(AF_INET,SOCK_STREAM,0);
   
   bzero(&ServerAddr,sizeof(ServerAddr));//清除内存块

   //填充协议族，使用IPV4
   ServerAddr.sin_family = AF_INET;

   //从命令行获取端口号，转化为网络字节序
   ServerAddr.sin_port = htons(port);

   //从命令行获取IP地址，并填充到套接字地址结构
   if(inet_pton(AF_INET,server_ip,&ServerAddr.sin_addr)==0){
        printf("inet_ntop error\n");//转换IP地址
        close(AcceptFd);//关闭描述符
        exit(0);
   }
  
   printf("bind in %s : %d\n", argv[1], ntohs(ServerAddr.sin_port));

   //绑定服务器套接字和监听描述符
   if (bind(AcceptFd, (struct sockaddr*)&ServerAddr,len) < 0) {
    printf("bind error\n");//绑定错误
    exit(0);
   }

   //listen，将监听套接字从主动变为被动
   if (listen(AcceptFd,MAX_CONNECTION_SIZE) < 0) {//listen，设置监听队列
    printf("listen error\n");
    return 0;
   }

   epoll_event events[MAX_EVENT_NUMBER];
   epollfd = epoll_create(10);//一个epollfd最多绑定多少个描述符
   if(epollfd < 0){
   	  printf("can not get the epollfd!");
   	  exit(0);
   }
   AddFd(epollfd,AcceptFd);//将监听描述符加到epollfd对应的事件表上

   int ret = 0;
   ret = socketpair(PF_UNIX,SOCK_STREAM,0,pipefd);//使用UNIX域套接字
   if(ret == -1){
      printf("create pipe error!");
      exit(0);
   }
   AddFd(epollfd,pipefd[1]);//将监听管道读端描述符加到epoll中
   //设置管道写端不阻塞
   SetNoBlocking(pipefd[0]);

   //注册信号处理函数
   AddSig(SIGINT);
   AddSig(SIGTERM);
   bool stop_server = false;//服务器是否工作的标志
   
   char buffer[MAX_BUFFER_SIZE];//数据缓冲区
   //事件循环
   while(!stop_server){
     int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);//等待事件触发返回
     if(number < 0 && (errno != EINTR)){ //发生错误
          printf("epoll error!");
          break;
     }
     //循环处理就绪事件，遍历events数组
     for(int i = 0;i < number;i++){
     	int sockfd = events[i].data.fd; //获取就绪事件的描述符
     	if(sockfd == AcceptFd){
     		//有新连接到来
     		struct sockaddr_in client_addr;
     		socklen_t client_len = sizeof(client_addr);//客户端地址长度
        //注意第二个参数必须是指针，然后执行强制类型转换
     		int conn_fd = accept(AcceptFd,(struct sockaddr*)&client_addr,(unsigned int *)&client_len);
     		AddFd(epollfd,conn_fd); //将新连接的套接字描述符加到epoll内核事件表
     	}
     	else if(sockfd == pipefd[0] && (events[i].events & EPOLLIN)){
     		//处理信号处理程序传输过来的数据
     		//int sig;
     		char signals[50];
     		ret = recv(sockfd,signals,sizeof(signals),0);//使用recv读取数据
     		if(ret == -1){ //检验返回值
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
            printf("get %d bytes of data from the client:%d\n",ret,sockfd);
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
   sleep(3000);
   //程序退出，释放资源
   close(AcceptFd);
   close(pipefd[0]);
   close(pipefd[1]);
   printf("The server is going to stop!");
   return 0;
}