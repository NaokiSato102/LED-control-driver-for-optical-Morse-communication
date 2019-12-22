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

MODULE_AUTHOR("Naoki Sato");
MODULE_DESCRIPTION("driver for LED control");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32 *gpio_base = NULL;

static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos)
{
	char c;
	int i;
	typedef struct s_morse {
		char str_type;
		int size;
		int code[LENGTH]
	} MORSE;

	MORSE mo[]={
		{"A", 8,	10111000          },
		{"B", 12,	111010101000      },
		{"C", 13,	1110111101000     },
		{"D", 10,	1110101000        },
		{"E", 5,	10000             },
		{"F", 12,	101011110000      },
		{"G", 11,	11111110000       },
		{"H", 11,	10101010000       },
		{"I", 7,	1010000           },
		{"J", 14,	10111111111000    },
		{"K", 11,	11110111000       },
		{"L", 12,	101111010000      },
		{"M", 9,	111111000         },
		{"N", 8,	11110000          },
		{"O", 12,	111111111000      },
		{"P", 12,	1011111110000     },
		{"Q", 14,	11111110111000    },
		{"R", 10,	1011110000        },
		{"S", 9,	101010000         },
		{"T", 6,	111000            },
		{"U", 10,	1010111000        },
		{"V", 12,	101010111000      },
		{"W", 11,	10111111000       },
		{"X", 13,	1111010111000     },
		{"Y", 14,	11110111111000    },
		{"Z", 13,	1111111010000     },
		{"1", 17,	10111111111111000 },
		{"2", 16,	1010111111111000  },
		{"3", 15,	101010111111000   },
		{"4", 14,	10101010111000    },
		{"5", 13,	1010101010000     },
		{"6", 14,	11110101010000    },
		{"7", 15,	111111101010000   },
		{"8", 16,	1111111111010000  },
		{"9", 17,	11111111111110000 },
		{"0", 18,	111111111111111000}
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
