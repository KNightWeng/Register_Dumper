obj-m    := iomem.o

ARCH=arm 
CROSS_COMPILE=arm-bcm2708-linux-gnueabi-
KDIR := /home/knight/projects/raspberry_pi/raspberry_pi_kernel/linux
PWD  := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)

clean: 
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
