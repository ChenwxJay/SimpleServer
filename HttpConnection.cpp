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
//由线程池中的工作线程调用，这是处理Http请求的入口函数
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
	ModifyFd(Epollfd,SockFd,EPOLLIN);
}

bool HttpConnection::ProcessWrite(){
	
}

//主状态机，处理Http请求，需要调用其他函数
HttpConnection::HTTP_CODE HttpConnection::ProcessRead(){
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE ret = NO_REQUEST;//初始化状态码，目前正常
	char* text = nullptr;

	//循环处理请求,正处于解析内容状态或者正在解析请求行
	while((CheckState == CHECK_STATE_CONTENT) && (line_status == LINE_OK) ||
		(line_status = ParseLine()) == LINE_OK)
	{  
        text = GETLine();//获取请求行
        CurrentStartLine = CurrentCheckedPos;//更新位置
        printf("get a http line: %s\n",text);

       //根据当前所处状态，实现状态机
        switch(CheckState){
        	case CHECK_REQUESTLINE:
            {
            	ret = ParseRequestLine(text);//当前处于解析请求行状态，调用函数
            	if(ret == BAD_REQUEST){ //出错
            		return BAD_REQUEST;//直接错误返回
            	}
            	break;
            }
            case CHECK_HEADER:
            {
            	ret = ParseHeaders(text);//解析Http请求头信息
            	if(ret == BAD_REQUEST){ //出错
            		return BAD_REQUEST;//直接错误返回
            	}
            	else if(ret = GET_REQUEST) //请求头部分析完成，处理请求
            	{
            		return HandleRequest();//处理请求
            	}	
            	break;
            }
            case CHECK_CONTENT:
            {
            	ret = ParseContent(text);//解析Http请求内容
            	if(ret == GET_REQUEST){
            		return HandleRequest();//处理请求
            	}
            	line_status = LINE_OPEN;//设置行状态
            	break;
            }
            default:{
            	return INTERNAL_ERROR;
            }
        }
	}
	return NO_REQUEST;//正常执行
}
HttpConnection::LINE_STATUS HttpConnection::ParseRequestLineLine(char* text){
   
}
//解析Http请求头部字段函数
HttpConnection:: HTTP_CODE HttpConnection::ParseHeaders(char* text){
	if(text[0] == '\0'){
		//遇到空行，表示头部字段解析完毕

		//如果Http请求有消息体，则需要继续读取
		if(ContentLength != 0){
			CheckState = CHECK_STATE_CONTENT;//状态机切换到解析内容状态
			return NO_REQUEST;//直接返回
		}
		return GET_REQUEST;//否则表示已经获得了一个完整的Http请求
	}
	else if(strncasecmp(text,"Connection:",11) == 0){
		//调用strncasecmp函数进行比较，该函数的第三个参数是需要比较前n个字符
		text += 11;
		text += strspn(text," \t");//作用？
		if(strncasecmp(text,"keep-alive") == 0){ //长连接，开启so_linger选项
            Linger = true;
		}
	}
	else if(strncasecmp(text,"Content-Length:",15) == 0){
		//解析Content-Length字段
		text += 15;
		text += strspn(text," \t");//作用？
		ContentLength = atol(tetx);//剩下的字符串转化为整数，long类型
	}
	else if(strncasecmp(text,"Host:",5) == 0){
		text += 5;
		text += strspn(text," \t");
        Host = text; //获取请求的主机字段
	}
	else{
		printf("Unknown Header!");
	}
	return NO_REQUEST;
}
HttpConnection::HTTP_CODE HttpConnection::ParseContent(char* text){
	//暂时不解析Http请求的消息体
	if(CurrentReadNextPos >= (ContentLength + CurrentCheckedPos)){
		text[ContentLength] = '\0';//空字符结尾，后面的字符串无效
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

bool HttpConnection::ProcessWrite(HTTP_CODE ret){
    switch(ret)
    {
       case INTERNAL_ERROR://服务器内部错误，返回500错误
       {
       	  AddStatusLine(500,Error_500_Title);//添加状态行信息
       	  AddHeaders(strlen(Error_500_Form));//添加头部信息
          if(! AddContent(Error_500_Form)){
          	   return false;//添加失败，返回false
          }	  
          break;
       }
       case BAD_REQUEST: //请求出错，返回400错误
       {
          AddStatusLine(400,Error_400_Title);//添加状态行信息
          AddHeaders(strlen(Error_400_Form));//添加头部信息
          if(! AddContent(Error_400_Form)){
          	   return false;//添加失败，返回false
          }	  
          break;
       }
       case NO_RESOURCE:
       {
       	 AddStatusLine(404,Error_404_Title);//添加状态行，404
       	 AddHeaders(strlen(Error_404_Form));
       	 if(!AddContent(Error_404_Form)){ //添加404页面内容，并判断是否成功
              return false;
       	 }
       	 break;
       }
       case FORBIDDEN_REQUEST:
       {
       	 AddStatusLine(404,Error_403_Title);//添加状态行，403
       	 AddHeaders(strlen(Error_403_Form));
       	 if(!AddContent(Error_403_Form)){ //添加403页面内容，并判断是否成功
              return false;
       	 }
       	 break;
       }
       case FILE_REQUEST:
       {
         AddStatusLine(200,Ok_200_Title);//添加状态行，200
         if(FileStat.size != 0){
         	AddHeaders(FileStat.size);
         	Iv[0].iov_base = WriteBuffer;//写入数据
         	Iv[0].iov_len = WritePos;
         	Iv[1].iov_base = FileAddress;
         	Iv[1].iov_len = FileStat.size;
         	IvCount = 2;//写入2个
         	return true; 
         }
         else{
         	//定义一个html字符串
         	const char* ok_string = "<html><head></head><body></body></html>";
         	AddHeaders(strlen(ok_string));//添加头部信息
         	if(!AddContent(ok_string)){
         		return false;
         	}
         }
       }
       default:
       {
       	 return false;
       }
    }
     Iv[0].iov_base = WriteBuffer;//填充缓冲区
     Iv[0].iov_len = WritePos;
     IvCount = 1;//写1个
     return true; 
}
bool HttpConnection::Write(){
	int temp = 0;
	int bytes_to_send = WritePos;
	int bytes_have_send = 0;
	if(bytes_to_send == 0){
		ModifyFd(EpollFd,SockFd,EPOLLIN);//修改文件描述符状态
		Init();//重新初始化
		return true;
	}
	//死循环
	while(true){
		temp = writev(SockFd,Iv,IvCount);
		if(temp <= -1){
			if(errno == EAGAIN){
				//当前TCP缓冲区没有可写空间
				ModifyFd(EpollFd,SockFd,EPOLLOUT);//更改sockfd状态，等待下一个的可写事件
				return true;
			}
			unmap();//解除映射
			return false;
		}
	    bytes_to_send -= temp;//已经写了temp个字节，减去
	    bytes_have_send += temp;//增加已写字节数
	    if(bytes_to_send <= bytes_have_send){ //发送Http响应成功
	    	unmap();
	    	if(Linger)//根据Http请求的Connection字段决定是否立即关闭连接
	    	{
	    		Init();//重新初始化连接的状态
	    		ModifyFd(EpollFd,SockFd,EPOLLIN);//重新设置为输入监听
	    		return true;
	    	}
	    	else{
	    		ModifyFd(EpollFd,SockFd,EPOLLIN);//重新设置为输入监听
	    		return true;
	    	}
	    }
	}
}
//向写缓冲区中写入待发送的数据
bool HttpConnection::AddReponse(const char* format,...){
	if(WritePos >= WRITE_BUFFER_SIZE){ //TCP缓冲区已经满了
		return false;
	}
	va_list arg_list;//可变参数列表
	va_start(arg_list,format); //获取参数，字符串格式
	int len = vsnprintf(WriteBuffer + WritePos,WRITE_BUFFER_SIZE-1-WritePos,
		                format,arg_list);
	if(len >= (WRITE_BUFFER_SIZE-1-WritePos)){
		return false;
	}
	WritePos += len;
	va_end(arg_list);//结束获取
	return true;
}
bool HttpConnection::AddStatusLine(int status,const char* title){
	return AddReponse("%s %d %s\r\n","HTTP/1.1",status,title);//调用可变参数函数
}
bool HttpConnection::AddHeaders(int content_length){
	AddContentLength(content_length);//添加content_length字段
	AddLinger();//添加长连接选项
	AddBlankLine();//添加空行
}
//添加Content-Length字段
bool HttpConnection::AddContentLength(int content_length){
	return AddReponse("Content-Length: %d\r\n",content_length);//写入一行内容长度字段
}
//添加Connection字段
bool HttpConnection::AddLinger(){
	//根据当前Http连接的状态，决定是否使用长连接，并保存为字段
	return AddReponse("Connection: %s\r\n",(Linger == true?)"keep-alive":"close");
}
//添加空白行
bool HttpConnection::AddBlankLine(){
	return AddReponse("%s","\r\n");//使用\r\n来实现换行
}
//添加Http请求的Content字段
bool HttpConnection::AddContent(const char* text){
   return AddReponse("%s",text);
}
//解除映射
void HttpConnection::unmap(){
	if(FileAddress){
		munmap(FileAddress,FileStat.size);//解除文件映射
		FileAddress = 0;//地址清零
	}
}

//处理Http请求，分析
HttpConnection::HTTP_CODE HttpConnection::DoRequest(){
	strcpy(RealFile,doc_root);
	size_t len = strlen(doc_root);//文档根路径字符串长度
	//将请求url中的文件信息与文件根路径字符串进行拼接
	strncpy(RealFile,Url,FILENAME_MAX_LEN-len-1);
	if(stat(RealFile,&FileStat) < 0){
		//找不到资源
		return NO_RESOURCE;
	}
	//判断请求的文件是否为目录，如果为目录则返回BAD_REQUEST
	if(S_ISDIR(FileStat.st_mode)){ 
		return BAD_REQUEST;
	} 
	

}