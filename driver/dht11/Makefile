obj-m += dht11.o

KERNELDIR ?= /home/zyp/linux/IMX6ULL/linux/alientek_linux
PWD := $(shell pwd)

all:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNELDIR) M=$(PWD) modules

clean:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KERNELDIR) M=$(PWD) clean
