#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/ctype.h>
#include <linux/delay.h>

#define LENGTH 32

MODULE_AUTHOR("Naoki Sato");
MODULE_DESCRIPTION("LED control driver for optical Morse communication");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32 *gpio_base = NULL;

static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos){
	char c;
	int i, j, result_num;
	typedef struct s_morse {
		char str_type;
		int size;
		int code[LENGTH]
	} MORSE;

	MORSE mo[]={//モールス符号リスト。和文モールスや送信開始等々の略符号には非対応。
		{'0' , 22, {1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,0 } },// 0
		{'1' , 18, {1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,0         } },// 1
		{'2' , 18, {1,0,1,0,1,1,1,0,1,1,1,0,1,1,1,0,0         } },// 2
		{'3' , 16, {1,0,1,0,1,0,1,1,1,0,1,1,1,0,0             } },// 3
		{'4' , 14, {1,0,1,0,1,0,1,0,1,1,1,0,0                 } },// 4
		{'5' , 12, {1,0,1,0,1,0,1,0,1,0,0                     } },// 5
		{'6' , 14, {1,1,1,0,1,0,1,0,1,0,1,0,0                 } },// 6
		{'7' , 16, {1,1,1,0,1,1,1,0,1,0,1,0,1,0,0             } },// 7
		{'8' , 18, {1,1,1,0,1,1,1,0,1,1,1,0,1,0,1,0,0         } },// 8
		{'9' , 20, {1,1,1,0,1,1,1,0,1,1,1,0,1,1,1,0,1,0,0     } },// 9
		{'A' ,  8, {1,0,1,1,1,0,0                             } },//10
		{'B' , 12, {1,1,1,0,1,0,1,0,1,0,0                     } },//11
		{'C' , 14, {1,1,1,0,1,0,1,1,1,0,1,0,0                 } },//12
		{'D' , 10, {1,1,1,0,1,0,1,0,0                         } },//13
		{'E' ,  4, {1,0,0                                     } },//14
		{'F' , 12, {1,0,1,0,1,1,1,0,1,0,0                     } },//15
		{'G' , 12, {1,1,1,0,1,1,1,0,1,0,0                     } },//16
		{'H' , 10, {1,0,1,0,1,0,1,0,0                         } },//17
		{'I' ,  6, {1,0,1,0,0                                 } },//18
		{'J' , 16, {1,0,1,1,1,0,1,1,1,0,1,1,1,0,0             } },//19
		{'K' , 12, {1,1,1,0,1,0,1,1,1,0,0                     } },//20
		{'L' , 12, {1,0,1,1,1,0,1,0,1,0,0                     } },//21
		{'M' , 10, {1,1,1,0,1,1,1,0,0                         } },//22
		{'N' ,  8, {1,1,1,0,1,0,0                             } },//23
		{'O' , 14, {1,1,1,0,1,1,1,0,1,1,1,0,0                 } },//24
		{'P' , 14, {1,0,1,1,1,0,1,1,1,0,1,0,0                 } },//25
		{'Q' , 16, {1,1,1,0,1,1,1,0,1,0,1,1,1,0,0             } },//26
		{'R' , 10, {1,0,1,1,1,0,1,0,0                         } },//27
		{'S' ,  8, {1,0,1,0,1,0,0                             } },//28
		{'T' ,  6, {1,1,1,0,0                                 } },//29
		{'U' , 10, {1,0,1,0,1,1,1,0,0                         } },//30
		{'V' , 12, {1,0,1,0,1,0,1,1,1,0,0                     } },//31
		{'W' , 12, {1,0,1,1,1,0,1,1,1,0,0                     } },//32
		{'X' , 14, {1,1,1,0,1,0,1,0,1,1,1,0,0                 } },//33
		{'Y' , 16, {1,1,1,0,1,0,1,1,1,0,1,1,1,0,0             } },//34
		{'Z' , 14, {1,1,1,0,1,1,1,0,1,0,1,0,0                 } },//35
		{'.' , 20, {1,0,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,0,0     } },//36
		{',' , 22, {1,1,1,0,1,1,1,0,1,0,1,0,1,1,1,0,1,1,1,0,0 } },//37
		{'?' , 18, {1,0,1,0,1,1,1,0,1,1,1,0,1,0,1,0,0         } },//38
		{'!' , 22, {1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,0,1,1,1,0,0 } },//39
		{'-' , 18, {1,1,1,0,1,0,1,0,1,0,1,0,1,1,1,0,0         } },//40
		{'/' , 16, {1,1,1,0,1,0,1,0,1,1,1,0,1,0,0             } },//41
		{'@' , 20, {1,0,1,1,1,0,1,1,1,0,1,0,1,1,1,0,1,0,0     } },//42
		{'(' , 18, {1,1,1,0,1,0,1,1,1,0,1,1,1,0,1,0,0         } },//43
		{')' , 22, {1,1,1,0,1,0,1,1,1,0,1,1,1,0,1,0,1,1,1,0,0 } },//44
		{'\"', 18, {1,0,1,1,1,0,1,0,1,0,1,1,1,0,1,0,0         } },//45
		{' ' ,  7, {0,0,0,0,0,0,0                             } },//46
		{'#' ,  1, {0                                         } } //47
	};

	if(copy_from_user(&c,buf,sizeof(char) ) ){
		return -EFAULT;
	}
	printk(KERN_INFO "receive \"%c\"\n",c);//受信記号表示

	if     ( isalpha((unsigned char)c) ) {//アルファベットを受信した場合
		result_num = toupper((unsigned char)c) -'A' +1 -'0' +'9';//符号リストのアルファベット項を出力。なお、マジックナンバーはリストへのオフセット
	}
	else if( isdigit((unsigned char)c) ) {//10進数の数字を受信した場合
		result_num = c -'0';//符号リストの10進数の数字項を出力。なお、マジックナンバーはリストへのオフセット
	}
	else{//それ以外を受信した場合。ctypeを活用しようにもモールス信号として出力可能な記号とASCIIコードとの相性がすこぶる悪かった。
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
			case '()' :
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
			default://対象外コードは1休符として解釈することとした。
				result_num = 47;
				break;
		}
	}//elseの括弧閉じ

	printk(KERN_INFO "result_num = \"%d\"\n",result_num);//受信記号の解釈結果の番号を表示

	for(i=0;i<mo[result_num].size;i++){
		printk(KERN_INFO "char\"%c\":no%2d:send %d\n",mo[result_num].str_type, i, mo[result_num].code[i]);//送信進捗を表示
		if(mo[result_num].code[i] ){//コードに沿ってLEDをONにするためにGPIO25番をONに
			gpio_base[7] = 1 << 25;
		}
		else{
			gpio_base[10] = 1 << 25;//OFFにする
		}
		for(j=0;j<100;j++){//udelayを使用したダーティーな実装方法。より良い方法は脳みそが足りなかったので思いつかず。100ms待つ。
			udelay(1000);
		}
	}

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
