/*~~~~~~~~~~~~~~~~~~~~~~~~~
	Travis Hermant		  
	ECE373, Spring 2018
	Homework 5
~~~~~~~~~~~~~~~~~~~~~~~~~*/

//Included Libraries
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pci/pci.h>
#include <linux/types.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <getopt.h>
#include <errno.h>


//E1000 info
#define	E1000_LEDCTL		0x00E00
#define E1000_VENDOR_ID		0x8086
#define E1000_DEVICE_ID		0x100F
#define E1000_GOOD_PACKETS	0x04074

//LED write values
#define LED0_ON		0xE
#define LED0_OFF	0xF
#define LED1_ON		0xE00
#define LED1_OFF	0xF00
#define LED2_ON		0xE0000
#define LED2_OFF	0xF0000
#define LED3_ON		0xE000000
#define LED3_OFF	0xF000000
#define RESERVED	0x30303030

#define MEM_WINDOW_SZ  0x00010000
volatile void *e1000_map;


/*
	Opens device and maps memory
	Code referenced from ledmon.c 
	Provided by Shannon Nelson and Brett Creeley
*/
int open_dev(off_t base_addr, volatile void **mem){
	
	int fd;

	fd = open("/dev/mem", O_RDWR);
	if(fd < 0) {
		perror("/dev/mem was not opened\n");
		return -1;
	}

	*mem = mmap(NULL, MEM_WINDOW_SZ, (PROT_READ|PROT_WRITE),
		    MAP_SHARED, fd, base_addr);

	if(*mem == MAP_FAILED) {
		perror("mmap failed\n");
		close(fd);
		return -1;
	}
	
	return fd;
}

/*
	Function to cycle through all devices until the device with the correct
		vendor and device ID are found. Sets base address
	Code referenced from example.c of the PCI Library
	Provided by Martin Mares for public use

*/
void grab_address(off_t *base_address){
	struct pci_access *pacc;
	struct pci_dev *dev;
	char namebuf[1024], *name;

	pacc = pci_alloc();
	
	pci_init(pacc);
	
	pci_scan_bus(pacc);
	
	for(dev = pacc->devices; dev; dev = dev->next){
		pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_BASES | PCI_FILL_CLASS);
		
		name = pci_lookup_name(pacc, namebuf, sizeof(namebuf), PCI_LOOKUP_DEVICE, dev->vendor_id, dev->device_id);
		
		if((dev->vendor_id == E1000_VENDOR_ID) && (dev->device_id == E1000_DEVICE_ID)){
			*base_address = dev->base_addr[0] -0x4;
			printf("The base address is: 0x%2x \n", *base_address);
		}
	}	
	pci_cleanup(pacc);
	return;	
}

/* 	Write from register
	Referenced from ledmon.c */
void write_32(uint32_t reg, uint32_t value)
{
	uint32_t *p = (uint32_t *)(e1000_map + reg);
	*p = value;
}

/*	Read from register 
	Referenced from ledmon.c */
uint32_t read_32(uint32_t reg)
{
	uint32_t *p = (uint32_t *)(e1000_map + reg);
	uint32_t ret = *p;
	return ret;
}

int main(){
	
	int fd;
	uint32_t control;
	uint32_t config;
	uint32_t value;
	off_t base_address;
	//Finds base address of E1000
	grab_address(&base_address);	
	//Map memory for device	
	fd = open_dev(base_address,&e1000_map);	
		if(fd<0){
			perror("Device not opened\n");
			return -1;
		}	
	//Fetching previous LEDCTL value and writing it out to user
	control = read_32(E1000_LEDCTL);
	config = control & RESERVED;
	printf("Current LEDCTL value of E1000: 0x%08x\n", control);
	
	/*----------------------------------------- 
		Time for some high-octane LED blinking
	-------------------------------------------*/
	//Turn off all LED's that were previously on
	write_32(E1000_LEDCTL, config | LED0_OFF | LED1_OFF | LED2_OFF | LED3_OFF);
		printf("Turned all LED's off\n");
		sleep(1);
	
	//Turn LED2 and LED0 on for 2 seconds
	write_32(E1000_LEDCTL, config | LED2_ON | LED0_ON);
		printf("LED3[ ] LED2[X] LED1 [ ] LED0[X]\n");
		sleep(2);
	
	//Turn all LED's off for 2 seconds
	write_32(E1000_LEDCTL, config | LED0_OFF | LED1_OFF | LED2_OFF | LED3_OFF);
		printf("LED3[ ] LED2[ ] LED1 [ ] LED0[ ]\n");
		sleep(2);
	
	//Loop 5 times and turn each LED on for 1 second indvidually
	for(int i = 0; i < 5; i++){
		printf("Loop number %d\n",i + 1);
		write_32(E1000_LEDCTL, config | LED0_ON | LED3_OFF);
			printf("LED3[ ] LED2[ ] LED1 [ ] LED0[X]\n");
			sleep(1);
		write_32(E1000_LEDCTL, config | LED1_ON | LED0_OFF);
			printf("LED3[ ] LED2[ ] LED1 [X] LED0[ ]\n");
			sleep(1);
		write_32(E1000_LEDCTL, config | LED2_ON | LED1_OFF);
			printf("LED3[ ] LED2[X] LED1 [ ] LED0[ ]\n");
			sleep(1);
		write_32(E1000_LEDCTL, config | LED3_ON | LED0_OFF);
			printf("LED3[X] LED2[ ] LED1 [ ] LED0[ ]\n");
			sleep(1);	
	}
	
	//Restoring LEDCTL to its initial value
	write_32(E1000_LEDCTL, control);
	value = read_32(E1000_LEDCTL);
	printf("Restored LEDCTL value: 0x%08x\n",value);
	
	//Reading contents of Good Packets Received register
	value = read_32(E1000_GOOD_PACKETS);
	printf("Good Packets Received register value: 0x%08x\n",value);
	
	/*--------------------------
	Clean up and get out of here
	--------------------------*/
	close(fd);
	munmap((void *)e1000_map, MEM_WINDOW_SZ);
	return 1;
}	
