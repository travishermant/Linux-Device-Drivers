/*	Travis Hermant
	4/30/2018
	ECE 373
	Homework 6 */
	
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
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>

#define DEVCNT 1
#define DEVNAME "homework3"
#define DEV_CLASS "homework3_class"
#define PCI_VENDOR_82540 0x8086
#define PCI_DEVICE_82540 0x100F
#define DEV_CONTROL 0x00000
#define LED_CONTROL 0x00E00
#define DEV_STATUS 0x00008


/* interrupts in e1000 
Interrupt Registers: ICR, ICS, IMS, IMC 
ICR = Interrupt Cause Read
IMS = Interrupt Mask Set
IMC = Interrupt Mask Clear
*/
#define ICR 0x000C0
#define IMS 0x000D0
#define IMC 0x000D8
#define IMS_IRQ_ENABLE 0x10
#define LED_INIT 0x0F0E0F0F
#define LED_ALLOFF 0x0F0F0F0F
#define LED_IRQON 0x0F0F0F0E
#define LED_ODDON 0x0F0F0E0F



#define RX_CNTRL 0x00100
#define RX_RDBL 0x02800
#define RX_RDBH 0x02804
#define RX_LEN 0x02808
#define RX_HEAD 0x02810
#define RX_TAIL 0x02818
#define RX_INIT 0x801A
#define LINK_UP 0x1A41
#define PROM 0x001A

static struct class *homework3_class = NULL;

static struct mydev_dev{
	struct cdev cdev;
	dev_t mydev_node;
} mydev;

static char *driver_name = "LED_DRIVER";

static const struct pci_device_id pe_pci_tbl[] = {
	{ PCI_DEVICE(PCI_VENDOR_82540, PCI_DEVICE_82540), 0, 0, 0 },

	/* required last entry */
	{0, }
};


/* reading led register */
uint32_t led_reg

struct my_pci {
	struct pci_dev *pdev;
	void *hw_addr;
	struct work_struct work_task;
};

struct my_pci *devs;

struct rx_descriptor{
	__le64 buffer_addr;
	union{
		__le32 data
		struct{
			__le16 length;
			__le16 css;
		} flags;
	} lower;
	union{
		__le32 data;
		struct{
			__u8 status;
			__u8 error;
			__le16 special;
		}field;
	}upper;
};

struct ring_buff{
	void *buff_mem;
	dma_addr_t dma_handle;
};

struct rx_ring{
	void *dma_mem;
	dma_addr_t dma_handle;
	struct rx_descriptor rx_desc_buff[16];
	struct ring_buff buff[16];
	size_t size;
	uint32_t head;
	uint32_t tail;
} rx_ring;



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
	
	uint16_t head,tail;
	uint32_t temp_conf;
	
	head = readl(devs->hw_addr + RX_HEAD);
	tail = readl(devs->hw_addr + RX_TAIL);
	
	temp_config = tail; temp_config |= head << 16;
	
	printk(KERN_INFO "Head and Tail: 0x%08x\n", temp_conf);
	
	if(*offset >= sizeof(int))
		return 0;	
	if(!buf){
		ret = -EINVAL;
		goto out;
	}

	if(copy_to_user(buf, &temp_conf, sizeof(uint32_t))){
		ret = -EFAULT;
		goto out;
	}
	ret = sizeof(uint32_t);
	*offset += len;
	
	printk(KERN_INFO "User received %d\n", temp_conf);
	
out:
	return ret;
}

/* function for updating syscall_val from what the user inputs */
static ssize_t homework3_write(struct file *file, const char __user *buf, size_t len, loff_t *offset){
	//int user_input; 
	int ret;

	printk(KERN_INFO "ayyy i'm writin' here \n");

	if(!buf){
		ret = -EINVAL;
		goto out;
	}
	
	
	if(copy_from_user(&led_reg, buf, len)){
		ret = -EFAULT;
		goto out;
	}
	ret = len;
	*offset = 0;
	printk(KERN_INFO "User wrote 0x%08x\n", led_reg);
	
	return ret;
}

