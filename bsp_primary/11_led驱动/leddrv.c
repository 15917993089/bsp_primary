/*注意：只有初始化和退出的时候会使用到全局变量pgmydev
 *5.原子操作：原子变量
 *一个设备一次只能被访问一次，只能被打开一次
 * 1：表示设备打开
 * 0：设备不可打开
 *9 增加秒设备a
 *11. 编写led设备驱动
 * myled_ioctl怎么使用呢？
 * */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include "leddrv.h"
//11.9 宏定义地址值，通过芯片手册获得
#define GPX1CON 0X11000C20
#define GPX1DAT 0x11000C24
#define GPX2CON 0X11000C40
#define GPX2DAT 0x11000C44
#define GPF3CON 0x114001E0
#define GPF3DAT 0x114001E4
int major=11;
int minor=0;
int myled_num=1;
//11.4 表示led这个字符设备
struct myled_dev{
	struct cdev mydev;
	//11.4 volatile修饰的只想空间的访问不做任何优化，可阻止cpu优化：
	//正常使用内存，编译器会帮代码做优化，回去看看cpu里面的寄存器
	//有没有空闲，有空闲会把内存空间里面的值读到寄存器里面去，
	//方便下一次访问的时候更快,但是对于一级外设控制器里面的寄存器进行访问，
	//每次读都要读寄存器里面的值，做了优化之后有可能会从cpu的寄存器里面读
	//而不是从一级外设控制器里面读，也就是内存空间里面的变量
	//可能会被意外改变，所以会导致内存和寄存器的值不一致，
	//确保你每次都从内存读到最新的值
	//加了volatile会阻止优化了。
	volatile unsigned long *pled2_con;
	volatile unsigned long *pled2_dat;

	volatile unsigned long *pled3_con;
	volatile unsigned long *pled3_dat;

	volatile unsigned long *pled4_con;
	volatile unsigned long *pled4_dat;

	volatile unsigned long *pled5_con;
	volatile unsigned long *pled5_dat;

#if 0
void __iomem * pled2_con;
void __iomem * pled2_dat;
void __iomem * pled3_con;
void __iomem * pled3_dat;
void __iomem * pled4_con;
void __iomem * pled4_dat;
void __iomem * pled5_con;
void __iomem * pled5_dat;
#else
	
#endif

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
//11.13 需要pmydev传递设备信息,可以访问到设备的地址值
void led_on(struct myled_dev *pmydev,int ledno){
	switch(ledno){
		case 2:
			//把某一位置1：位或
			writel(readl(pmydev->pled2_dat) | (0x1 << 7),pmydev->pled2_dat);
			break;
		case 3:
			writel(readl(pmydev->pled3_dat) | (0x1),pmydev->pled3_dat);
			break;
		case 4:
			writel(readl(pmydev->pled4_dat) | (0x1 << 4),pmydev->pled4_dat);
			break;
		case 5:
			writel(readl(pmydev->pled5_dat) | (0x1 << 5),pmydev->pled5_dat);
			break;

	}
}
void led_off(struct myled_dev *pmydev,int ledno){
	switch(ledno){
		case 2:
			//把某一位置0：位与
			writel(readl(pmydev->pled2_dat) & (~(0x1 << 7)),pmydev->pled2_dat);
			break;
		case 3:
			writel(readl(pmydev->pled3_dat) & ((0x1)),pmydev->pled3_dat);
			break;
		case 4:
			writel(readl(pmydev->pled4_dat) & (-(0x1 << 4)),pmydev->pled4_dat);
			break;
		case 5:
			writel(readl(pmydev->pled5_dat) & (~(0x1 << 5)),pmydev->pled5_dat);
			break;

	}
}


long myled_ioctl(struct file *pfile,unsigned int cmd,unsigned long arg){
	//11.12
	//struct file里面的私有数据存储着设备信息
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
//11.5.1 传入设备地址
void ioremap_ledreg(struct myled_dev *pmydev){
	pmydev->pled2_con=ioremap(GPX2CON,4);
	pmydev->pled2_dat=ioremap(GPX2DAT,4);
	pmydev->pled3_con=ioremap(GPX1CON,4);
	pmydev->pled3_dat=ioremap(GPX1DAT,4);
	pmydev->pled4_con=ioremap(GPF3CON,4);
	pmydev->pled4_dat=ioremap(GPF3DAT,4);
	pmydev->pled5_con=pmydev->pled4_con;
	pmydev->pled5_dat=pmydev->pled4_dat;
}
void set_output_ledconreg(struct myled_dev *pmydev){
	//11.10 设置为输出
	writel((readl(pmydev->pled2_con) & (~(0xF<<28))) | (0x1<<28),pmydev->pled2_con);//读出来再写回去
	writel((readl(pmydev->pled3_con) & (~(0xF))) | (0x1),pmydev->pled3_con);
	writel((readl(pmydev->pled2_con) & (~(0xF<<16))) | (0x1<<16),pmydev->pled4_con);
	writel((readl(pmydev->pled2_con) & (~(0xF<<20))) | (0x1<<20),pmydev->pled5_con);
	//11.11 同时将引脚预设为低电平
	writel(readl(pmydev->pled2_dat) & (~(0x1 << 7)),pmydev->pled2_dat);
	writel(readl(pmydev->pled3_dat) & ((0x1)),pmydev->pled3_dat);
	writel(readl(pmydev->pled4_dat) & (-(0x1 << 4)),pmydev->pled4_dat);
	writel(readl(pmydev->pled5_dat) & (~(0x1 << 5)),pmydev->pled5_dat);
}
void iounmap_ledreg(struct myled_dev *pmydev){
	iounmap(pmydev->pled2_con);
	pmydev->pled2_con = NULL;
	iounmap(pmydev->pled2_dat);
	pmydev->pled2_dat = NULL;

	iounmap(pmydev->pled3_con);
	pmydev->pled3_con = NULL;
	iounmap(pmydev->pled3_dat);
	pmydev->pled3_dat = NULL;

	iounmap(pmydev->pled4_con);
	pmydev->pled4_con = NULL;
	iounmap(pmydev->pled4_dat);
	pmydev->pled4_dat = NULL;

	pmydev->pled5_con = NULL;
	pmydev->pled5_dat = NULL;
}
int __init myled_init(void) 
{ 
	int ret=0;
	dev_t devno=MKDEV(major,minor);
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
	ioremap_ledreg(pgmydev);
	//硬件操作
	//11.6 con-register set output
	set_output_ledconreg(pgmydev);
	return 0;
}
void __exit myled_exit(void)
{
	dev_t devno=MKDEV(major,minor);//注销设备号
	//11.8 iounmap退出前解除映射
	iounmap_ledreg(pgmydev);

	cdev_del(&pgmydev->mydev);//退出时从内核的哈希链表中删除
	unregister_chrdev_region(devno,myled_num);//将设备号归还给系统
	printk("myled will exit\n");

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

