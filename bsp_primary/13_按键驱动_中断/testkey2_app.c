#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <stdio.h>

#include "fs4412_key.h"

int main(int argc,char *argv[])
{
	int fd = -1;
	struct keyvalue keydata = {0};
	int ret = 0;

	if(argc < 2)
	{
		printf("The argument is too few\n");
		return 1;
	}

	fd = open(argv[1],O_RDONLY);
	if(fd < 0)
	{
		printf("open %s failed\n",argv[1]);
		return 3;
	}

	while((ret = read(fd,&keydata,sizeof(keydata))) == sizeof(keydata))
	{
		if(keydata.status == KEY_DOWN)
		{
			printf("Key2 is down!\n");
		}
		else
		{
			printf("Key2 is up!\n");
		}
	}

	close(fd);
	fd = -1;
	return 0;
}
