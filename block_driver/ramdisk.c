/*
********************************************************************************
* File Name   :   ramdisk.c
* Author      :   dongxiang
* Version     :   V1.0
* Description :   Create driver for ramdisk.
* Journal     :   2020-09-17 init v1.0 by dongxiang
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
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

//#define DECLARE_RAMDISK_SUPPORT 
#define DEBUG_LOG_SUPPORT

#define PRINT_ERR(format,x...)    \
do{ printk(KERN_ERR "ERROR: func: %s line: %04d info: " format,          \
                                 __func__, __LINE__, ## x); }while(0)
#define PRINT_INFO(format,x...)   \
do{ printk(KERN_INFO "[ramdisk]" format, ## x); }while(0)

#ifdef DEBUG_LOG_SUPPORT
#define PRINT_DEBUG(format,x...)  \
do{ printk(KERN_INFO "[ramdisk] func: %s line: %d info: " format,    \
                                 __func__, __LINE__, ## x); }while(0)
#else
#define PRINT_DEBUG(format,x...)  
#endif

#define DEVICE_NAME "dx_ramdisk"
#define RAMDISK_SIZE (2 * 1024 * 1024)

struct ramdisk_dev {
    int major;
    char *block_buf;
    spinlock_t lock;
    struct gendisk *gendisk;
    struct request_queue *queue;
};

struct ramdisk_dev ramdisk;

int ramdisk_open(struct block_device *dev, fmode_t mode)
{
	printk("ramdisk open\r\n");
	return 0;
}

void ramdisk_release(struct gendisk *disk, fmode_t mode)
{
	printk("ramdisk release\r\n");
}


int ramdisk_getgeo(struct block_device *dev, struct hd_geometry *geo)
{
    geo->heads = 2;            
    geo->cylinders = 32;    
    geo->sectors = RAMDISK_SIZE / (2 * 32 *512); 
    return 0;
}

static struct block_device_operations ramdisk_fops =
{
	.owner	 = THIS_MODULE,
	.open	 = ramdisk_open,
	.release = ramdisk_release,
	.getgeo  = ramdisk_getgeo,
};

static void ramdisk_transfer(struct request *req)
{	
	unsigned long start = blk_rq_pos(req) << 9;  	
	unsigned long len  = blk_rq_cur_bytes(req);		

	void *buffer = bio_data(req->bio);		
	
	if(rq_data_dir(req) == READ) 		
		memcpy(buffer, ramdisk.block_buf + start, len);
	else if(rq_data_dir(req) == WRITE) 	
		memcpy(ramdisk.block_buf + start, buffer, len);

}


void ramdisk_request(struct request_queue *q)
{
	int err = 0;
	struct request *req;


	req = blk_fetch_request(q);
	while(req != NULL) {

		ramdisk_transfer(req);

		if (!__blk_end_request_cur(req, err))
			req = blk_fetch_request(q);
	}
}

static int __init ramdisk_init(void)
{
    int ret;

    ramdisk.block_buf = kzalloc(RAMDISK_SIZE, GFP_KERNEL);
    if (ramdisk.block_buf == NULL) {
        ret = -ENOMEM;
        goto err; 
    }

    ramdisk.major = register_blkdev(0, DEVICE_NAME);
    if (ramdisk.major < 0) {
        PRINT_ERR("Get major error! \n");
        ret = -ENODEV;
        goto major_err;    
    }
    PRINT_INFO("ramdisk.major: %d \n", ramdisk.major);
    
    ramdisk.gendisk = alloc_disk(3);
    if (!ramdisk.gendisk) {
        PRINT_ERR("Get disk error! \n");
        ret = -EDEADLK;
        goto disk_err;    
    }

    spin_lock_init(&ramdisk.lock);
    ramdisk.queue = blk_init_queue(ramdisk_request, &ramdisk.lock);
    if (!ramdisk.queue) {
        PRINT_ERR("Get queue error! \n");
        goto queue_err;    
    }

    ramdisk.gendisk->major = ramdisk.major;
    ramdisk.gendisk->first_minor = 0;
    ramdisk.gendisk->fops = &ramdisk_fops;
    ramdisk.gendisk->queue = ramdisk.queue;
    sprintf(ramdisk.gendisk->disk_name, "dx_ramdisk");
    set_capacity(ramdisk.gendisk, RAMDISK_SIZE/512);
    
    PRINT_INFO("add disk \n");
    add_disk(ramdisk.gendisk);
    PRINT_INFO("add disk end\n");
    
    return 0;

queue_err:
    put_disk(ramdisk.gendisk);
disk_err:
    unregister_blkdev(ramdisk.major, DEVICE_NAME);
major_err:
    kfree(ramdisk.block_buf);
err:
    return ret;
}

static void __exit ramdisk_exit(void)
{
    PRINT_INFO("Entry %s \n", __func__);

    put_disk(ramdisk.gendisk);
    del_gendisk(ramdisk.gendisk);
    blk_cleanup_queue(ramdisk.queue);
    unregister_blkdev(ramdisk.major, DEVICE_NAME);
    kfree(ramdisk.block_buf);
}

module_init(ramdisk_init);
module_exit(ramdisk_exit);

MODULE_LICENSE("GPL");

