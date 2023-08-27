/*
 *2.这个代码增加了读写阻塞和非阻塞的代码实现
 *需要在表示字符设备的结构体mychar_dev里边增加一个成员
 *3.增加了多路复用的功能：要在操作函数集里边增加一个支持多路复用的支持成员！！！
 *4.增加了信号驱动的IO模式！要在操作函数集里边增加一个支持多路复用的支持成员！！！
 *	而且如果描述符被关闭，这个对象要不用要关闭！close掉
 *7.信号量：允许多个任务同时使用一个资源(字符设备)（把curlen和BUF_LEN看作是一个资源所以就要用信号量控制一下）
 *8.互斥锁：只用于单个资源之间
 * */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/mutex.h>

#include "mychar.h"
//#include <linux/poll.h>
//#include <linux/regset.h>
#include <asm/uaccess.h>
#define BUF_LEN 100
int major=11;
int minor=0;
int mychar_num=1;

struct mychar_dev{
	char mydev_buf[BUF_LEN];//sizeof，设备产生的数据放在这个空间里边
	char curlen;//表示数据总是从0开始
	struct cdev mydev;
	//7.1
	//struct semaphore sem;
	//8.1
	struct mutex lock;
	/* 增加等待队列 */
	wait_queue_head_t rq;
	wait_queue_head_t wq;/*使用之前要对这两个进行初始化__init mychar_init*/
	/*4.3增加信号驱动成员*/
	struct fasync_struct *pasync_obj;
};
struct mychar_dev gmydev;//全局变量就只使用在入口函数这里，如init等等！

int mychar_open(struct inode *pnode,struct file *pfile){//要使与设备相关的几个函数不使用全局变量！例如：read阿write啊！
							//所以要在open函数里边做手脚
	pfile->private_data=(void *)(container_of(pnode->i_cdev,struct mychar_dev,mydev));//这样就可以获得gmydev的地址，就避免了访问全局变量
	printk("mychar_open is called\n");
	return 0;
}
int mychar_close(struct inode *pnode,struct file *pfile){
	struct mychar_dev *pmydev=(struct mychar_dev *)pfile->private_data;//要用到gmydev就加个这样的语句就好了
	printk("mychar_close is called\n");
	/*4.4关闭销毁号驱动创建的对象*/
	if(pmydev->pasync_obj){//不为空在进行销毁 
		fasync_helper(-1,pfile,0,&pmydev->pasync_obj);//设备对象需要得到设备对象的地址
	}
	return 0;
} 
/* 设备相关函数避免使用全局变量 */
ssize_t mychar_read(struct file *pfile,char __user *puser,size_t count,loff_t *p_pos){
	/* 像这个以后要用到gmydev这个全局变量的怎么办呢？ */
	/* 定义一个全局变量 */
	struct mychar_dev *pmydev=(struct mychar_dev *)pfile->private_data;//要用到gmydev就加个这样的语句就好了
	int size=0;
	int ret=0;
	//7.3 使用信号量
	mutex_lock(&pmydev->lock);
	if(pmydev->curlen<=0){
		if(pfile->f_flags & O_NONBLOCK){
			mutex_unlock(&pmydev->lock);
			printk("O_NONBLOCK No Data Read\n");
			return -1;
		}else{
			mutex_unlock(&pmydev->lock);//7.4一定要注意是在睡眠前把资源放开～
			ret=wait_event_interruptible(pmydev->rq,pmydev->curlen>0);//when > return
			if(ret){
				printk("Wake up by signal\n");
				return -ERESTARTSYS; 	 
			}
			mutex_lock(&pmydev->lock);//7.5 醒来之后重新加锁
		} 
	}
	if(count>pmydev->curlen){//如果期望读到的值大于实际有的至
		size=pmydev->curlen;//就只能读这么多，防止越界
	}else{
		size=count;
	}
	ret=copy_to_user(puser,pmydev->mydev_buf,size);
	if(ret){
		mutex_unlock(&pmydev->lock);//7.6这个地方有出去
		printk("cope_to_user filed\n");
		return -1;
	}
	/* 将生于字节拷贝到起始位置上去 */
	memcpy(pmydev->mydev_buf,pmydev->mydev_buf+size,pmydev->curlen-size);
	pmydev->curlen=pmydev->curlen-size;
	
	mutex_unlock(&pmydev->lock);//7.7这个地方有出去
	wake_up_interruptible( &pmydev->wq); 
	return size;
}
ssize_t mychar_write(struct file *pfile,const char __user *puser,size_t count,loff_t *p_pos){
	int size=0;
	int ret=0;
	struct mychar_dev *pmydev=(struct mychar_dev *)pfile->private_data;//要用到gmydev就加个这样的语句就好了
	mutex_lock(&pmydev->lock);
	if(pmydev->curlen>=BUF_LEN){
		if(pfile->f_flags & O_NONBLOCK){
	    		mutex_unlock(&pmydev->lock);
			printk("O_NONBLOCK Can Not Write Data\n");
			return -1;
		}else{
			mutex_unlock(&pmydev->lock);
			ret=wait_event_interruptible(pmydev->wq,pmydev->curlen<BUF_LEN);
			if(ret){
				printk("Wake up by signal\n"); 
				return -ERESTARTSYS;
			}
	   		mutex_lock(&pmydev->lock);
		}
	}

	if(count>BUF_LEN-pmydev->curlen){
		size=BUF_LEN-pmydev->curlen;
	}else{
		size=count;//能满足就尽量满足，没法满足限定一下；
	}
	ret=copy_from_user(pmydev->mydev_buf+pmydev->curlen,puser,size);
	if(ret){
		mutex_unlock(&pmydev->lock);
		printk("copy from user fialed\n");
		return -1;
	}
	pmydev->curlen=pmydev->curlen+size;

	mutex_unlock(&pmydev->lock);
	wake_up_interruptible(&pmydev->rq); //有数据就唤醒，有数据写入把读数据唤醒
	return size;//返回实际写成功的字节数
	/*4.5写函数中有数据可读时向应用层发信号*/

	if(pmydev->pasync_obj){//对象创建成功存在！
		kill_fasync(&pmydev->pasync_obj,SIGIO,POLL_IN);
	}


} 
/* ioctl属性控制 */
long mychar_ioctl(struct file *pfile,unsigned int cmd,unsigned long arg){
	int __user *pret=(int *)arg;//定义一个用户空间的指针
	int maxlen=BUF_LEN;//然后需要将这个值拷贝到用户空间
	int ret=0;
	struct mychar_dev *pmydev=(struct mychar_dev *)pfile->private_data;//要用到gmydev就加个这样的语句就好了
	switch(cmd){
		case MYCHAR_IOCTL_GET_MAXLEN:
			ret=copy_to_user(pret,&maxlen,sizeof(int));//值在maxlen里边
			if(ret){
				printk("copy to user MAXLEN failed\n");
				return -1;
			}
			break;
		case MYCHAR_IOCTL_GET_CURLEN:
			mutex_lock(&pmydev->lock);
			ret=copy_to_user(pret,&pmydev->curlen,sizeof(int));//值在maxlen里边
			if(ret){
				mutex_unlock(&pmydev->lock);
				printk("copy to user CURLEN failed\n");
				return -1;
			}
			mutex_unlock(&pmydev->lock);

			break;
		default:
			printk("The cmd is unknow\n");
			return -1;
	}
	return 0;

}
unsigned int mychar_poll(struct file *pfile,poll_table *ptb){
	/*得到设备对象的地址*/
	struct mychar_dev *pmydev=(struct mychar_dev *)pfile->private_data;//要用到gmydev就加个这样的语句就好了
	unsigned int mask=0;
	/*将等待队列加入到poll_table的表中*/
	poll_wait(pfile,&pmydev->rq,ptb);
	poll_wait(pfile,&pmydev->wq,ptb);

	mutex_lock(&pmydev->lock);
	if(pmydev->curlen>0){
		//表示有数据可读
		mask |= POLLIN | POLLRDNORM;
	}
	if(pmydev->curlen <BUF_LEN){
		//有空间可写
		mask |= POLLOUT | POLLWRNORM; 
	}
	mutex_unlock(&pmydev->lock);
	/*通过返回值判断是否有相应的位被置位*/
	return mask;
		//这样多路复用的功能就由这个函数实现就好了
}
//4.2
int mychar_fasync(int fd,struct file *pfile,int mode){
	//要先得到设备对象的地址
	struct mychar_dev *pmydev=(struct mychar_dev *)pfile->private_data;//要用到gmydev就加个这样的语句就好了
	return fasync_helper(fd,pfile, mode,&pmydev->pasync_obj);//这样就创建了pmydev->pasync_obj这种类新的对象

}

