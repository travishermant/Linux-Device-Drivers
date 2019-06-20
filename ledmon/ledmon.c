/**********************************************************************
 *  ledmon.c
 *
 *  Following the LED register notes in the Intel datasheet for the
 *  82575eb network device, this code is meant to monitor and display
 *  the current status for the 3 LEDs on the back of the typical NIC.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <pci/pci.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <linux/types.h>
#include <errno.h>

#define IFNAMSIZ 16

#define MEM_WINDOW_SZ  0x00010000

/* LED Control */
#define E1000_LEDCTL	0x00E00	/* LED Control - RW */

#define E1000_LEDCTL_LED0_SHIFT		0
#define E1000_LEDCTL_LED1_SHIFT		8
#define E1000_LEDCTL_LED2_SHIFT		16
#define E1000_LEDCTL_LED3_SHIFT		24

#define E1000_LEDCTL_MODE_MASK		0x0F
#define E1000_LEDCTL_IVRT		0x40
#define E1000_LEDCTL_BLINK		0x80

#define E1000_LEDCTL_MODE_LINK_10K	0x0
#define E1000_LEDCTL_MODE_LINK_100K	0x1
#define E1000_LEDCTL_MODE_LINK_UP	0x2
#define E1000_LEDCTL_MODE_ACTIVITY	0x3
#define E1000_LEDCTL_MODE_LINK_ACT	0x4
#define E1000_LEDCTL_MODE_LINK_10	0x5
#define E1000_LEDCTL_MODE_LINK_100	0x6
#define E1000_LEDCTL_MODE_LINK_1000	0x7
#define E1000_LEDCTL_MODE_PCIX		0x8
#define E1000_LEDCTL_MODE_FULL_DUP	0x9
#define E1000_LEDCTL_MODE_COLLISION	0xA
#define E1000_LEDCTL_MODE_BUS_SPEED	0xB
#define E1000_LEDCTL_MODE_BUS_SIZE	0xC
#define E1000_LEDCTL_MODE_PAUSED	0xD
#define E1000_LEDCTL_MODE_LED_ON	0xE
#define E1000_LEDCTL_MODE_LED_OFF	0xF

#define E1000_LEDCTL_INITIAL_VALUE  \
	E1000_LEDCTL_MODE_LINK_UP << E1000_LEDCTL_LED0_SHIFT   | \
	(E1000_LEDCTL_BLINK | E1000_LEDCTL_MODE_ACTIVITY)        \
		<< E1000_LEDCTL_LED1_SHIFT                     | \
	E1000_LEDCTL_MODE_LINK_100 << E1000_LEDCTL_LED2_SHIFT  | \
	E1000_LEDCTL_MODE_LINK_1000 << E1000_LEDCTL_LED3_SHIFT


volatile void *e1000e_mem;
char *portname;
char *pci_bus_slot;


