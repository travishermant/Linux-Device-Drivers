/*
	Travis Hermant
	ECE373, Spring 2018
	Helper program for character device module
						*/
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char buffer[100];
int value;
int ret;


int main(){

int fd = open("/dev/homework2", O_RDWR);
if(fd < 0){
	printf("No module found \n");
	return -1;
}

value = read(fd,buffer,sizeof(int));
memcpy(&value, buffer, sizeof(int));
printf("Char device is sending %d\nEnter a new value for the driver\n",value);
//printf("Enter a new value for the char driver: ");

ret = read(0,buffer,sizeof(int));
if(ret < 0){
	printf("Reading from user failed\n");
	return -1;
}

ret = write(fd,buffer,sizeof(int));
if(ret < 0){
	printf("Failed to write to device\n");
	return -1;
}

ret = read(fd,buffer,sizeof(int));
if(ret < 0){
	printf("Failed to read back new value\n");
	return -1;
}


//memcpy(&value, buffer, sizeof(int));
value = atoi(buffer);
printf("New number from device is %d\n",value);

close(fd);
return 0;
}
