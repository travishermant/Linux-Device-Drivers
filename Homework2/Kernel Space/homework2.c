/*	Travis Hermant
	4/18/2018
	ECE 373
	Homework 2 */
	
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVCNT 1
#define DEVNAME "homework2"
#define DEV_CLASS "homework2_class"

static struct class *homework2_class = NULL;

static struct mydev_dev{
	struct cdev cdev;
	dev_t mydev_node;
	int sys_int;
	int syscall_val;
} mydev;

/* Setting default param to 25, unless otherwise stated */
int homework2_param = 25;
module_param(homework2_param, int, S_IRUSR | S_IWUSR);

/* function for opening the module */
static int homework2_open(struct inode *inode, struct file *file){
	printk(KERN_INFO "Opened succesfully \n");
	mydev.syscall_val = homework2_param;
	return 0;
}

/* function for reading syscall_val from the module */
static ssize_t homework2_read(struct file *file, char __user *buf, size_t len, loff_t *offset){
	int ret;
	printk(KERN_INFO "ayyy i'm readin' here \n");

	if(*offset >= sizeof(int))
		return 0;	
	if(!buf){
		ret = -EINVAL;
		goto out;
	}
	if(copy_to_user(buf, &mydev.syscall_val, sizeof(int))){
		ret = -EFAULT;
		goto out;
	}
	ret = sizeof(int);
	*offset += len;
	
	printk(KERN_INFO "User received %d \n", mydev.syscall_val);
	
out:
	return ret;
}

/* function for updating syscall_val from what the user inputs */
static ssize_t homework2_write(struct file *file, const char __user *buf, size_t len, loff_t *offset){
	char *kern_buf;
	int ret;

	printk(KERN_INFO "ayyy i'm writin' here \n");

	if(!buf){
		ret = -EINVAL;
		goto out;
	}
	
	kern_buf = kmalloc(len, GFP_KERNEL);
	
	if(!kern_buf){
		ret = -ENOMEM;
		goto out;
	}
	
	if(copy_from_user(kern_buf, buf, len)){
		ret = -EFAULT;
		goto mem_out;
	}
	
	ret = len;
	printk(KERN_INFO "Userspace wrote %s to us \n", kern_buf);
	*offset = 0;
	/*converting the input string to an int */ 
	if(kstrtoint(kern_buf, 10, &mydev.syscall_val)){
		ret = -EFAULT;
		goto out;	
	} 

mem_out:
	kfree(kern_buf);
out:
	return ret;
}

/* struct for directing open, read, and write syscalls */
static struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.open = homework2_open,
	.read = homework2_read,
	.write = homework2_write,
};	

/* init function to allocate for the module */
static int __init homework2_init(void){
	printk(KERN_INFO "Module loading \n");	
 

	if(alloc_chrdev_region(&mydev.mydev_node, 0, DEVCNT, DEVNAME)){
		printk(KERN_ERR "Allocating chrdev failed \n");
		return -1;
	}
	
	/*initialize the character device */
	cdev_init(&mydev.cdev, &mydev_fops);
	mydev.cdev.owner = THIS_MODULE;
		
	
	printk(KERN_INFO "Allocated %d devices at major: %d\n", DEVCNT, MAJOR(mydev.mydev_node));
	
	homework2_class = class_create(THIS_MODULE, DEV_CLASS);

	if(cdev_add(&mydev.cdev, mydev.mydev_node, DEVCNT)){
		printk(KERN_ERR "cdev_add failed \n");
		/* clean up allocation of chrdev */
		unregister_chrdev_region(mydev.mydev_node, DEVCNT);
		return -1;
	}
	 return 0;	 
}

/* exit function to clean up program and extras */
static void __exit homework2_exit(void){
	cdev_del(&mydev.cdev);
	device_destroy(homework2_class, mydev.mydev_node);
	class_destroy(homework2_class);
	unregister_chrdev_region(mydev.mydev_node, DEVCNT);	
	printk(KERN_INFO "Unloaded module \n");
}

MODULE_AUTHOR("TRAVIS HERMANT");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
module_init(homework2_init);
module_exit(homework2_exit);
