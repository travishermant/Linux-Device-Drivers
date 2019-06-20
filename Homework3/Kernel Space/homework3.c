/*	Travis Hermant
	4/30/2018
	ECE 373
	Homework 3 */
	
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/errno.h>
#include <linux/init.h>

#define DEVCNT 1
#define DEVNAME "homework3"
#define DEV_CLASS "homework3_class"
#define PCI_VENDOR_82540 0x8086
#define PCI_DEVICE_82540 0x100F
#define DEV_CONTROL 0x00000
#define LED_CONTROL 0x00E00


static struct class *homework3_class = NULL;

static struct mydev_dev{
	struct cdev cdev;
	dev_t mydev_node;
	int syscall_val;
} mydev;

static char *driver_name = "LED_DRIVER";

static const struct pci_device_id pe_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_82540, PCI_DEVICE_82540), 0, 0, 0 },

	/* required last entry */
	{0, }
};


/* reading led register */
static u32 led_value;
module_param(led_value, int, 0);

struct my_pci {
	struct pci_dev *pdev;
	void *hw_addr;
};
struct my_pci *devs;

/* 
	FUNCTIONS 
				*/


/* function for opening the module */
static int homework3_open(struct inode *inode, struct file *file){
	printk(KERN_INFO "Opened succesfully \n");
	return 0;
}

/* function for reading syscall_val from the module */
static ssize_t homework3_read(struct file *file, char __user *buf, size_t len, loff_t *offset){
	int ret;
	printk(KERN_INFO "ayyy i'm readin' here \n");

	if(*offset >= sizeof(uint32_t))
		return 0;	
	if(!buf){
		ret = -EINVAL;
		goto out;
	}

	if(copy_to_user(buf, &led_value, sizeof(uint32_t))){
		ret = -EFAULT;
		goto out;
	}
	ret = sizeof(uint32_t);
	*offset += len;
	
	printk(KERN_INFO "User received %08x\n", led_value);
	
out:
	return ret;
}

/* function for updating syscall_val from what the user inputs */
static ssize_t homework3_write(struct file *file, const char __user *buf, size_t len, loff_t *offset){
	uint32_t user_input; 
	int ret;

	printk(KERN_INFO "ayyy i'm writin' here \n");

	if(!buf){
		ret = -EINVAL;
		goto out;
	}
	
	
	if(copy_from_user(&user_input, buf, len)){
		ret = -EFAULT;
		goto mem_out;
	}
	
	ret = len;
	*offset = 0;
	
	printk(KERN_INFO "User sent %08x \n", led_value);
	writel(user_input, devs->hw_addr + LED_CONTROL);
	led_value = readl(devs->hw_addr + LED_CONTROL);
	printk(KERN_INFO "New value %08x \n", led_value);

mem_out:
out:
	return ret;
}


static int dev_probe(struct pci_dev *pdev,
			      const struct pci_device_id *ent)
{
	
	uint32_t ioremap_len;
	uint32_t led_conf;
	int err;

	err = pci_enable_device_mem(pdev);
	if (err)
		return err;

	/* set up for high or low dma */
	err = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
	if (err) {
		dev_err(&pdev->dev, "DMA configuration failed: 0x%x\n", err);
		goto err_dma;
	}

	/* set up pci connections */
	err = pci_request_selected_regions(pdev, pci_select_bars(pdev,
					   IORESOURCE_MEM), driver_name);
	if (err) {
		dev_info(&pdev->dev, "pci_request_selected_regions failed %d\n", err);
		goto err_pci_reg;
	}

	pci_set_master(pdev);

	devs = kzalloc(sizeof(*devs), GFP_KERNEL);
	if (!devs) {
		err = -ENOMEM;
		goto err_dev_alloc;
	}
	devs->pdev = pdev;
	pci_set_drvdata(pdev, devs);

	/* map device memory */
	ioremap_len = min_t(int, pci_resource_len(pdev, 0), 1024);
	devs->hw_addr = ioremap(pci_resource_start(pdev, 0), ioremap_len);
	if (!devs->hw_addr) {
		err = -EIO;
		dev_info(&pdev->dev, "ioremap(0x%04x, 0x%04x) failed: 0x%x\n",
			 (unsigned int)pci_resource_start(pdev, 0),
			 (unsigned int)pci_resource_len(pdev, 0), err);
		goto err_ioremap;
	}

	led_conf = readl(devs->hw_addr + DEV_CONTROL);
	dev_info(&pdev->dev, "led_conf = 0x%08x\n", led_conf);
	
	led_conf = readl(devs->hw_addr + DEV_CONTROL + LED_CONTROL);
	dev_info(&pdev->dev, "led_conf = 0x%08x\n", led_conf);
	
	led_value = led_conf;
	dev_info(&pdev->dev, "led_value = 0x%08x\n", led_value);
	

	return 0;

err_ioremap:
	kfree(devs);
err_dev_alloc:
	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
err_pci_reg:
err_dma:
	pci_disable_device(pdev);
	return err;
}

static void dev_remove(struct pci_dev *pdev)
{
	struct my_pci *devs = pci_get_drvdata(pdev);

	/* unmap device from memory */
	iounmap(devs->hw_addr);

	/* free any allocated memory */
	kfree(devs);

	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
	pci_disable_device(pdev);
}



/* struct for directing open, read, and write syscalls */
static struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.open = homework3_open,
	.read = homework3_read,
	.write = homework3_write,
};	

static struct pci_driver LED_DRIVER = {
	.name     = "pci_travis",
	.id_table = pe_pci_tbl,
	.probe    = dev_probe,
	.remove   = dev_remove,
};

/* init function to allocate for the module */
static int __init homework3_init(void){
	int ret;
	printk(KERN_INFO "Module loading \n");	
 

	if(alloc_chrdev_region(&mydev.mydev_node, 0, DEVCNT, DEVNAME)){
		printk(KERN_ERR "Allocating chrdev failed \n");
		return -1;
	}
	
	/*initialize the character device */
	cdev_init(&mydev.cdev, &mydev_fops);
	mydev.cdev.owner = THIS_MODULE;
		
	
	printk(KERN_INFO "Allocated %d devices at major: %d\n", DEVCNT, MAJOR(mydev.mydev_node));
	
	homework3_class = class_create(THIS_MODULE, DEV_CLASS);

	if(cdev_add(&mydev.cdev, mydev.mydev_node, DEVCNT)){
		printk(KERN_ERR "cdev_add failed \n");
		/* clean up allocation of chrdev */
		unregister_chrdev_region(mydev.mydev_node, DEVCNT);
		return -1;
	}
	
	ret = pci_register_driver(&LED_DRIVER);
	return ret;
	 
}

/* exit function to clean up program and extras */
static void __exit homework3_exit(void){
	cdev_del(&mydev.cdev);
	device_destroy(homework3_class, mydev.mydev_node);
	class_destroy(homework3_class);
	unregister_chrdev_region(mydev.mydev_node, DEVCNT);	
	
	pci_unregister_driver(&LED_DRIVER);
	printk(KERN_INFO "Unloaded module \n");
}

MODULE_DEVICE_TABLE(pci, pe_pci_tbl);
MODULE_AUTHOR("TRAVIS HERMANT");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
module_init(homework3_init);
module_exit(homework3_exit);