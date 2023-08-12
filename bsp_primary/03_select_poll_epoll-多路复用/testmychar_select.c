#include <stdio.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "mychar.h"
#include <sys/ioctl.h>
int main(int argc,char *argv[]){//mychat模块插入成功写一个app来验证一下
	int fd=-1;
	char buf[8]="";
	//int max=0;
	//int cur=0;
	int ret=0;
	fd_set rfds;//重点在这里

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
	while(1){
		FD_ZERO(&rfds);
		FD_SET(fd,&rfds);
		ret=select(fd+1,&rfds,NULL,NULL,NULL);//只是检查有没有数据可读
		if(ret<0){
			if(errno==EINTR){
				continue;//由信号（中断）引起的错误继续
			}else{
				printf("Select error\n");
				break;//这才是真正的出错！
			}
		}
		if(FD_ISSET(fd,&rfds)){
			read(fd,buf,8);
			printf("buf=%s\n",buf) ;
		}
	}
	close(fd);
	fd=-1;
	return 0;
}

