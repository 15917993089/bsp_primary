/*
 *5.原子操作：原子变量
 *一个设备一次只能被访问一次，只能被打开一次
 * 1：表示设备打开
 * 0：设备不可打开
 * */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

int major=11;
int minor=0;
int openonce_num=1;

struct openonce_dev{
	struct cdev mydev;
	//5.1
	atomic_t openflag;
};
struct openonce_dev gmydev;//全局变量就只使用在入口函数这里，如init等等！

int openonce_open(struct inode *pnode,struct file *pfile){//要使与设备相关的几个函数不使用全局变量！例如：read阿write啊！
							//所以要在open函数里边做手脚
	struct openonce_dev *pmydev=NULL;
	pfile->private_data=(void *)(container_of(pnode->i_cdev,struct openonce_dev,mydev));//这样就可以获得gmydev的地址，就避免了访问全局变量(设置回去)
	pmydev=(struct openonce_dev *)pfile->private_data;//要用到gmydev就加个这样的语句就好（取回来),就可以避免访问全局变量
	//5.3
	if(atomic_dec_and_test(&pmydev->openflag)){
		
		return 0;//打开成功
	}else{
		atomic_inc(&pmydev->openflag);//5.4 加1变为0
		printk("The device is open already\n");
		return -1;
	}
	printk("openonce_open is called\n");
	return 0;
}
int openonce_close(struct inode *pnode,struct file *pfile){
	struct openonce_dev *pmydev=(struct openonce_dev *)pfile->private_data;//要用到gmydev就加个这样的语句就好
	printk("openonce_close is called\n");
	atomic_set(&pmydev->openflag,1);
	return 0;
} 

struct file_operations myops = {//类型管理对象
	.owner=THIS_MODULE,
	.open=openonce_open,
	.release=openonce_close,
};

int __init openonce_init(void) 
{ 
	int ret=0;
	dev_t devno=MKDEV(major,minor);
	/* 申请设备号 */
	ret=register_chrdev_region(devno,openonce_num,"openonce");//申请设备号
	if(ret){//返回为1就出错！
		ret=alloc_chrdev_region(&devno,minor,openonce_num,"openonce");
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
	cdev_add(&gmydev.mydev,devno,openonce_num);//将我们的对象加入到我们内核管理设备的哈希链表里边去
	//5.2将openflag置为1,表示开始可以打开
	atomic_set(&gmydev.openflag,1);

	return 0;
}

void __exit openonce_exit(void)
{
	dev_t devno=MKDEV(major,minor);//注销设备号
	cdev_del(&gmydev.mydev);//退出时从内核的哈希链表中删除
	unregister_chrdev_region(devno,openonce_num);//将设备号归还给系统
	printk("openonce will exit\n");
}


MODULE_LICENSE("GPL");
//MODULE_AUTHOR("曾东城");
//MODULE_DESCRIPTION("It is only a simple test.");
//MODULE_ALIAS("HI");

module_init(openonce_init);
module_exit(openonce_exit);