/* map the network device into our memory space */
int open_dev(off_t base_addr, volatile void **mem)
{
	int fd;

	fd = open("/dev/mem", O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	*mem = mmap(NULL, MEM_WINDOW_SZ, (PROT_READ|PROT_WRITE), MAP_SHARED, fd,
			base_addr);
	if (*mem == MAP_FAILED) {
		perror("mmap/readable - try rebooting with iomem=relaxed");
		close(fd);
		return -1;
	}

	return fd;
}

/* write to a device register */
void ew32(u32 reg, u32 value)
{
	u32 *p = (u32 *)(e1000e_mem + reg);
	*p = value;
}

/* read from a device register */
u32 er32(u32 reg)
{
	u32 *p = (u32 *)(e1000e_mem + reg);
	u32 v = *p;
	return v;
}

/* decode the led bits into on or off (true or false) */
bool get_led_state(u8 led_bits)
{
	bool led_state;
	FILE *input;
	char speed_buf[16];
	char filename[64];
	int speed = 0;
	u8 mode;

	/* If the 'speed' file exists, we have link */
	snprintf(filename, sizeof(filename), "/sys/class/net/%s/speed", portname);
	input = fopen(filename, "r");
	if (!input) {
		if (errno != ENOENT) {
			perror(filename);
			exit(1);
		}
	} else {
		fgets(speed_buf, sizeof(speed_buf), input);
		fclose(input);
		speed = atoi(speed_buf);
	}

	/* should the LED be on or off? */
	led_state = false;
	mode = led_bits & E1000_LEDCTL_MODE_MASK;
	switch (mode) {
	case E1000_LEDCTL_MODE_LINK_10K:
		led_state = (speed == 10 || speed == 1000);
		break;

	case E1000_LEDCTL_MODE_LINK_100K:
		led_state = (speed == 100 || speed == 1000);
		break;

	case E1000_LEDCTL_MODE_LINK_UP:
		led_state = (speed != 0);
		break;

	case E1000_LEDCTL_MODE_ACTIVITY:
		break;

	case E1000_LEDCTL_MODE_LINK_ACT:
		led_state = (speed != 0);
		break;

	case E1000_LEDCTL_MODE_LINK_10:
		led_state = (speed == 10);
		break;

	case E1000_LEDCTL_MODE_LINK_100:
		led_state = (speed == 100);
		break;

	case E1000_LEDCTL_MODE_LINK_1000:
		led_state = (speed == 1000);
		break;

	case E1000_LEDCTL_MODE_PCIX:
		break;

	case E1000_LEDCTL_MODE_FULL_DUP:
		break;

	case E1000_LEDCTL_MODE_COLLISION:
		break;

	case E1000_LEDCTL_MODE_BUS_SPEED:
		break;

	case E1000_LEDCTL_MODE_BUS_SIZE:
		break;

	case E1000_LEDCTL_MODE_PAUSED:
		break;

	case E1000_LEDCTL_MODE_LED_ON:
		led_state = true;
		break;

	case E1000_LEDCTL_MODE_LED_OFF:
		led_state = false;
		break;
	}

	if (led_bits & E1000_LEDCTL_IVRT)
		led_state = !led_state;

	if (led_bits & E1000_LEDCTL_BLINK)
		led_state = !led_state;

	return led_state;
}

/* print a simple ASCII representation of the 4 LEDs */
void print_leds()
{
	bool led0, led1, led2, led3;
	u8 led_bits;
	u32 ledctl;

	ledctl = er32(E1000_LEDCTL);

	led_bits = ledctl & 0xFF;
	led0 = get_led_state(led_bits);

	led_bits = (ledctl >> E1000_LEDCTL_LED1_SHIFT) & 0xFF;
	led1 = get_led_state(led_bits);

	led_bits = (ledctl >> E1000_LEDCTL_LED2_SHIFT) & 0xFF;
	led2 = get_led_state(led_bits);

	led_bits = (ledctl >> E1000_LEDCTL_LED3_SHIFT) & 0xFF;
	led3 = get_led_state(led_bits);

	printf("%s.0 LEDs:   %c %c %c %c\r",
		pci_bus_slot,
		led0 ? '0' : '.',
		led1 ? '0' : '.',
		led2 ? '0' : '.',
		led3 ? '0' : '.');
	fflush(stdout);
}

int main(int argc, char **argv)
{
	int dev_mem_fd;
	char buf[128] = { 0 };
	char pci_entry[128] = { 0 };
	char addr_str[10] = { 0 };
	bool readloop = false;
	off_t base_addr;
	FILE *input;
	int ret = 0;
	int len;

	if (getuid() != 0) {
		fprintf(stderr, "%s: must be run as root\n", argv[0]);
		exit(1);
	}

	if (argv[1] == NULL) {
		fprintf(stderr, "Usage: %s <bus:slot> [ethX] [-L]\n", argv[0]);
		exit(1);
	}

	pci_bus_slot = argv[1];
	snprintf(buf, sizeof(buf), "lspci -s %s.0", pci_bus_slot);

	/* Index depends on whether or not user specified <ethX> argument */
	if ((argc == 3 && strcmp(argv[2], "-L") == 0) ||
	    (argc == 4 && strcmp(argv[3], "-L") == 0))
		readloop = true;

	/* Does pci device specified by the user exist? */
	input = popen(buf, "r");
	if (!input) {
		perror("pci entry specified does not exist");
		exit(1);
	}

	fgets(pci_entry, sizeof(pci_entry), input);
	fclose(input);
	len = strlen(pci_entry);
	if (len <= 1) {
		fprintf(stderr, "%s.0 not found\n", pci_bus_slot);
		exit(1);
	}

	/* Using the network interface to determine link speed is optional */
	if (argc == 4 && strlen(argv[2]) > IFNAMSIZ)
		fprintf(stderr, "%s: bad interface name\n", argv[0]);
	else if (argc == 4)
		portname = argv[2];

#define LSPCI_DEVICE_TYPE_OFFSET 8
	if (strncmp(pci_entry + LSPCI_DEVICE_TYPE_OFFSET, "Ethernet controller",
		    strlen("Ethernet controller"))) {
			fprintf(stderr, "%s wrong pci device\n", pci_entry);
			exit(1);
	}

#define LSPCI_DEVICE_SPEED_OFFSET 55
	if (strncmp(pci_entry + LSPCI_DEVICE_SPEED_OFFSET, "Gigabit",
		    strlen("Gigabit"))) {
		printf("%s\n", pci_entry + LSPCI_DEVICE_SPEED_OFFSET);
		fprintf(stderr, "%s wrong pci device speed\n", pci_entry);
		exit(1);
	}

	snprintf(buf, sizeof(buf),
		 "lspci -s %s.0 -v | grep \"Memory at\" | awk '{ print $3 }'",
		 pci_bus_slot);
	input = popen(buf, "r");
	if (!input) {
		printf("%s\n", buf);
		perror("getting device mem info");
		exit(1);
	}
	fgets(addr_str, sizeof(addr_str), input);
	fclose(input);

        base_addr = strtol(addr_str, NULL, 16);
        if (len <= 1) {
                fprintf(stderr, "%s memory address invalid\n", addr_str);
                exit(1);
        }

	/* open and read memory */
	dev_mem_fd = open_dev(base_addr, &e1000e_mem);
	if (dev_mem_fd >= 0 && e1000e_mem) {
		/* printf("LEDCTL 0x%x\n", er32(E1000_LEDCTL)); */
		do {
			print_leds();
			usleep(250);
		} while (readloop);
		printf("\n");
	}

	close(dev_mem_fd);
	munmap((void *)e1000e_mem, MEM_WINDOW_SZ);

	return ret;
}

