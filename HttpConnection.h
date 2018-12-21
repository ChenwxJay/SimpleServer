#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

class HttpConnection{
private:
	//文件名最大长度
	static const int FILENAME_MAX_LEN = 200;

	//读缓冲区大小
	static const int READ_BUFFER_SIZE = 1024;

	//写缓冲区大小
	static const int WRITE_BUFFER_SIZE = 1024;

	//所有的socket上的事件都注册到同一个epoll fd上，因此定义为静态的
	static int EpollFd;
	//http请求方法
	enum{ //使用枚举类型来来表示
		GET = 0,
		POST = 1,
		PUT = 2,
		HEAD
		DELETE,
        TRACE
	};
	//服务器解析http请求时所处的状态
	enum{
	   CHECK_REQUESTLINE = 0,//正在检查请求行
	   CHECK_HEADER = 1,//正在检查头部
	   CHECK_CONTENT = 2//正在检查内容
	};
	//
public:
	HttpConnection(){ } //默认构造函数
	~HttpConnection(){ }
    //初始化新接受的连接
    void Init(int sockfd,const sockaddr_in& sockaddr);
    //关闭连接
    void CloseConnection(bool real_close = true);//使用函数参数默认参数
    //处理客户请求
    void Service();
    //非阻塞读操作
    void Read();
    //非阻塞写操作
    void Write();
private:
	//被公有方法Int调用
    void Init();
    //解析Http请求
    HTTP_CODE ProcessRead();
    //填充HTTP响应
    bool ProcessWrite(HTTPCODE data);

    //设计一组函数，用来解析HTTP请求
    //解析请求头
    HTTP_CODE ParseRequestLine(string& text);
    //解析头部
    HTTP_CODE ParseHeaders(string& text);
    //解析内容
    HTTP_CODE ParseContent(string& text);
    
    //类的成员
    int SockFd; //http连接的socket描述符
    struct sockadde_in Address; //对端套接字地址

    //读缓冲区
    char ReadBuffer[BUFFER_MAX_SIZE];
    //标识读缓冲区中已经进入的客户端数据的最后一个字节的下一个位置
    int CurrentReadNextPos;
    //当前正在解析的字节在缓冲区中的位置
    int CurrentCheckedPos;
    //当前正在解析的行的相对位置
    int CurrentStartLine;

    //写缓冲区，使用字符数组实现
    char WriteBuffer[BUFFER_MAX_SIZE];
    //写缓冲待发送的字节数
    int  ToBeSendDataSize;
   
    CHECK_STATE CheckState;//检查状态
    //客户请求的目标文件的完整路径
    char FileName[FILENAME_MAX_LEN];
    //客户请求的目标文件名

    //该连接是否处于keep-alive状态
    bool Linger;
    //Http请求头部字段内容(如果有)
    long ContentLength;

}