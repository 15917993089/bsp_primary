/*注意：只有初始化和退出的时候会使用到全局变量pgmydev
 *5.原子操作：原子变量
 *一个设备一次只能被访问一次，只能被打开一次
 * 1：表示设备打开
 * 0：设备不可打开
 *9 增加秒设备a
 *11. 编写led设备驱动
 * myled_ioctl怎么使用呢？
 * 12.编写设备树版驱动程序
 * (指针很有用，结构体和结构体变量都很有用)
 * */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/of_gpio.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include "leddrv.h"
int major=11;
int minor=0;
int myled_num=1;
//11.4 表示led这个字符设备
struct myled_dev{
	struct cdev mydev;
	//12.6 对编号进行操作
	unsigned int led2gpio;
	unsigned int led3gpio;
	unsigned int led4gpio;
	unsigned int led5gpio;

};
struct myled_dev *pgmydev=NULL;//全局变量就只使用在入口函数这里，如init等等！
int myled_open(struct inode *pnode,struct file *pfile){//要使与设备相关的几个函数不使用全局变量！例如：read阿write啊！
						       //所以要在open函数里边做手脚
	pfile->private_data=(void *)(container_of(pnode->i_cdev,struct myled_dev,mydev));//这样就可以获得gmydev的地址，就避免了访问全局变量(设置回去)
	return 0;
}
int myled_close(struct inode *pnode,struct file *pfile){
	return 0;
} 
//11.13 需要pmydev传递设备信息
void led_on(struct myled_dev *pmydev,int ledno){
	switch(ledno){
		case 2:
			gpio_set_value(pmydev->led2gpio,1);
			break;
		case 3:
			gpio_set_value(pmydev->led3gpio,1);
			break;
		case 4:
			gpio_set_value(pmydev->led4gpio,1);
			break;
		case 5:
			gpio_set_value(pmydev->led5gpio,1);
			break;

	}
}
void led_off(struct myled_dev *pmydev,int ledno){
	switch(ledno){
		case 2:
			gpio_set_value(pmydev->led2gpio,0);
			break;
		case 3:
			gpio_set_value(pmydev->led3gpio,0);
			break;
		case 4:
			gpio_set_value(pmydev->led4gpio,0);
			break;
		case 5:
			gpio_set_value(pmydev->led5gpio,0);
			break;

	}
}


long myled_ioctl(struct file *pfile,unsigned int cmd,unsigned long arg){
	//11.12
	struct myled_dev *pmydev = (struct myled_dev *)pfile->private_data;
	if(arg < 2 || arg > 5){
		return -1;
	}
	switch(cmd){
		case MY_LED_ON:
			//调用开灯函数
			led_on(pmydev,arg);
			break;
		case MY_LED_OFF:
			//调用关灯函数
			led_off(pmydev,arg);
			break;
		default:
			return -1;
	}
	return 0;
}
struct file_operations myops = {//类型管理对象
	.owner=THIS_MODULE,
	.open=myled_open,
	.release=myled_close,
	.unlocked_ioctl=myled_ioctl,
};
//12.6 获取编号
void request_leds_gpio(struct myled_dev *pmydev,struct device_node *pnode){
	pmydev->led2gpio=of_get_named_gpio(pnode, "led2-gpio",0);//0表示设备树中该对应的属性只有一组属性，第0个
	//12.7 向内核请占用
	gpio_request(pmydev->led2gpio,"led2"); 
	
	pmydev->led3gpio=of_get_named_gpio(pnode, "led3-gpio",0);
	gpio_request(pmydev->led3gpio,"led3"); 
	
	pmydev->led4gpio=of_get_named_gpio(pnode, "led4-gpio",0);
	gpio_request(pmydev->led4gpio,"led4"); 
	
	pmydev->led5gpio=of_get_named_gpio(pnode, "led5-gpio",0);
	gpio_request(pmydev->led5gpio,"led5"); 
}
void set_leds_gpio_output(struct myled_dev *pmydev){
	//11.10 设置为输出
	//11.11 同时将引脚预设为低电平
	//12.11
	gpio_direction_output(pmydev->led2gpio,0);
	gpio_direction_output(pmydev->led3gpio,0);
	gpio_direction_output(pmydev->led4gpio,0);
	gpio_direction_output(pmydev->led5gpio,0);
}
void free_leds_gpio(struct myled_dev *pmydev){
	gpio_free(pmydev->led2gpio);	
	gpio_free(pmydev->led3gpio);	
	gpio_free(pmydev->led4gpio);	
	gpio_free(pmydev->led5gpio);	
}
int __init myled_init(void) 
{ 
	int ret=0;
	dev_t devno=MKDEV(major,minor);
	//12.9 
	struct device_node *pnode=NULL;
	pnode=of_find_node_by_path("/fs4412-leds");
	if(pnode==NULL){
		printk("find node by path failed\n");
		return -1;
	}


	/* 申请设备号 */
	ret=register_chrdev_region(devno,myled_num,"myled");//申请设备号
	if(ret){//返回为1就出错！
		ret=alloc_chrdev_region(&devno,minor,myled_num,"myled");
		if(ret){
			printk("Get devno failed\n");
			return -1;
		}
		major=MAJOR(devno);//注意！容易遗漏
	}
	//11.1 在注册前对它进行动态分配
	pgmydev=(struct myled_dev *)kmalloc(sizeof(struct myled_dev),GFP_KERNEL);
	if(NULL==pgmydev){
		//11.2
		unregister_chrdev_region(devno,myled_num);//将设备号归还给系统
		printk("kmalloc for myled_dev failed\n");
		return -1;
	}
	//11.3
	memset(pgmydev,0,sizeof(struct myled_dev));

	/* 给struct cdev对象指定操作函数集 */
	cdev_init(&pgmydev->mydev,&myops);//这样初始化就完成了
	/* 将struct cdev对象添加到内核对应的数据结构里 */
	pgmydev->mydev.owner=THIS_MODULE;
	cdev_add(&pgmydev->mydev,devno,myled_num);//将我们的对象加入到我们内核管理设备的哈希链表里边去
	//11.5 ioremap
	//ioremap_ledreg(pgmydev);
	//12.8
	request_leds_gpio(pgmydev,pnode);


	//11.6 con-register set output
	//set_output_ledconreg(pgmydev);
	//12.11
	set_leds_gpio_output(pgmydev);

	//12.1获取gpio编号
	//of_get_named_gpio(struct device_node *np, const char *propname, int index);
	//12.2调用某个编号
	
	//12.3struct leddev结构体中记录所有用到的GPIO编号
	//
	//12.4向内核申请
	//gpio_request(unsigned gpio,const char *label);

	return 0;
}
void __exit myled_exit(void)
{
	dev_t devno=MKDEV(major,minor);//注销设备号
	//11.8 iounmap退出前解除映射
	//iounmap_ledreg(pgmydev);
	//12.10
	//gpio_free(unsigned gpio);
	free_leds_gpio(pgmydev);
	
	cdev_del(&pgmydev->mydev);//退出时从内核的哈希链表中删除
	unregister_chrdev_region(devno,myled_num);//将设备号归还给系统
	printk("myled will exit\n");
	
	//12.5
	//gpio_free(unsigned gpio);

	//11.7
	kfree(pgmydev);
	pgmydev=NULL;
}


MODULE_LICENSE("GPL");
//MODULE_AUTHOR("曾东城");
//MODULE_DESCRIPTION("It is only a simple test.");
//MODULE_ALIAS("HI");

module_init(myled_init);
module_exit(myled_exit);
