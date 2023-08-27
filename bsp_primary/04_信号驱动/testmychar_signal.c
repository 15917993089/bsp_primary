#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "mychar.h"
#include <sys/ioctl.h>
//4.1
int fd=-1;
//4.2
void sigio_handler(int sig){
	
	char buf[8]="";
	read(fd,buf,8);
	printf("buf=%s\n",buf);
}

int main(int argc,char *argv[]){//mychat模块插入成功写一个app来验证一下
	//int max=0;
	//int cur=0;
	
	int ret=0;
	int flg=0;
	//4.3
	signal(SIGIO,sigio_handler);
	if(argc<2){
		printf("The argument is too few\n");
		//return -1;
		return 1;
	}
	fd=open(argv[1],O_RDWR);//无需O_NONBLOCK,默认就是阻塞
	if(fd<0){
		printf("open %s failed\n",argv[1]); 
		//perror("open");
		//exit(1);
		return 2;
	}
	fcntl(fd,F_SETOWN,getpid());//4.4将描述符设置给对应进程，好由描述符获知PID
	flg=fcntl(fd,F_GETFL);
	flg |= FASYNC;
	fcntl(fd,F_SETFL,flg);
	while(1){//为了防止在收到信号之前程序一直往下跑！
	
	}
	

	//ioctl(fd,MYCHAR_IOCTL_GET_MAXLEN,&max);//文件打开之后
	//printf("max len is %d\n",max);
	//write(fd,"Hello",6);
	//ioctl(fd,MYCHAR_IOCTL_GET_CURLEN,&cur);//文件写完之后
	//printf("cur len is %d\n",cur);
/*
	ret=read(fd,buf,8);
	if(ret<0){
		printf("read data failed\n");
		return -1;
	}else{
		printf("read buf=%s\n",buf);

	}
*/

	close(fd);
	fd=-1;
	return 0;
}

