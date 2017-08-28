TARGET = com_chan

KERNEL_DIR := /lib/modules/$(shell uname -r)/build

obj-m += $(TARGET).o

all:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD) modules
clean:
	rm -rf *.o *.ko *.mod.* *.cmd .module* modules* Module* .*.cmd .tmp*
	make -C $(KERNEL_DIR) SUBDIRS=$(PWD) clean
