#KERN_DIR := /home/dx/projects/imx-linux4.9.88
# 在环境变量中将内核源码路径添加到 imx6ull_kernel

KERN_DIR := $(imx6ull_kernel)
all:
	make -C $(KERN_DIR) M=`pwd` modules 
	#$(CROSS_COMPILE)gcc -o   

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order
	rm -f ramdisk

obj-m	+= ramdisk.o
