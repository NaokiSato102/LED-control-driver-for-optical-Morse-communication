#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/ctype.h>
#include <linux/delay.h>

#define LENGTH 16

MODULE_AUTHOR("Naoki Sato");
MODULE_DESCRIPTION("LED control driver for optical Morse communication");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.2");

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32 *gpio_base = NULL;

static void led_operator(char code){
	int ON_time  = 0;
	int OFF_time = 0;
	switch (code){
	case '.':
		ON_time  = 1;
		OFF_time = 1;
		break;

	case '-':
		ON_time  = 3;
		OFF_time = 1;
		break;	

	case ' ':
		ON_time  = 0;
		OFF_time = 4;
		break;
	}
	gpio_base[10] = 1 << 25;//OFF
	msleep(OFF_time * 100);
	gpio_base[7] = 1 << 25;//ON
	msleep(ON_time  * 100);
}


static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos){
	char c;
	int i, result_num;
	typedef struct s_morse {
		char str_type;
		char code[LENGTH];
	} MORSE;

	//���[���X�������X�g�B�a�����[���X�⑗�M�J�n���X�̗������ɂ͔�Ή��B
	MORSE mo[]={
		{'0' ,"----- "  },// 0
		{'1' ,".---- "  },// 1
		{'2' ,"..--- "  },// 2
		{'3' ,"...-- "  },// 3
		{'4' ,"....- "  },// 4
		{'5' ,"..... "  },// 5
		{'6' ,"-.... "  },// 6
		{'7' ,"--... "  },// 7
		{'8' ,"---.. "  },// 8
		{'9' ,"----. "  },// 9
		{'A' ,".- "     },//10
		{'B' ,"-... "   },//11
		{'C' ,"-.-. "   },//12
		{'D' ,"-.. "    },//13
		{'E' ,". "      },//14
		{'F' ,"..-. "   },//15
		{'G' ,"--. "    },//16
		{' ' ,".... "   },//17
		{'I' ,".. "     },//18
		{'J' ,".--- "   },//19
		{'K' ,"-.- "    },//20
		{'L' ,".-.. "   },//21
		{'M' ,"-- "     },//22
		{'N' ,"-. "     },//23
		{'O' ,"--- "    },//24
		{'P' ,".--. "   },//25
		{'Q' ,"--.- "   },//26
		{'R' ,".-. "    },//27
		{'S' ,"... "    },//28
		{'T' ,"- "      },//29
		{'U' ,"..- "    },//30
		{'V' ,"...- "   },//31
		{'W' ,".-- "    },//32
		{'X' ,"-..- "   },//33
		{'Y' ,"-.-- "   },//34
		{'Z' ,"--.. "   },//35
		{'.' ,".-.-.- " },//36
		{',' ,"--..-- " },//37
		{'?' ,"..--.. " },//38
		{'!' ,"-.-.-- " },//39
		{'-' ,"-....- " },//40
		{'/' ,"-..-. "  },//41
		{'@' ,".--.-. " },//42
		{'(' ,"-.--. "  },//43
		{')' ,"-.--.- " },//44
		{'\"',".-..-. " },//45
		{' ' ," "       },//46
		{'#' ,""        },//47
	};

	if(copy_from_user(&c,buf,sizeof(char) ) ){
		return -EFAULT;
	}
	//��M�L���\��
	printk(KERN_INFO "receive \"%c\"\n",c);

	//�A���t�@�x�b�g����M�����ꍇ
	if     ( isalpha((unsigned char)c) ) {
		//�������X�g�̃A���t�@�x�b�g�����o�́B�Ȃ��A�}�W�b�N�i���o�[�̓��X�g�ւ̃I�t�Z�b�g
		result_num = toupper((unsigned char)c) -'A' +1 -'0' +'9';
	}
	//10�i���̐�������M�����ꍇ
	else if( isdigit((unsigned char)c) ) {
		//�������X�g��10�i���̐��������o�́B�Ȃ��A�}�W�b�N�i���o�[�̓��X�g�ւ̃I�t�Z�b�g
		result_num = c -'0';
	}
	//����ȊO����M�����ꍇ�B
	//ctype�����p���悤�ɂ����[���X�M���Ƃ��ďo�͉\�ȋL����ASCII�R�[�h�Ƃ̑����������Ԃ鈫�������B
	else{
		switch (c){
			case '.' :
				result_num = 36;
				break;
			case ',' :
				result_num = 37;
				break;
			case '?' :
				result_num = 38;
				break;
			case '!' :
				result_num = 39;
				break;
			case '-' :
				result_num = 40;
				break;
			case '/' :
				result_num = 41;
				break;
			case '@' :
				result_num = 42;
				break;
			case '(' :
				result_num = 43;
				break;
			case ')' :
				result_num = 44;
				break;
			case '\"':
				result_num = 45;
				break;
			case ' ':
				result_num = 46;
				break;
			default://�ΏۊO�R�[�h��1�x���Ƃ��ĉ��߂��邱�ƂƂ����B
				result_num = 47;
				break;
		}
	}//else�̊��ʕ�

	printk(KERN_INFO "result_num = \"%d\"\n",result_num);//��M�L���̉��ߌ��ʂ̔ԍ���\��

	for(i=0; mo[result_num].code[i] != '\0' ;i++){
		//���M�i����\��
		printk(KERN_INFO "char\"%c\":no%2d:send %d\n",
			mo[result_num].str_type, i, mo[result_num].code[i]
		);

		led_operator(mo[result_num].code[i]);
	}
	gpio_base[10] = 1 << 25;//OFF
	msleep(300);


	return 1;

}//end of led_write

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.write = led_write
};

static int __init init_mod(void){
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
