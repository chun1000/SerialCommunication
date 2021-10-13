KERNEL_DIR = /lib/modules/4.19.66-v7+/build

obj-m := dev.o

PWD := $(shell pwd)

all:
	make -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) M=$(PWD) clean
