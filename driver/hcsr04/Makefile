obj-m := hcsr04.o

KDIR := /home/zyp/linux/IMX6ULL/linux/alientek_linux
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
