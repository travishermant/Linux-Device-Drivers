/*
	Travis Hermant
	ECE373, Spring 2018
	Helper program for switching LEDs
						*/
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint32_t buffer;
int value;
int ret;
int fd;

int main(){

	fd = open("/dev/homework3", O_RDWR);
	if(fd < 0){
		printf("No module found \n");
		return -1;
	}
	
	ret = read(fd, &buffer, sizeof(uint32_t));
	if(ret < 0){
		printf("Reading from device failed\n");
		return -1;
	}
	
	printf("Received %08x from driver\n",buffer);
	
	buffer = 0xe;
	ret = write(fd,&buffer, sizeof(uint32_t));
	if(ret < 0){
		printf("writing to device failed\n");
		return -1;
	}
	printf("Wrote %08x to driver\n",buffer);
	sleep(2);
	buffer = 0xf;
	ret = write(fd,&buffer, sizeof(uint32_t));
	if(ret < 0){
		printf("writing to device failed\n");
		return -1;
	}
	printf("Wrote %08x to driver\n",buffer);
	return 0;
}


