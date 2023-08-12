#ifndef __MYCHAR_H__
#define __MYCHAR_H__
#include <asm/ioctl.h>

#define MY_CHAR_MAGIC 'a'
//为了在应用层里面获得最大值和当前值
#define MYCHAR_IOCTL_GET_MAXLEN _IOR(MY_CHAR_MAGIC,1,int *)
#define MYCHAR_IOCTL_GET_CURLEN _IOR(MY_CHAR_MAGIC,2,int *)


#endif

