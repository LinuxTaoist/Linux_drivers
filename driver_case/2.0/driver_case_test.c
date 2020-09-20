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

#include <linux/of.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

//#define DEV_MAJOR  201
#define DEV_MINOR    0
#define ERROR       -1
#define OK           0

#define DRIVER_CASE_NUM          1
#define DRIVER_CASE_CLASS_NAME   "driver_case_class"
#define DRIVER_CASE_DEVICE_NAME  "driver_case"
#define DRIVER_CASE_NAME         "driver_case"
#define PLATFORM_NAME            "platform_driver_case"
#define COMPATABLE_NAME          "dx, driver_case"


//#define DECLARE_DRIVER_CASE_SUPPORT 
#define DEBUG_LOG_SUPPORT

#define PRINT_ERR(format,x...)    \
do{ printk(KERN_ERR "ERROR: func: %s line: %04d info: " format,          \
                                 __func__, __LINE__, ## x); }while(0)
#define PRINT_INFO(format,x...)   \
do{ printk(KERN_INFO "[driver_case]" format, ## x); }while(0)

#ifdef DEBUG_LOG_SUPPORT
#define PRINT_DEBUG(format,x...)  \
do{ printk(KERN_INFO "[driver_case] func: %s line: %d info: " format,    \
                                 __func__, __LINE__, ## x); }while(0)
#else
#define PRINT_DEBUG(format,x...)  
#endif

struct driver_case_platform_data {
    int    gpio_nums;
    char   label[20]; 
    char   date[10];
};

struct driver_case_dev {
    int    major;
    dev_t  devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
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
    PRINT_INFO("Entry %s \n", __func__);
    
    /* 1. 设置设备号 
     * 主设备号已知， 静态注册；未知， 动态注册。
     */
#ifdef DEV_MAJOR
        driver_case.devid = MKDEV(DEV_MAJOR, 0);
        register_chrdev_region(driver_case.devid, DRIVER_CASE_NUM, DRIVER_CASE_NAME);
#else    
        alloc_chrdev_region(&driver_case.devid, 0, DRIVER_CASE_NUM, DRIVER_CASE_NAME);
        driver_case.major = MAJOR(driver_case.devid);    
#endif

    /* 2. 注册驱动结构体 */
    driver_case.cdev.owner = THIS_MODULE;
    cdev_init(&driver_case.cdev, &driver_case_fops);
    cdev_add(&driver_case.cdev, driver_case.devid, DRIVER_CASE_NUM);
    PRINT_DEBUG("driver_case_fops succesful! \n");
    
    /* 3. 创建类 */
    driver_case.class = class_create(THIS_MODULE, DRIVER_CASE_CLASS_NAME);    
    if(IS_ERR(driver_case.class)) {
        PRINT_ERR("%s under class created failed! \n", DRIVER_CASE_DEVICE_NAME);
        
        return ERROR;
    }
    
    /* 4.创建设备 */
    driver_case.device = device_create(driver_case.class, NULL, 
                                    driver_case.devid, NULL, DRIVER_CASE_DEVICE_NAME);
    if(NULL == driver_case.device) {   
        PRINT_ERR("%s device created failed! \n", DRIVER_CASE_DEVICE_NAME); 
        return ERROR;    
    }

    return OK;
}

static struct driver_case_platform_data  *driver_case_parse_dt (
                                              struct device *pdev)
{
    struct driver_case_platform_data *pdata;
    struct device_node *np = pdev->of_node;
    const char *str;

    PRINT_INFO("Entry %s \n", __func__);
    
    pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
    if (!pdata) {
        PRINT_ERR("Could not allocate memory for platform data \n");

        return NULL;
    }
 
	if(of_property_read_u32(np, "gpio_num", &pdata->gpio_nums) < 0){
		PRINT_ERR("Get gpio_num from device tree failed! \n");

		return NULL;
	}    
    PRINT_INFO("gpio_num: %d \n", pdata->gpio_nums);
 
    if(of_property_read_string(np, "label", 
                               &str) < 0) {
    
		PRINT_ERR("Get label from device tree failed! \n");

		return NULL;
    }
    memcpy(pdata->label, str, strlen(str));
    PRINT_INFO("label1: %s \n", pdata->label);

    return pdata;
}

static int driver_case_probe(struct platform_device *pdev)
{
    int ret = -1 ;
    struct driver_case_platform_data *pdata = NULL; // = pdev->dev.platform_data;
    struct driver_case_dev *pdriver_case = &driver_case;    
    PRINT_INFO("Entry %s \n", __func__);

    if (!pdata) {
        PRINT_ERR("could not allocate memory for platform data\n");
        pdata = driver_case_parse_dt(&pdev->dev);
        if(pdata) {
            pdev->dev.platform_data = pdata;
        } else {
            ret = -EINVAL;
            PRINT_ERR("get platform_data NULL\n");
            goto fail_to_get_platform_data;
        }
    }
    pdriver_case->platform_data = pdata;
    
    ret = register_driver();
    if(ERROR == ret ) {
        PRINT_ERR("driver register error! \r\n");
        goto fail_to_get_platform_data;
    }  else {
        PRINT_INFO("driver register successfully \n");
    }

    return 0;
    
fail_to_get_platform_data:
    return ret;
}

int driver_case_remove(struct platform_device *pdev)
{
    cdev_del(&driver_case.cdev);
    unregister_chrdev_region(driver_case.devid, DRIVER_CASE_NUM);    
    device_destroy(driver_case.class, driver_case.devid);
    class_destroy(driver_case.class);

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
    PRINT_INFO("Entry %s \n", __func__);
    platform_driver_register(&driver_case_device_driver);
    
    return 0;
}

static void __exit driver_case_exit(void)
{
    PRINT_INFO("Entry %s \n", __func__);
    platform_driver_unregister(&driver_case_device_driver);
}

module_exit(driver_case_exit);
module_init(driver_case_init);

MODULE_LICENSE("GPL");

