#include "HttpConnection.h"

//从epoll内核注册事件表中删除fd
void RemoveFd(int epollfd,int fd){
  //使用epoll_ctl函数
  epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
  //关闭文件描述符
  close(fd);
}

//修改fd对应的需要监听的事件
void ModifyFd(int epollfd,int fd,int ev){
  epoll_event event;
  event.data.fd = fd;
  event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;//设置需要监听的事件，参数ev是一个事件
  //调用epoll_ctl实现修改事件
  epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}

//关闭Http连接
void HttpConnection::CloseConnection(bool real_close){
   if(real_close == true && SockFd != -1){
       printf("The connection will be closed!");
       RemoveFd(EpollFd,SockFd);//从epoll内核事件注册表中删除套接字描述符SockFd
       SockFd = -1;//描述符无效

       //客户端连接数减一，即客户总量减一
       UserCount--;
   }
}
void HttpConnection::Init(int sockfd,const struct sockaddr_in& addr){
	printf("HttpConnection initialize!");
	//填充http连接的相关字段
	SockFd = sockfd;
	Address = addr;
    //调试时避免出现TIME_WAIT状态
    int reuse = 1;
    //设置重用地址选项，传入一个变量标志使能
    setsockpt(SockFd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    //注册fd到epoll内核事件表
    AddFd(EpollFd,SockFd,true);

    //用户总数加1
    UserCount++;
    //调用默认的init方法
    Init();
}
void HttpConnection::Init(){
    this->CheckState = CHECK_REQUESTLINE;
    this->Linger = false;//默认不开启长连接选项
    
    //初始化Http连接的信息，默认值
    this->Method = GET;
    this->Url = 0;
    this->Version = 0;
    this->ContentLength = 0;
    this->Host = 0;
    this->CurrentStartLine= 0;//起始行
    this->CurrentCheckedPos = 0;
    this->CurrentReadNextPos = 0;
}

//循环读取数据，直至没有数据可读或者对方关闭连接
bool HttpConnection::read(){
   if(this->CurrentReadNextPos >= READ_BUFFER_SIZE)
   		return false; //读位置下标已经超过缓冲区大小，读失败
   //循环读取
   int nRead = 0;
   while(true){
   	  //从客户端套接字读数据
      nRead = recv(SockFd,ReadBuffer + CurrentReadNextPos,READ_BUFFER_SIZE - CurrentReadNextPos,0);
      if(nRead = -1){
      	if(errno == EAGAIN || errno == EWOULDBLOCK){
      		printf("The connection woulb be blocked!");
      		break;
      	}
      	return false;//否则表示读失败，返回false
      }
      else if(nRead == 0){
      	 return false;//客户端套接字已经关闭
      }
      //已读字节位置偏移
      CurrentReadNextPos += nRead;
   }
   return true;
}

//解析Http请求的请求行，返回状态码，用枚举类型变量
HttpConnection::HTTP_CODE HttpConnection::ParseRequestLine(char* text){
	printf("Parse the header of the request!");
	Url = strpbrk(text,"\t");
	if(Url == nullptr){
		return BAD_REQUEST;
	}
	*Url++ = '\0';//先使用后++,写入空字符

	char* method = text;
	if(strcasecmp(text, "GET" ) == 0){
		Method = GET;//GET请求，填充枚举类型变量
	}
	else{ //其他方法都返回错误请求，暂时只处理GET请求
		return BAD_REQUEST;
	}

	Url += strspn(Url," \t");
	Version = strpbrk(Url," \t");
	if(!Version){
       return BAD_REQUEST;//错误请求
	}


   
}

void HttpConnection::Process(){
	HTTP_CODE read_ret = ProcessRead();//处理读请求，获取状态码
	if(read_ret == NO_REQUEST){
		ModifyFd(Epollfd,SockFd,EPOLLIN);
		return;
	} 
	bool write_ret = ProcessWrite();//处理写请求
	if(!write_ret){
		CloseConnection();
	}
	
}