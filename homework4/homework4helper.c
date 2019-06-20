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

int buffer;
int value;
int ret;
int fd;

int main(){

	fd = open("/dev/homework4", O_RDWR);
	if(fd < 0){
		printf("No module found \n");
		return -1;
	}
	
	ret = read(fd, &buffer, sizeof(int));
	if(ret < 0){
		printf("Reading from device failed\n");
		return -1;
	}
	
	printf("Received %dx from driver\n",buffer);
	sleep(10);
	printf("Set the blinking rate: ");
	scanf("%d",&buffer);

	ret = write(fd,&buffer, sizeof(int));
	if(ret < 0){
		printf("writing to device failed\n");
		return -1;
	}

	printf("Wrote %d to driver\n",buffer);
	//ret = write(fd,&buffer, sizeof(int));
	sleep(10);
}



