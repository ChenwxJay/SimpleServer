#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc,char* argv[]){
	int sockfd;
 	int len;
 	struct sockaddr_in address;
 	int result;
 	char text[20] = "Hello World!\n";
 	char receive[20];
 	sockfd = socket(AF_INET, SOCK_STREAM, 0);//创建流套接字
 	//填充服务器套接字地址结构体
 	address.sin_family = AF_INET;
 	address.sin_addr.s_addr = inet_pton(argv[1]);//使用命令行参数
 	int port = atoi(argv[2]);//端口号
 	address.sin_port = htons(port);
    //套接字地址长度
    len = sizeof(address);

    //连接服务器
    result = connect(sockfd, (struct sockaddr *)&address, len);
    if(result == -1){
       perror("connect the server fail!");
       exit(0);
    }
    //与服务器通信
    write(sockfd,text,sizeof(text));//写数据
    printf("write data:%s",text);
    read(sockfd,receive,sizeof(receive));//接收服务器传送过来的数据
    printf("receive data:%s",receive);
    //结束通信
    close(sockfd);
    printf("The client is going to disconnect");
    return 0;
}