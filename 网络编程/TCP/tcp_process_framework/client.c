/* client SREVER_IP SERVER_PORT */
#include "net.h"
void usage(char *gv){
    printf("\n%s server_ip server_port",gv);
    printf("\n\t server_ip: server ip address");
    printf("\n\t server_port: server port (> 5000)\n\n");
}

int main(int argc, char **argv)
{
    int port = -1;
    int fd = -1;
    int ret = 0;
    if(argc != 3){
        usage(argv[0]);
        return 0;
    }
    //填充套接字结构体
    struct sockaddr_in sin;
    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
        return 0;
    }
    port = atoi(argv[2]);
    if(port < 5000){
        usage(argv[0]);
        return 0;
    }
    bzero(&sin, sizeof(sin));

    sin.sin_family = AF_INET;//ipv4 
    sin.sin_port = htons(port);//两个字节，将本地字节序转化为网络字节序
    //将点分形式的ip地址转化为二进制网络字节序
    if((inet_pton(AF_INET, argv[1], (void *)&sin.sin_addr.s_addr)) != 1){
        perror("inet_pton");
        return 0;
    }
    //填充完结构体建立连接
    if((connect(fd, (struct sockaddr *)&sin, sizeof(sin))) < 0){
		perror("connet");
		exit(1);
	}

    printf("Client start .................OK!\n");

    char buf[BUFSIZ];
    //从键盘上获取数据
    while(1){
        bzero(buf, BUFSIZ);
        if((fgets(buf, BUFSIZ - 1, stdin)) == NULL){
            continue;
        }
        do{
            ret = write(fd, buf, strlen(buf));//写到建立链接的fd,也就是想向服务器写内容
        }while(ret < 0 && EINTR == errno);//由中断引起的继续写
        //提示客户端退出
        if(!(strncasecmp(buf, QUIT_STR, strlen(QUIT_STR)))){
            printf("Client is exited..........\n");
            break;//退出客户端
        }
    }
    
    close(fd);

    return 0;
}