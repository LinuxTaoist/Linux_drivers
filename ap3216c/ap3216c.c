/*
********************************************************************************
* Copyright (C),1999-2020, Jimi IoT Co., Ltd.
* File Name   :   ap3216c.c
* Author      :   dongxiang
* Version     :   V1.0
* Description :   General driver template, if wanting to use it, you can use 
*                 global case matching to replace DRIEVER_CASE and ap3216c 
*                 with your custom driver name.
* Journal     :   2020-05-09 init v1.0 by dongxiang
* Others      :   
********************************************************************************
*/

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "ap3216c.h"

#define DEV_MAJOR 201
#define DEV_MINOR 0
#define DEV_NUM   1
#define ERROR    -1
#define OK        0

#define AP3216C_CLASS_NAME "ap3216c_class"
#define AP3216C_DEVICE_NAME  "ap3216c"
#define AP3216C_NAME "ap3216c"
#define PLATFORM_NAME "platform_ap3216c"
#define COMPATABLE_NAME "100ask,ap3216c"
#define AP3216C_NUM 1

struct sap3216c_dev{
	int    major;
	int    gpio_num;
	dev_t  devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
    void *private_data; 
    unsigned short ir, als, ps;		/* 三个光传感器数据 */
};

struct sap3216c_dev ap3216c_dev;

static int ap3216c_read_regs(struct sap3216c_dev *dev, unsigned char reg, 
                                              void *val, int len)
{
    int ret;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->private_data;
    
    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].buf = &reg;
    msg[0].len = 1;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = val;
    msg[1].len = len;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret ==2 ) {
        ret = 0;    
    } else {
        printk("%s %d: i2c transfer error!\n ", __func__, __LINE__);
        ret = -EREMOTEIO;    
    }    

    return ret;
}

static int ap3216c_write_regs(struct sap3216c_dev *dev, unsigned char reg, 
                                   unsigned char *buf, unsigned char len) 
{
    unsigned char temp_buf[256];
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    temp_buf[0] = reg;
    memcpy(&temp_buf[1], buf, len);
    
    msg.addr = client->addr;
    msg.flags = 0;
    msg.buf = temp_buf;
    msg.len = len+1;

    return i2c_transfer(client->adapter, &msg, 1);
}

static unsigned char ap3216c_read_reg(struct sap3216c_dev *dev, u8 reg)
{
    u8 data = 0;
    int ret = 0;

    ret = ap3216c_read_regs(dev, reg, &data, 1);
    if (ret == 0) {
        return data;    
    } else {
        return -EREMOTEIO;   
    }
} 

static void ap3216c_write_reg(struct sap3216c_dev *dev, u8 reg, 
                                       unsigned char data)
{
    unsigned char buf = 0;
    buf = data;
    ap3216c_write_regs(dev, reg, &buf, 1);
}

void ap3216c_readdata(struct sap3216c_dev *dev)
{
	unsigned char i =0;
    unsigned char buf[6];
	
	/* 循环读取所有传感器数据 */
    for(i = 0; i < 6; i++)	
    {
        buf[i] = ap3216c_read_reg(dev, AP3216C_IRDATALOW + i);	
    }

    if(buf[0] & 0X80) 	/* IR_OF位为1,则数据无效 */
		dev->ir = 0;					
	else 				/* 读取IR传感器的数据   		*/
		dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03); 			
	
	dev->als = ((unsigned short)buf[3] << 8) | buf[2];	/* 读取ALS传感器的数据 			 */  
	
    if(buf[4] & 0x40)	/* IR_OF位为1,则数据无效 			*/
		dev->ps = 0;    													
	else 				/* 读取PS传感器的数据    */
		dev->ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] & 0X0F); 
}

static int ap3216c_open(struct inode *inode, struct file *file)
{
    /* ap3216c初始化 在probe配置也可以 */
    file->private_data = &ap3216c_dev;
    ap3216c_write_reg(&ap3216c_dev, AP3216C_SYSTEMCONG, 0x04);
    mdelay(50);    
    ap3216c_write_reg(&ap3216c_dev, AP3216C_SYSTEMCONG, 0x03);

    return 0;
}

