/*
********************************************************************************
* Copyright (C),1999-2020, Jimi IoT Co., Ltd.
* File Name   :   work.c
* Author      :   dongxiang
* Version     :   V1.0
* Description :   General driver template, if wanting to use it, you can use 
*                 global case matching to replace WORK and work 
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
#include <linux/workqueue.h>

#define DEV_MAJOR 201
#define DEV_MINOR 0
#define DEV_NUM   1
#define ERROR    -1
#define OK        0

#define WORK_CLASS_NAME "work_class"
#define WORK_DEVICE_NAME  "work"
#define WORK_NAME "work"
#define PLATFORM_NAME "platform_work"
#define COMPATABLE_NAME "jimi,work"
#define WORK_NUM 1

//#define DECLARE_WORK_SUPPORT 

struct work_platform_data {
    int    major;
    int    gpio_num;
    dev_t  devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
};

struct  work_dev {
    struct work_platform_data *platform_data;
};

struct work_dev work;

static struct workqueue_struct *  test1_workqueue = NULL;
void test1_callback(struct work_struct *work);

#if defined (DECLARE_WORK_SUPPORT )
static DECLARE_WORK(test1_item, (void *) test1_callback);
#endif

#if !defined(DECLARE_WORK_SUPPORT)
static struct work_struct test1_item;
#endif

void test1_callback(struct work_struct *work) 
{
    printk("test1_callback!");
}


static int work_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t work_write(struct file *file, const char __user *buf, 
                             size_t size, loff_t *offset)
{
    printk("dx_test: work_write");
    queue_work(test1_workqueue, &test1_item);
    return 0;
}

/* 驱动结构体 */
static struct file_operations work_fops = {
    .owner = THIS_MODULE,
    .open  = work_open,
    .write = work_write,
};

static int register_driver(void)
{
    printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);
    /* 1. 设置设备号 
     * 主设备号已知， 静态注册；未知， 动态注册。
     */
#ifdef DEV_MAJOR    
        printk("%s:%d -----------DEV_MAJOR------------", __func__, __LINE__);
        work.platform_data->devid = MKDEV(DEV_MAJOR, 0);
        register_chrdev_region(work.platform_data->devid, WORK_NUM, WORK_NAME);
        printk("%s:%d DEV_MAJOR", __func__, __LINE__);
#else    
        alloc_chrdev_region(&work.platform_data->devid, 0, WORK_NUM, WORK_NAME);
        work.platform_data->major = MAJOR(work.platform_data->devid);    
#endif

    /* 2. 注册驱动结构体 */
    work.platform_data->cdev.owner = THIS_MODULE;
    cdev_init(&work.platform_data->cdev, &work_fops);
    cdev_add(&work.platform_data->cdev, work.platform_data->devid, WORK_NUM);
    printk("Failed:%s:%d work_fops succesful! \r\n", 
                __func__, __LINE__);
    /* 3. 创建类 */
    work.platform_data->class = class_create(THIS_MODULE, WORK_CLASS_NAME);    
    if(IS_ERR(work.platform_data->class)) {
        printk("Failed:%s:%d: %s under class created failed! \r\n", 
                __func__, __LINE__, WORK_DEVICE_NAME);
        return ERROR;
    }
     /* 4.创建设备 */
    work.platform_data->device = device_create(work.platform_data->class, NULL, 
                                    work.platform_data->devid, NULL, WORK_DEVICE_NAME);
    if(NULL == work.platform_data->device) {
        printk("Failed:%s:%d: %s device created failed! \r\n", 
                __func__, __LINE__,  WORK_DEVICE_NAME);    
        return ERROR;    
    }

    return OK;
}

#ifdef CONFIG_OF
static struct work_platform_data  *work_parse_dt(struct device *pdev)
{
    struct work_platform_data *pdata;
    //struct device_node *np = pdev->of_node;

    pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
    if (!pdata) {
        //dev_err(dev, "could not allocate memory for platform data\n");
        return NULL;
    }

    return pdata;
}
#endif

static int work_probe(struct platform_device *pdev)
{
    int ret = -1 ;
    struct work_platform_data *pdata = pdev->dev.platform_data;
    struct work_dev *pwork = &work;
    
    printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);

    if (!pdata) {
        printk("could not allocate memory for platform data\n");
        pdata = work_parse_dt(&pdev->dev);
        if(pdata) {
            pdev->dev.platform_data = pdata;
        }
    }    
    if (!pdata) {
        printk(KERN_WARNING "get platform_data NULL\n");
        ret = -EINVAL;
        goto fail_to_get_platform_data;
    }    
    
    pwork->platform_data = pdata;
    
    ret = register_driver();
    if(ERROR == ret ) {
        printk("Failed:%s:%d: driver register error! \r\n", __func__, __LINE__);
        goto fail_to_get_platform_data;
    }  else {
        printk("%s:%d: driver register successfully \r\n", __func__, __LINE__);
    }
    
#if !defined(DECLARE_WORK_SUPPORT)
    test1_workqueue = create_singlethread_workqueue("test1_wq");    
    INIT_WORK(&test1_item, test1_callback);
#endif

    return 0;
    
fail_to_get_platform_data:
    return ret;
}

int work_remove(struct platform_device *pdev)
{
    cdev_del(&work.platform_data->cdev);
    unregister_chrdev_region(work.platform_data->devid, WORK_NUM);    
    device_destroy(work.platform_data->class, work.platform_data->devid);
    class_destroy(work.platform_data->class);

    return 0;
}

const struct of_device_id work_table[] = {
    {
        .compatible = COMPATABLE_NAME,
    },
    {
    },
};

static struct platform_driver work_device_driver = {
    .probe  = work_probe,
    .remove = work_remove,
    .driver = {
        .name = PLATFORM_NAME,
        .owner = THIS_MODULE,
        .of_match_table = work_table,
    },
};


static int __init work_init(void)
{
    printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);

    platform_driver_register(&work_device_driver);
    
    return 0;
}

static void __exit work_exit(void)
{
    printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);

    platform_driver_unregister(&work_device_driver);
}

module_exit(work_exit);
module_init(work_init);

MODULE_LICENSE("GPL");

