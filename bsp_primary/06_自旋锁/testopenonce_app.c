/*原子操作的app
 *
 * */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "mychar.h"
#include <sys/ioctl.h>
int main(int argc,char *argv[]){//mychat模块插入成功写一个app来验证一下
	int fd=-1;

	if(argc<2){
		printf("The argument is too few\n");
		//return -1;
		return 1;
	}
	fd=open(argv[1],O_RDONLY);
	if(fd<0){
		printf("open %s failed\n",argv[1]);
		//perror("open");
		//exit(1);
		return 2;
	}
	while(1){
	
	}
	close(fd);
	fd=-1;
	return 0;
}

