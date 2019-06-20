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

/* opening the homework2 module with read and write permissions */
int fd = open("/dev/homework2", O_RDWR);
if(fd < 0){
	printf("No module found \n");
	return -1;
}

/* reading syscall_val from the module, error checking */
value = read(fd,buffer,sizeof(int));
memcpy(&value, buffer, sizeof(int));

/* Displaying initial syscall_val, prompting for new value to update it with */ 
printf("Char device is sending %d\nEnter a new value for the driver\n",value);
scanf("%s",buffer);

/* Writing to the module and updating syscall_val */
ret = write(fd,buffer,sizeof(int));
if(ret < 0){
	printf("Failed to write to device\n");
	return -1;
}

/* Rereading from the module to see syscall_val updated */
ret = read(fd,buffer,sizeof(int));
if(ret < 0){
	printf("Failed to read back new value\n");
	return -1;
}

/* Displaying syscall_val with our inputted value */
memcpy(&value, buffer, sizeof(int));
printf("New number from device is %d\n",value);

return 0;
}
