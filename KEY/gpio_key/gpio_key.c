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
#include <linux/irq.h> 
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
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
#define DRIVEROPS_SUPPORT 1
#define ERROR -1
#define OK    0

#define DRIVER_CLASS_NAME "plat_key"
#define DRIVER_DEVICE_NAME  "plat1"
#define PLATDRV_NAME "driver_case"
#define PLATFORM_NAME "platform_keys"
#define COMPATABLE_NAME "100as,test"
#define PLATDRV_NUM 1

struct sdriver_dev{
	int    major;
	int    gpio_num;
	int    irqnum;
    dev_t  devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
    irqreturn_t (*handler)(int, void *);
};

struct sdriver_dev plat_dev;

#if DRIVEROPS_SUPPORT
static int platdrv_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t platdrv_write(struct file *file, const char __user *buf, 
                             size_t size, loff_t *offset)
{
    return 0;
}

/* 驱动结构体 */
static struct file_operations platdrv_fops = {
	.owner = THIS_MODULE,
	.open  = platdrv_open,
	.write = platdrv_write,
};
#endif

static irqreturn_t key_handler(int irq, void *dev_id)
{
    printk("Enter key_handler \r\n");
    return IRQ_RETVAL(IRQ_HANDLED);
}

static int register_driver(void)
{
    /* 1. 设置设备号 
	 * 主设备号已知， 静态注册；未知， 动态注册。
	 */
	if (plat_dev.major){
		plat_dev.devid = MKDEV(plat_dev.major, 0);
		register_chrdev_region(plat_dev.devid, PLATDRV_NUM, PLATDRV_NAME);
	} else {
		alloc_chrdev_region(&plat_dev.devid, 0, PLATDRV_NUM, PLATDRV_NAME);
		plat_dev.major = MAJOR(plat_dev.devid);	
	}

	/* 2. 注册驱动结构体 */
    plat_dev.cdev.owner = THIS_MODULE;
	cdev_init(&plat_dev.cdev, &platdrv_fops);
	cdev_add(&plat_dev.cdev, plat_dev.devid, PLATDRV_NUM);

	/* 3. 创建类 */
	plat_dev.class = class_create(THIS_MODULE, DRIVER_CLASS_NAME);	
	if(IS_ERR(plat_dev.class)) {
		printk("Failed:%s:%d: %s under class created failed! \r\n", 
                __func__, __LINE__, DRIVER_DEVICE_NAME);
        return ERROR;
	}
 	/* 4.创建设备 */
	plat_dev.device = device_create(plat_dev.class, NULL, 
                                    plat_dev.devid, NULL, DRIVER_DEVICE_NAME);
	if(NULL == plat_dev.device) {
	    printk("Failed:%s:%d: %s device created failed! \r\n", 
                __func__, __LINE__,  DRIVER_DEVICE_NAME);	
        return ERROR;	
    }

    return 0;
}

static int imx6ull_probe(struct platform_device *pdev)
{
    int ret ;
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
    plat_dev->nd = of_find_compatible_node();
#else
    /* 通过绝对路径查找设备节点 */
    plat_dev.nd = of_find_node_by_path("/input_key");

    if(NULL == plat_dev.nd ) {
        printk("Failed:%s:%d: 100ask_test node not find! \r\n", __func__, __LINE__);
    }  else {
        printk("%s:%d: 100ask_test node find! \r\n", __func__, __LINE__);
    }
#endif
#if 0
    plat_property = of_find_property(plat_dev.nd, "compatible", NULL);
    if(NULL == plat_property ) {
        printk("Failed:%s:%d: compatible property  not find! \r\n", __func__, __LINE__);
    }  else {
        printk("%s:%d: compatible property value is %s. \r\n", __func__, __LINE__, (char *)plat_property->value);
    }
    sprintf(all_properties, "%s \r\n", (char *)plat_dev.nd);
    printk("Print all  propertise of 100_test :  %s ", plat_dev.nd->child->name );
#endif
     
    plat_dev.gpio_num = of_get_named_gpio(plat_dev.nd, "key-gpio", 0);
    if(plat_dev.gpio_num < 0){
        printk("Failed:%s:%d: can't get keynum \r\n", __func__, __LINE__);
    } else {
        printk("<%s> %d Get keynum is %d \r\n", __func__, __LINE__, plat_dev.gpio_num);
    }
   
    gpio_request(plat_dev.gpio_num, "key");
    gpio_direction_input(plat_dev.gpio_num);
    plat_dev.irqnum = irq_of_parse_and_map(plat_dev.nd, 0);
    plat_dev.handler = key_handler;

    ret = request_irq(plat_dev.irqnum, key_handler, 
                      IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "key_eint", &plat_dev);

    return 0;
}

int imx6ull_remove(struct platform_device *pdev)
{
	cdev_del(&plat_dev.cdev);
	unregister_chrdev_region(plat_dev.devid, PLATDRV_NUM);	
	device_destroy(plat_dev.class, plat_dev.devid);
	class_destroy(plat_dev.class);

    return 0;
}

const struct of_device_id platform_table[] = {
	{
		.compatible = COMPATABLE_NAME,
	},
	{
	},
};

static struct platform_driver imx6ull_platform_key = {
	.probe  = imx6ull_probe,
	.remove = imx6ull_remove,
	.driver = {
		.name = PLATFORM_NAME,
		.owner = THIS_MODULE,
		.of_match_table = platform_table,
  	},
};


static int __init imx6ull_key_init(void)
{
	printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);

	platform_driver_register(&imx6ull_platform_key);
	
	return 0;
}

static void __exit imx6ull_key_exit(void)
{
	printk("%s:%d: Entry %s \r\n", __FILE__, __LINE__, __func__);

	platform_driver_unregister(&imx6ull_platform_key);
}

module_exit(imx6ull_key_exit);
module_init(imx6ull_key_init);

MODULE_LICENSE("GPL");

