#include <stdio.h>
#include <iostream>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc,char* argv[]){
    //校验命令行参数是否正确
    if(argc < 2){
       printf("usage:%s ip_address port_number!\n",argv[0]);
    }
    char* serv_ip = argv[1];//服务器IP地址，从命令行参数获取，注意使用const char* 保存
    int port = atoi(argv[2]);//服务器端口号
    int limit = atoi(argv[3]);// 发送消息的上限次数，从命令行参数获得

	int sockfd;
 	int len;
 	struct sockaddr_in address;
 	int result;
 	char text[20] = "Hello World!";
 	char receive[20];
 	sockfd = socket(AF_INET, SOCK_STREAM, 0);//创建流套接字
 	//填充服务器套接字地址结构体
 	address.sin_family = AF_INET;
 	if (inet_ntop(AF_INET, &address.sin_addr, serv_ip, sizeof(serv_ip)) == NULL) {
        printf("inet_ntop error\n");//转换IP地址
        close(sockfd);//关闭套接字
        return 0;
    }

 	address.sin_port = htons(port);
    //套接字地址长度
    len = sizeof(address);

    //连接服务器
    result = connect(sockfd, (struct sockaddr *)&address, len);
    if(result == -1){
       perror("connect the server fail!\n");
       exit(0);
    }
    //与服务器通信
    int count = 0;
    bool stopped = false;
    while(count < limit && !stopped){
        write(sockfd,text,sizeof(text));//写数据
        printf("write data to the server:%s",text);
        int n = read(sockfd,receive,sizeof(receive));//接收服务器传送过来的数据
        if(n > 0){ //读到数据
            printf("receive data from the server:%s\n",receive);
        }
        else if(n == 0){ //服务器已经断开连接或者socket已经断开
            printf("Connection is interrupted!Please try to connect the server again!\n");
            stopped = true;
        }  
        else{ //连接出错
            printf("The connection failed!The client is stopped!\n");
            stopped = true;
        }
        count++;//通信次数加1
        sleep(1);
    }
    //结束通信
    sleep(1);
    close(sockfd);
    printf("times:%d\n",count);
    printf("\nThe client is going to disconnect!\n");
    return 0;
}