/*
********************************************************************************
* Copyright (C),1999-2020, Jimi IoT Co., Ltd.
* File Name   :   driver_case.c
* Author      :   dongxiang
* Version     :   V1.0
* Description :   General driver template, if wanting to use it, you can use 
*                 global case matching to replace DRIVER_CASE and driver_case 
*                 with your custom driver name.
* Journal     :   2020-05-09 init v1.0 by dongxiang
* Others      :   
********************************************************************************
*/

#include <linux/cdev.h>
#include <linux/circ_buf.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/genalloc.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mxc_mlb.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/regulator/consumer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>

#define DEV_MAJOR 201
#define DEV_MINOR 0
#define DEV_NUM   1
#define ERROR    -1
#define OK        0

#define DRIVER_CASE_CLASS_NAME "driver_case_class"
#define DRIVER_CASE_DEVICE_NAME  "driver_case"
#define DRIVER_CASE_NAME "driver_case"
#define PLATFORM_NAME "platform_driver_case"
#define COMPATABLE_NAME "myself,driver_case"
#define DRIVER_CASE_NUM 1

struct sdriver_dev{
	int    major;
	int    gpio_num;
	dev_t  devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
};

struct sdriver_dev driver_case_dev;

static int driver_case_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t driver_case_write(struct file *file, const char __user *buf, 
                             size_t size, loff_t *offset)
{
    return 0;
}

/* 驱动结构体 */
static struct file_operations driver_case_fops = {
	.owner = THIS_MODULE,
	.open  = driver_case_open,
	.write = driver_case_write,
};

static int register_driver(void)
{
    /* 1. 设置设备号 
	 * 主设备号已知， 静态注册；未知， 动态注册。
	 */
	if (driver_case_dev.major){
		driver_case_dev.devid = MKDEV(driver_case_dev.major, 0);
		register_chrdev_region(driver_case_dev.devid, DRIVER_CASE_NUM, DRIVER_CASE_NAME);
	} else {
		alloc_chrdev_region(&driver_case_dev.devid, 0, DRIVER_CASE_NUM, DRIVER_CASE_NAME);
		driver_case_dev.major = MAJOR(driver_case_dev.devid);	
	}

	/* 2. 注册驱动结构体 */
    driver_case_dev.cdev.owner = THIS_MODULE;
	cdev_init(&driver_case_dev.cdev, &driver_case_fops);
	cdev_add(&driver_case_dev.cdev, driver_case_dev.devid, DRIVER_CASE_NUM);

	/* 3. 创建类 */
	driver_case_dev.class = class_create(THIS_MODULE, DRIVER_CASE_CLASS_NAME);	
	if(IS_ERR(driver_case_dev.class)) {
		printk("Failed:%s:%d: %s under class created failed! \r\n", 
                __func__, __LINE__, DRIVER_CASE_DEVICE_NAME);
        return ERROR;
	}
 	/* 4.创建设备 */
	driver_case_dev.device = device_create(driver_case_dev.class, NULL, 
                                    driver_case_dev.devid, NULL, DRIVER_CASE_DEVICE_NAME);
	if(NULL == driver_case_dev.device) {
	    printk("Failed:%s:%d: %s device created failed! \r\n", 
                __func__, __LINE__,  DRIVER_CASE_DEVICE_NAME);	
        return ERROR;	
    }

    return OK;
}

static int driver_case_probe(struct platform_device *pdev)
{
    int ret = -1 ;
	//struct device_node *node = NULL;
    struct property *plat_property;
    char all_properties[100];
	printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);
	
	ret = register_driver();
    if(ERROR == ret ) {
        printk("Failed:%s:%d: driver register error! \r\n", __func__, __LINE__);
    }  else {
        printk("%s:%d: driver register successfully \r\n", __func__, __LINE__);
    }
	
	/* 5.硬件初始化 读取设备数 */
/* 读取设备树  */
#if 0
    /* 通过compatible值匹配设备节点 */
    driver_case_dev->nd = of_find_compatible_node();
#else
    /* 通过绝对路径查找设备节点 */
    driver_case_dev.nd = of_find_node_by_path("/myself_test");

    if(NULL == driver_case_dev.nd ) {
        printk("Failed:%s:%d: myself_test node not find! \r\n", __func__, __LINE__);
    }  else {
        printk("%s:%d: myself_test node find! \r\n", __func__, __LINE__);
    }
#endif

    plat_property = of_find_property(driver_case_dev.nd, "compatible", NULL);
    if(NULL == plat_property ) {
        printk("Failed:%s:%d: compatible property  not find! \r\n", __func__, __LINE__);
    }  else {
        printk("%s:%d: compatible property value is %s. \r\n", __func__, __LINE__, (char *)plat_property->value);
    }
    sprintf(all_properties, "%s \r\n", (char *)driver_case_dev.nd);
    printk("Print all  propertise of 100_test :  %s ", driver_case_dev.nd->child->name );
	return 0;
}

int driver_case_remove(struct platform_device *pdev)
{
	cdev_del(&driver_case_dev.cdev);
	unregister_chrdev_region(driver_case_dev.devid, DRIVER_CASE_NUM);	
	device_destroy(driver_case_dev.class, driver_case_dev.devid);
	class_destroy(driver_case_dev.class);

    return 0;
}

const struct of_device_id driver_case_table[] = {
	{
		.compatible = COMPATABLE_NAME,
	},
	{
	},
};

static struct platform_driver driver_case_device_driver = {
	.probe  = driver_case_probe,
	.remove = driver_case_remove,
	.driver = {
		.name = PLATFORM_NAME,
		.owner = THIS_MODULE,
		.of_match_table = driver_case_table,
  	},
};


static int __init driver_case_init(void)
{
	printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);

	platform_driver_register(&driver_case_device_driver);
	
	return 0;
}

static void __exit driver_case_exit(void)
{
	printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);

	platform_driver_unregister(&driver_case_device_driver);
}

module_exit(driver_case_exit);
module_init(driver_case_init);

MODULE_LICENSE("GPL");

