KERNEL_DIR = /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
obj-m += homework2.o

default:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
clean:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) clean
