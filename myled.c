#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#if LINUX_VERSION_CODE < 0x020100
#define schedule_timeout(a){current->timeout = jiffies + (a); schedule();}
#endif

#define LENGTH 32

#include <linux/ctype.h>

MODULE_AUTHOR("Naoki Sato");
MODULE_DESCRIPTION("driver for LED control");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");



static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32 *gpio_base = NULL;

static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos){
	char c;
	int i;
	typedef struct s_morse {
		char str_type;
		int size;
		int code[LENGTH]
	} MORSE;

	MORSE mo[]={
		{"A",  8, {1,0,1,1,1,0,0,0                     } },
		{"B", 12, {1,1,1,0,1,0,1,0,1,0,0,0             } },
		{"C", 13, {1,1,1,0,1,1,1,1,0,1,0,0,0           } },
		{"D", 10, {1,1,1,0,1,0,1,0,0,0                 } },
		{"E",  5, {1,0,0,0,0                           } },
		{"F", 12, {1,0,1,0,1,1,1,1,0,0,0,0             } },
		{"G", 11, {1,1,1,1,1,1,1,0,0,0,0               } },
		{"H", 11, {1,0,1,0,1,0,1,0,0,0,0               } },
		{"I",  7, {1,0,1,0,0,0,0                       } },
		{"J", 14, {1,0,1,1,1,1,1,1,1,1,1,0,0,0         } },
		{"K", 11, {1,1,1,1,0,1,1,1,0,0,0               } },
		{"L", 12, {1,0,1,1,1,1,0,1,0,0,0,0             } },
		{"M",  9, {1,1,1,1,1,1,0,0,0                   } },
		{"N",  8, {1,1,1,1,0,0,0,0                     } },
		{"O", 12, {1,1,1,1,1,1,1,1,1,0,0,0             } },
		{"P", 12, {1,0,1,1,1,1,1,1,1,0,0,0,0           } },
		{"Q", 14, {1,1,1,1,1,1,1,0,1,1,1,0,0,0         } },
		{"R", 10, {1,0,1,1,1,1,0,0,0,0                 } },
		{"S",  9, {1,0,1,0,1,0,0,0,0                   } },
		{"T",  6, {1,1,1,0,0,0                         } },
		{"U", 10, {1,0,1,0,1,1,1,0,0,0                 } },
		{"V", 12, {1,0,1,0,1,0,1,1,1,0,0,0             } },
		{"W", 11, {1,0,1,1,1,1,1,1,0,0,0               } },
		{"X", 13, {1,1,1,1,0,1,0,1,1,1,0,0,0           } },
		{"Y", 14, {1,1,1,1,0,1,1,1,1,1,1,0,0,0         } },
		{"Z", 13, {1,1,1,1,1,1,1,0,1,0,0,0,0           } },
		{"1", 17, {1,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0   } },
		{"2", 16, {1,0,1,0,1,1,1,1,1,1,1,1,1,0,0,0     } },
		{"3", 15, {1,0,1,0,1,0,1,1,1,1,1,1,0,0,0       } },
		{"4", 14, {1,0,1,0,1,0,1,0,1,1,1,0,0,0         } },
		{"5", 13, {1,0,1,0,1,0,1,0,1,0,0,0,0           } },
		{"6", 14, {1,1,1,1,0,1,0,1,0,1,0,0,0,0         } },
		{"7", 15, {1,1,1,1,1,1,1,0,1,0,1,0,0,0,0       } },
		{"8", 16, {1,1,1,1,1,1,1,1,1,1,0,1,0,0,0,0     } },
		{"9", 17, {1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0   } },
		{"0", 18, {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0 } }
	};

	if(copy_from_user(&c,buf,sizeof(char)))
		return -EFAULT;

	
/*

	if(c == '0')
		gpio_base[10] = 1 << 25;
	else if(c == '1')
		gpio_base[7] = 1 << 25;
*/
	printk(KERN_INFO "receive %c\n",c);



	for(i=0;i<(c-'0');i++ ){
		printk(KERN_INFO "%c:%d",c,i);
	}
	return 1;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.write = led_write
};

static int __init init_mod(void)
{
	int retval;

	gpio_base = ioremap_nocache(0x3f200000, 0xA0);
	//0x3f200000: base address, 0xA0: region to map

	const u32 led = 25;
	const u32 index = led/10;//GPFSEL2
	const u32 shift = (led%10)*3;//15bit
	const u32 mask = ~(0x7 << shift);//11111111111111000111111111111111
	gpio_base[index] = (gpio_base[index] & mask) | (0x1 << shift);//001: output flag

	retval =  alloc_chrdev_region(&dev, 0, 1, "myled");
	if(retval < 0){
		printk(KERN_ERR "alloc_chrdev_region faimyled.\n");
		return retval;
	}

	printk(KERN_INFO "%s is loaded. major:%d\n",__FILE__,MAJOR(dev));

	cdev_init(&cdv, &led_fops);
	cdv.owner = THIS_MODULE;
	retval = cdev_add(&cdv, dev, 1);
	if(retval < 0){
		printk(KERN_ERR "cdev_add failed. major:%d, minor:%d\n",MAJOR(dev),MINOR(dev));
		return retval;
	}

	cls = class_create(THIS_MODULE,"myled");
	if(IS_ERR(cls)){
		printk(KERN_ERR "class_create failed.");
		return PTR_ERR(cls);
	}
	device_create(cls, NULL, dev, NULL, "myled%d",MINOR(dev)); 

	return 0;
}

static void __exit cleanup_mod(void)
{
	cdev_del(&cdv);
	device_destroy(cls, dev); 
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "%s is unloaded. major:%d\n",__FILE__,MAJOR(dev));
}

module_init(init_mod);
module_exit(cleanup_mod);
