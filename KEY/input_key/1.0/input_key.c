/*
********************************************************************************
* Copyright (C),1999-2020, Jimi IoT Co., Ltd.
* File Name   :   input_key.c
* Author      :   dongxiang
* Version     :   V1.0
* Description :   Adding gpio keys by input subsystem, and all GPIO keys under 
*                 the device tree node are added at one time
* Journal     :   2020-05-14 init v1.0 by dongxiang
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
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/regulator/consumer.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#define DEV_MAJOR 201
#define DEV_MINOR 0
#define DEV_NUM   1
#define ERROR    -1
#define OK        0

#define INPUT_KEY_CLASS_NAME "input_key_class"
#define INPUT_KEY_DEVICE_NAME  "input_key"
#define INPUT_KEY_NAME "input_key"
#define PLATFORM_NAME "platform_input_key"
#define COMPATABLE_NAME "100ask,test"
#define INPUT_KEY_NUM 1

/* 中断IO描述结构体 */
struct sirq_keydesc {
    int gpionum;                            /* gpio标号 */
    int irqnum;                             /* 中断号     */
    unsigned int value;                     /* 按键对应的键值 */
    char name[10];                          /* 名字 */
    irqreturn_t (*handler)(int, void *);    /* 中断服务函数 */
};

/* input_key 设备结构体 */
struct sinput_key_dev{
    int          count;
    struct sirq_keydesc  *irq_keydesc;
    struct       device_node *nd;
    struct input_dev *inputdev;             /* input结构体 */
};

struct sinput_key_dev input_key_dev;

static irqreturn_t gpio_key_handler(int irq, void *dev_id)
{
    int key_val = -1;
    struct sinput_key_dev *dev = (struct sinput_key_dev *)dev_id;
    disable_irq_nosync(irq);
    printk("Enter %s \n", __func__);
    
    key_val = dev->irq_keydesc->value;
    input_report_key(dev->inputdev, dev->irq_keydesc->value, 0);
    input_sync(dev->inputdev);
    
    enable_irq(irq);
    return IRQ_RETVAL(IRQ_HANDLED); 
}

static int input_key_probe(struct platform_device *pdev)
{
    int ret = -1 ;
    int i = 0;
    printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);
    

    /* 读取设备树  */
#if 0
    /* 通过compatible值匹配设备节点 */
    input_key_dev->nd = of_find_compatible_node();

#else
    /* 通过绝对路径查找设备节点 */
    input_key_dev.nd = of_find_node_by_path("/input_key");
    if (NULL == input_key_dev.nd ) {
        printk("Failed:%s %d: 100ask_test node not find! \r\n", 
               __func__, __LINE__);
    } else {
        printk("%s %d: 100ask_test node find! \r\n",__func__, __LINE__);
    }
#endif
    
    input_key_dev.count = of_gpio_count(input_key_dev.nd);
    if (0 == input_key_dev.count) {
        printk("Failed:%s:%d: gpio count not find! \r\n", __func__, __LINE__);
    } else {
        printk("%s %d: gpio count is %d \r\n",
               __func__, __LINE__, input_key_dev.count);
    }
    
    input_key_dev.irq_keydesc = kzalloc(sizeof(input_key_dev.irq_keydesc) 
                                * input_key_dev.count, GFP_KERNEL);
    for (i = 0; i < input_key_dev.count; i++) {
        input_key_dev.irq_keydesc[i].gpionum = 
                      of_get_named_gpio(input_key_dev.nd, "gpios", i);
        of_property_read_u32(input_key_dev.nd, "key_code", 
                             &input_key_dev.irq_keydesc[i].value);
        if(input_key_dev.irq_keydesc[i].gpionum < 0){
            printk("<%d> %s Error: not get %d gpio_num! \r\n", 
                                                __LINE__, __func__, i);
 
            /* 获取失败gpio序数和总数减1, 保证gpio序号连续 */
            --i;
            --input_key_dev.count; 
        } else {
            printk("<%d> %s get %d:gpio_num: %d! \r\n", __LINE__, __func__, i, input_key_dev.irq_keydesc[i].gpionum);        
        }
    }
    
    for (i = 0; i < input_key_dev.count; i++) {
        memset(input_key_dev.irq_keydesc[i].name, 0, sizeof(input_key_dev.irq_keydesc[i].name));
        sprintf(input_key_dev.irq_keydesc[i].name, "KEY%d", i);
        gpio_request(input_key_dev.irq_keydesc[i].gpionum, input_key_dev.irq_keydesc[i].name);
        input_key_dev.irq_keydesc[i].irqnum = gpio_to_irq(input_key_dev.irq_keydesc[i].gpionum);
        input_key_dev.irq_keydesc[i].handler = gpio_key_handler;
        
    }
        
    for (i = 0; i < input_key_dev.count; i++) {
        ret = request_irq(input_key_dev.irq_keydesc[i].irqnum, gpio_key_handler, 
                          IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, 
                          input_key_dev.irq_keydesc[i].name, 
                          &input_key_dev);
        if(ret < 0){
            printk("irq %d request failed!\r\n", input_key_dev.irq_keydesc[i].irqnum);
            return ERROR;
        }
    }
    
    input_key_dev.inputdev = input_allocate_device();
    input_key_dev.inputdev->name = INPUT_KEY_NAME;
    
    /* 初始化input_dev，设置产生哪些事件 */
    __set_bit(EV_KEY, input_key_dev.inputdev->evbit);    /* 设置产生按键事件          */
    __set_bit(EV_REP, input_key_dev.inputdev->evbit);    /* 重复事件，比如按下去不放开，就会一直输出信息          */

    /* 初始化input_dev，设置产生哪些按键 */
    __set_bit(KEY_0, input_key_dev.inputdev->keybit);    
    
    ret = input_register_device(input_key_dev.inputdev);
    if (ret) {
        printk("register input device failed!\r\n");
        return ret;
    }
    
    return 0;
}

int input_key_remove(struct platform_device *pdev)
{
    int i;

    for (i = 0; i < input_key_dev.count; i++) {
        free_irq(input_key_dev.irq_keydesc[i].irqnum, &input_key_dev);
    }

    input_unregister_device(input_key_dev.inputdev);
    input_free_device(input_key_dev.inputdev);

    return 0;
}

const struct of_device_id input_key_table[] = {
    {
        .compatible = COMPATABLE_NAME,
    },
    {
    },
};

static struct platform_driver input_key_device_driver = {
    .probe  = input_key_probe,
    .remove = input_key_remove,
    .driver = {
        .name = PLATFORM_NAME,
        .owner = THIS_MODULE,
        .of_match_table = input_key_table,
      },
};


static int __init input_key_init(void)
{
    printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);
    
    platform_driver_register(&input_key_device_driver);
    
    return 0;
}

static void __exit input_key_exit(void)
{
    printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);
    
    platform_driver_unregister(&input_key_device_driver);
}

module_exit(input_key_exit);
module_init(input_key_init);

MODULE_LICENSE("GPL");

