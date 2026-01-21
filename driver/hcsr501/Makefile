obj-m += sr501.o

KDIR := /home/zyp/linux/IMX6ULL/linux/alientek_linux    # 修改为你的内核源码目录

PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