static void ring_create(struct pci_dev *pdev){
	uint32_t ring_conf;
	
	rx_ring.size = sizeof(struct rx_descriptor)*16;
	
	rx_ring.dma_mem = dma_alloc_coherent(&pdev->dev, rx_ring.size, &rx_ring.dma_handle,
										GFP_KERNEL);
	/*high&low register set up*/
	ring_conf = (rx_ring.dma_handle >> 32) & 0xFFFFFFFF;
	writel(ring_conf, devs->hw_addr + RX_RDBH);
	ring_conf = (rx_ring.dma_handle & 0xFFFFFFFF);
	writel(ring_conf, devs->hw_addr + RX_RDBL);
	printk(KERN_INFO "Set up high and low registers\n");
	/*create head and tail, from 0 to 15 for 16*/
	writel(0, devs->hw_addr + RX_HEAD);
	writel(15,devs->hw_addr + RX_TAIL);
	printk(KERN_INFO "Created head and tail\n");
	/*rx length register set up*/
	writel(rx_ring.size, devs->hw_addr + RX_LEN);
	/*create 2KB buffers*/
	for(int i = 0; i < 16; i++){
		rx_ring.buff[i].dma_handle = dma_map_single(&pdev->dev, 
					rx_ring.buff[i].buff_mem, 2048, DMA_FROM_DEVICE);;
		rx_ring.rx_desc_buff[i].buffer_addr = cpu_to_le64(rx_ring.buffer[i].dma_handle);
		printk(KERN_INFO "Allocated buffer #%d\n",i);
	}
	printk(KERN_INFO "Ring succesfully created\n");
}

static void work_task(struct work_struct *worker){
	uint64_t val;
	
	/*load head and tail*/
	rx_ring.head = readl(devs->hw_addr + RX_HEAD);
	rx_ring.tail = readl(devs->hw_addr + RX_TAIL);
	/*sleeping for half a second*/
	msleep(500);
	
	/*Turn off all LED's*/
	writel(LED_ALLOFF, devs->hw_addr + LED_CONTROL);
	printk(KERN_INFO "All LED's off\n");
	
	val = (uint64_t)rx_ring.rx_desc_buff[rx_ring.head].buffer_addr;
	printk(KERN_INFO "head address: 0x%16x\n", val);
	val = (uint64_t)rx_ring.rx_desc_buff[rx_ring.tail].buffer_addr;
	printk(KERN_INFO "tail address: 0x%16x\n", val);
	
	if(rx_ring.tail == 15)
		writel(0,devs->hw_addr + RX_TAIL) /*if tail is at 15, set it to 0 */
	else
		writel((rx_ring.tail + 1), devs->hw_addr + RX_TAIL);
	
	/*reload tail*/
	rx_ring.tail = readl(devs->hw_addr + RX_TAIL);
	
	if((rx_ring.tail % 2) == 0)
		writel(LED_ALLOFF, devs->hw_addr + LED_CONTROL); /*turn off all LED's if tail is even*/
	else
		writel(LED_ODDON, devs->hw_addr + LED_CONTROL);
}

static irqreturn_t int_handler(int irq, void *data){
	uint32_t val;
	/*turn on LED because interrupt triggered*/
	writel(LED_IRQON, devs->hw_addr + LED_CONTROL);
	
	schedule_work(&devs->work_task);
	
	val = readl(devs->hw_addr + ICR);
	printk(KERN_INFO "Interrupt caused: 0x%08x\n",val);
	writel(IMS_IRQ_ENABLE, devs->hw_addr + IMS);
	
	return IRQ_HANDLED;
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

	writel((1 << 26), devs->hw_addr + DEV_CONTROL);
	udelay(5);
	
	writel(LINK_UP, devs->hw_addr + DEV_CONTROL);
	udelay(5);
	
	writel(IMS_IRQ_ENABLE,devs->hw_addr + IMS);
	udelay(5);
	
	ring_create(pdev);
	
	writel(RX_INIT, devs->hw_addr + RX_CNTRL);
	udelay(5);
	
	INIT_WORK(&devs->work_task, work_task);
	err = request_irq(pdev->irq, int_handler, 0, "e1000_irq", devs);
	writel(LED_INIT, devs->hw_addr + LED_CONTROL)

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

	cancel_work_sync(&devs->service_task);
	free_irq(pdev->irq, devs);
	
	for(int i = 0; i < 16; i++)
		dma_unmap_single(&pdev->dev, rx_ring.buffer[i].dma_handle, 2048, DMA_TO_DEVICE);
	
	dma_free_coherent(&pdev->dev, rx_ring.size, rx_ring.dma_mem, rx_ring.dma_handle);
	
	devs = pce_get_drvdata(pdev);
	
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
	
	del_timer_sync(&homework_timer);

	pci_unregister_driver(&LED_DRIVER);
	printk(KERN_INFO "Unloaded module \n");
}

MODULE_DEVICE_TABLE(pci, pe_pci_tbl);
MODULE_AUTHOR("TRAVIS HERMANT");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.2");
module_init(homework3_init);
module_exit(homework3_exit);
