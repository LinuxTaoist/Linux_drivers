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
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define DEV_MAJOR 201
#define DEV_MINOR 0
#define DEV_NUM   1
#define ERROR    -1
#define OK        0

#define DRIVER_CASE_CLASS_NAME "driver_case_class"
#define DRIVER_CASE_DEVICE_NAME  "driver_case"
#define DRIVER_CASE_NAME "driver_case"
#define PLATFORM_NAME "platform_driver_case"
#define COMPATABLE_NAME "jimi,driver_case"
#define DRIVER_CASE_NUM 1

//#define DECLARE_DRIVER_CASE_SUPPORT 

struct driver_case_platform_data {
    int    major;
    int    gpio_num;
    dev_t  devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
};

struct  driver_case_dev {
    struct driver_case_platform_data *platform_data;
};

struct driver_case_dev driver_case;

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
    printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);
    /* 1. 设置设备号 
     * 主设备号已知， 静态注册；未知， 动态注册。
     */
#ifdef DEV_MAJOR    
        printk("%s:%d -----------DEV_MAJOR------------", __func__, __LINE__);
        driver_case.platform_data->devid = MKDEV(DEV_MAJOR, 0);
        register_chrdev_region(driver_case.platform_data->devid, DRIVER_CASE_NUM, DRIVER_CASE_NAME);
        printk("%s:%d DEV_MAJOR", __func__, __LINE__);
#else    
        alloc_chrdev_region(&driver_case.platform_data->devid, 0, DRIVER_CASE_NUM, DRIVER_CASE_NAME);
        driver_case.platform_data->major = MAJOR(driver_case.platform_data->devid);    
#endif

    /* 2. 注册驱动结构体 */
    driver_case.platform_data->cdev.owner = THIS_MODULE;
    cdev_init(&driver_case.platform_data->cdev, &driver_case_fops);
    cdev_add(&driver_case.platform_data->cdev, driver_case.platform_data->devid, DRIVER_CASE_NUM);
    printk("Failed:%s:%d driver_case_fops succesful! \r\n", 
                __func__, __LINE__);
    /* 3. 创建类 */
    driver_case.platform_data->class = class_create(THIS_MODULE, DRIVER_CASE_CLASS_NAME);    
    if(IS_ERR(driver_case.platform_data->class)) {
        printk("Failed:%s:%d: %s under class created failed! \r\n", 
                __func__, __LINE__, DRIVER_CASE_DEVICE_NAME);
        return ERROR;
    }
     /* 4.创建设备 */
    driver_case.platform_data->device = device_create(driver_case.platform_data->class, NULL, 
                                    driver_case.platform_data->devid, NULL, DRIVER_CASE_DEVICE_NAME);
    if(NULL == driver_case.platform_data->device) {
        printk("Failed:%s:%d: %s device created failed! \r\n", 
                __func__, __LINE__,  DRIVER_CASE_DEVICE_NAME);    
        return ERROR;    
    }

    return OK;
}

#ifdef CONFIG_OF
static struct driver_case_platform_data  *driver_case_parse_dt(struct device *pdev)
{
    struct driver_case_platform_data *pdata;
    //struct device_node *np = pdev->of_node;

    pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
    if (!pdata) {
        //dev_err(dev, "could not allocate memory for platform data\n");
        return NULL;
    }

    return pdata;
}
#endif

static int driver_case_probe(struct platform_device *pdev)
{
    int ret = -1 ;
    struct driver_case_platform_data *pdata = pdev->dev.platform_data;
    struct driver_case_dev *pdriver_case = &driver_case;
    
    printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);

    if (!pdata) {
        printk("could not allocate memory for platform data\n");
        pdata = driver_case_parse_dt(&pdev->dev);
        if(pdata) {
            pdev->dev.platform_data = pdata;
        }
    }    
    if (!pdata) {
        printk(KERN_WARNING "get platform_data NULL\n");
        ret = -EINVAL;
        goto fail_to_get_platform_data;
    }    
    
    pdriver_case->platform_data = pdata;
    
    ret = register_driver();
    if(ERROR == ret ) {
        printk("Failed:%s:%d: driver register error! \r\n", __func__, __LINE__);
        goto fail_to_get_platform_data;
    }  else {
        printk("%s:%d: driver register successfully \r\n", __func__, __LINE__);
    }

    return 0;
    
fail_to_get_platform_data:
    return ret;
}

int driver_case_remove(struct platform_device *pdev)
{
    cdev_del(&driver_case.platform_data->cdev);
    unregister_chrdev_region(driver_case.platform_data->devid, DRIVER_CASE_NUM);    
    device_destroy(driver_case.platform_data->class, driver_case.platform_data->devid);
    class_destroy(driver_case.platform_data->class);

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