/*操作函数集*/
struct file_operations myops = {//类型管理对象
	.owner=THIS_MODULE,
	.open=mychar_open,
	.release=mychar_close,
	.read=mychar_read,
	.write=mychar_write,
	.unlocked_ioctl=mychar_ioctl,//表示指向我们设备的ioctl函数,然后就到外边去实现他
	/* 增加多路复用 */
	.poll=mychar_poll,
	/*4.1增加信号驱动*/
	.fasync=mychar_fasync,
};

int __init mychar_init(void)
{ 
	int ret=0;
	dev_t devno=MKDEV(major,minor);
	/* 申请设备号 */
	ret=register_chrdev_region(devno,mychar_num,"mychar");//申请设备号
	if(ret){//返回为1就出错！
		ret=alloc_chrdev_region(&devno,minor,mychar_num,"mychar");
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
	cdev_add(&gmydev.mydev,devno,mychar_num);//将我们的对象加入到我们内核管理设备的哈希链表里边去
	/* 这里进行阻塞非阻塞初始化 */
	init_waitqueue_head(&gmydev.rq);
	init_waitqueue_head(&gmydev.wq);/* 接下来改写读写函数 */

	//7.2 初始化
	//sema_init(&gmydev.sem,1);
	//8.2
	mutex_init(&gmydev.lock);
	return 0;
}

void __exit mychar_exit(void)
{
	dev_t devno=MKDEV(major,minor);//注销设备号
	cdev_del(&gmydev.mydev);//退出时从内核的哈希链表中删除
	unregister_chrdev_region(devno,mychar_num);//将设备号归还给系统
	printk("mychar will exit\n");
}


MODULE_LICENSE("GPL");
//MODULE_AUTHOR("曾东城");
//MODULE_DESCRIPTION("It is only a simple test.");
//MODULE_ALIAS("HI");

module_init(mychar_init);
module_exit(mychar_exit);