static ssize_t ap3216c_read(struct file *file, char __user *buf, size_t cnt, loff_t *off)
{
    /* 读取I2C设备数据 */
    unsigned short data[3];
    int ret = 0;
    struct sap3216c_dev *dev = (struct sap3216c_dev *)file->private_data;    
    
    ap3216c_readdata(dev);
    
    data[0] = dev->ir;
    data[1] = dev->als;
    data[2] = dev->ps;
    ret = copy_to_user(buf, data, sizeof(data));
    
    return 0;
}

static int ap3216c_release(struct inode *inode, struct file *file)
{
    
    return 0;
}

/* 驱动结构体 */
static struct file_operations ap3216c_fops = {
	.owner = THIS_MODULE,
	.open  = ap3216c_open,
    .read  = ap3216c_read,
    .release = ap3216c_release,
};

static int register_driver(void)
{
    /* 1. 设置设备号 
	 * 主设备号已知， 静态注册；未知， 动态注册。
	 */
	if (ap3216c_dev.major){
		ap3216c_dev.devid = MKDEV(ap3216c_dev.major, 0);
		register_chrdev_region(ap3216c_dev.devid, AP3216C_NUM, AP3216C_NAME);
	} else {
		alloc_chrdev_region(&ap3216c_dev.devid, 0, AP3216C_NUM, AP3216C_NAME);
		ap3216c_dev.major = MAJOR(ap3216c_dev.devid);	
	}

	/* 2. 注册驱动结构体 */
    ap3216c_dev.cdev.owner = THIS_MODULE;
	cdev_init(&ap3216c_dev.cdev, &ap3216c_fops);
	cdev_add(&ap3216c_dev.cdev, ap3216c_dev.devid, AP3216C_NUM);

	/* 3. 创建类 */
	ap3216c_dev.class = class_create(THIS_MODULE, AP3216C_CLASS_NAME);	
	if(IS_ERR(ap3216c_dev.class)) {
		printk("Failed:%s:%d: %s under class created failed! \r\n", 
                __func__, __LINE__, AP3216C_DEVICE_NAME);
        return ERROR;
	}
 	/* 4.创建设备 */
	ap3216c_dev.device = device_create(ap3216c_dev.class, NULL, 
                                    ap3216c_dev.devid, NULL, AP3216C_DEVICE_NAME);
	if(NULL == ap3216c_dev.device) {
	    printk("Failed:%s:%d: %s device created failed! \r\n", 
                __func__, __LINE__,  AP3216C_DEVICE_NAME);	
        return ERROR;	
    }   
    
    return OK;
}

static int ap3216c_probe(struct i2c_client * client, const struct i2c_device_id *id)
{
    int ret = -1 ;
	printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);
	
	ret = register_driver();
    if(ERROR == ret ) {
        printk("Failed:%s:%d: driver register error! \r\n", __func__, __LINE__);
    }  else {
        printk("%s:%d: driver register successfully \r\n", __func__, __LINE__);
    }
    ap3216c_dev.private_data = client;      
	
    return 0;
}

int ap3216c_remove(struct i2c_client *client)
{
	cdev_del(&ap3216c_dev.cdev);
	unregister_chrdev_region(ap3216c_dev.devid, AP3216C_NUM);	
	device_destroy(ap3216c_dev.class, ap3216c_dev.devid);
	class_destroy(ap3216c_dev.class);

    return 0;
}

const struct of_device_id ap3216c_table[] = {
	{
		.compatible = COMPATABLE_NAME,
	},
	{
	},
};

static struct i2c_driver ap3216c_device_driver = {
	.probe  = ap3216c_probe,
	.remove = ap3216c_remove,
	.driver = {
		.name = PLATFORM_NAME,
		.owner = THIS_MODULE,
		.of_match_table = ap3216c_table,
  	},
};


static int __init ap3216c_init(void)
{
    int ret = 0;
    printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);

	ret = i2c_add_driver(&ap3216c_device_driver);
	
	return ret;
}

static void __exit ap3216c_exit(void)
{
	printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);

	i2c_del_driver(&ap3216c_device_driver);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("dongxiang");

