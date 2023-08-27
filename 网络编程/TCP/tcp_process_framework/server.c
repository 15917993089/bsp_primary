#include "net.h"
#include <pthread.h>
#include <signal.h>

void client_data_handle(int *arg);
void signal_child_handle(int sig){
    if(SIGCHLD == sig){
        waitpid(-1, NULL, WNOHANG);//采用非阻塞的方式
    }
}
int main(int argc, char **argv){
    int fd = -1;
    struct sockaddr_in sin;
    
    signal(SIGCHLD, signal_child_handle);//子进程结束时发给父进程的信号

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("socket");
        return 0;
    }
    //允许地址快速重用
    int b_reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &b_reuse, sizeof(int));

    bzero(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(SERVER_PORT);

    //绑定在任意的ip地址上
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
   

    /* bind listen accept read/write */
    if((bind(fd, (struct sockaddr *)&sin, sizeof(sin)))  < 0){//转化为通用型
        perror("bind");
        return 0;
    }
    if((listen(fd, BACKLOG)) < 0){
        perror("listen");
        return 0;
    }
    printf("Server start...............OK!\n");


    //利用多进程对客户端进行连接
    int newfd = -1;
    struct sockaddr_in cin;
    socklen_t addrlen = sizeof(cin);
    while(1){
        pid_t pid;
        //与客户端建立连接，并获取客户端的端口信息
        if((newfd = accept(fd,(struct sockaddr *)&cin, &addrlen)) < 0){
            perror("accept");
            return 0;
        }
        //客户端连接成功，创建进程
        if((pid = fork()) < 0){
            perror("fork");
            break;//先退出循环后面退出程序
        }else if(pid == 0){
            close(fd);
            char ipv4_addr[16];//两个字节
            //打印客户端ip
            if(!(inet_ntop(AF_INET, (void *)&cin.sin_addr.s_addr, ipv4_addr, sizeof(cin)))){//将二进制网络字节序转化为本地点分ip
                perror("inet_ntop");
                return 0;
            }
            printf("Client(%s: %d) is conneted...........\n",ipv4_addr, ntohs(cin.sin_port));//将网络字节序转化为本地字节序
            //再开一个进程/线程对数据进行处理
            client_data_handle(&newfd);
            return 0;//结束返回return一个信号，信号被信号处理函数接收，回收子进程

        }else{//父进程要回收子进程，通过信号的方式进行处理
            close(newfd);
        }
        close(newfd);
    }

    close(fd);
    return 0;
}

void client_data_handle(int *arg){
    int newfd = *arg;
    int ret;
    char buf[BUFSIZ];
    printf("Child handler process: newfd = %d\n", newfd);

    //对客户端的数据进行处理
    while(1){
        bzero(buf, BUFSIZ);
        do{
            ret = read(newfd, buf, BUFSIZ - 1);//防止越界
        }while(ret < 0 && EINTR == errno);//中断引起继续
        if(ret < 0){
            perror("read");//还是小于0就是错误了
            exit(1);
        }
        if(!ret){//客户端结束
            break;
        }
        printf("Receive data: %s\n", buf);
        if(!(strncasecmp(buf, QUIT_STR, strlen(QUIT_STR)))){
            printf("Client(fd = %d) is exited...............\n", newfd);
            break;
        }

    }
    close(newfd);

}