/*
 *5.原子操作：原子变量
 *一个设备一次只能被访问一次，只能被打开一次
 * 1：表示设备打开
 * 0：设备不可打开
 *9 增加秒设备
 * */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

int major=11;
int minor=0;
int mysecond_num=1;

struct mysecond_dev{
	struct cdev mydev;
	//9.4
	int second;
	struct timer_list timer;
	//5.1
	atomic_t openflag;

};
struct mysecond_dev gmydev;//全局变量就只使用在入口函数这里，如init等等！

void timer_func(unsigned long arg){//9.7 一秒之后这个函数将会被调用
	struct mysecond_dev *pmydev=(struct mysecond_dev *)arg;//再强转回来！
	pmydev->second++;
	mod_timer(&pmydev->timer,jiffies+HZ*1);//每隔一秒被调用一次
	
}

int mysecond_open(struct inode *pnode,struct file *pfile){//要使与设备相关的几个函数不使用全局变量！例如：read阿write啊！
							//所以要在open函数里边做手脚
	struct mysecond_dev *pmydev=NULL;
	pfile->private_data=(void *)(container_of(pnode->i_cdev,struct mysecond_dev,mydev));//这样就可以获得gmydev的地址，就避免了访问全局变量(设置回去)
	pmydev=(struct mysecond_dev *)pfile->private_data;//要用到gmydev就加个这样的语句就好（取回来),就可以避免访问全局变量
	//5.3(9.3防止重复打开，这里已经做好了)
	if(atomic_dec_and_test(&pmydev->openflag)){
		//9.6
		pmydev->timer.expires=jiffies+HZ*1;
		pmydev->timer.function=timer_func;
		pmydev->timer.data=(unsigned long)pmydev;//9.7定时时间到了给函数传递的参数
		//9.8
		add_timer(&pmydev->timer);//把我们这个定时器加入到内核的定时器链表里边去
		return 0;
	}else{
		atomic_inc(&pmydev->openflag);//5.4 加1变为0
		printk("The device is open already\n");
		return -1;
	}
	printk("mysecond_open is called\n");
	return 0;
}
//9.2
ssize_t mysecond_read(struct file *pfile,char __user *puser,size_t size,loff_t *p_pos){
	struct mysecond_dev *pmydev=(struct mysecond_dev *)pfile->private_data;//要用到gmydev就加个这样的语句就好
	int ret;
	if(size<sizeof(int)){
		printk("The expect read size is invalid\n");
		return -1;
	}
	if(size>=sizeof(int)){
		size=sizeof(int);
	}
	//9.9
	ret=copy_to_user(puser,&pmydev->second,size);
	if(ret){//不为0拷贝失败
		printk("Copy to user failed\n");
		return -1;
	}
	return size; 
}
int mysecond_close(struct inode *pnode,struct file *pfile){
	struct mysecond_dev *pmydev=(struct mysecond_dev *)pfile->private_data;//要用到gmydev就加个这样的语句就好
	printk("mysecond_close is called\n");
	//9.10这样设备定时器就完成了
	del_timer(&pmydev->timer);
	atomic_set(&pmydev->openflag,1);
	return 0;
} 

struct file_operations myops = {//类型管理对象
	.owner=THIS_MODULE,
	.open=mysecond_open,
	.release=mysecond_close,
	//9.1
	.read=mysecond_read,
};

int __init mysecond_init(void) 
{ 
	int ret=0;
	dev_t devno=MKDEV(major,minor);
	/* 申请设备号 */
	ret=register_chrdev_region(devno,mysecond_num,"mysecond");//申请设备号
	if(ret){//返回为1就出错！
		ret=alloc_chrdev_region(&devno,minor,mysecond_num,"mysecond");
		if(ret){
			printk("Get devno failed\n");
			return -1;
		}
		major=MAJOR(devno);//注意！容易遗漏
	}
	/* 给struct cdev对象指定操作函数集 */
	cdev_init(&gmydev.mydev,&myops);//这样初始化就完成了
	/* 将struct cdev对象添加到内核对应的数据结构里 */
	gmydev.mydev.owner=THIS_MODULE;
	cdev_add(&gmydev.mydev,devno,mysecond_num);//将我们的对象加入到我们内核管理设备的哈希链表里边去
	//5.2将openflag置为1,表示开始可以打开
	atomic_set(&gmydev.openflag,1);
	//9.5
	init_timer(&gmydev.timer); 
	return 0;
}

void __exit mysecond_exit(void)
{
	dev_t devno=MKDEV(major,minor);//注销设备号
	cdev_del(&gmydev.mydev);//退出时从内核的哈希链表中删除
	unregister_chrdev_region(devno,mysecond_num);//将设备号归还给系统
	printk("mysecond will exit\n");
}


MODULE_LICENSE("GPL");
//MODULE_AUTHOR("曾东城");
//MODULE_DESCRIPTION("It is only a simple test.");
//MODULE_ALIAS("HI");

module_init(mysecond_init);
module_exit(mysecond_exit);
