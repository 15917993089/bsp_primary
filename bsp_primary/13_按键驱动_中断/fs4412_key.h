/*编写结构体表示按键的键值和状态
 *
 * */
#ifndef FS4412_KEY_H
#define FS4412_KEY_H

enum KEYCODE{
	KEY2 = 1002;
	KEY3;
	KEY4;	
};

enum KEY_STATUS{
	KEY_DOWN = 0;
	KEY_UP;
};
//建立一个结构体表示按键的数据
struct keyvalue{
	int code;//while KEY
	int status;//	
};

#endif

