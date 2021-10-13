#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/mach/map.h>
#include <asm/io.h>
#include <asm/uaccess.h>


#define CHAT_MAJOR_NUMBER 502
#define CHAT_DEV_NAME "chat_dev"

#define GPIO_BASE_ADDR 0x3F200000
#define GPFSEL1 0X04 //000 input 001 output
#define GPSET0 0x1c
#define GPLEV0 0x34
#define GPCLR0 0x28
#define CLOCK 30

#define IOCTL_MAGIC_NUMBER 'j'
#define CHAT_SET _IOWR(IOCTL_MAGIC_NUMBER, 0, char)
#define CHAT_SEND_CHAR _IOWR(IOCTL_MAGIC_NUMBER, 1, char)
#define CHAT_RECEIVE_CHAR _IOWR(IOCTL_MAGIC_NUMBER, 2, char)
#define CHAT_HAND_SHAKE _IOWR(IOCTL_MAGIC_NUMBER, 3, char)
#define CHAT_EXIT _IOWR(IOCTL_MAGIC_NUMBER, 4, char)
volatile int is_exit = 0;

static void __iomem *gpio_base;
volatile unsigned int *gpsel1;
volatile unsigned int *gpset1;
volatile unsigned int *gpclr1;
volatile unsigned int *gplev1;

int chat_open(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "Chat driver open!!\n");
   
   gpio_base = ioremap(GPIO_BASE_ADDR, 0x60);
   gpsel1=(volatile unsigned int *)(gpio_base+GPFSEL1);
   gpset1=(volatile unsigned int *)(gpio_base+GPSET0);
   gpclr1=(volatile unsigned int *)(gpio_base+GPCLR0);
   gplev1=(volatile unsigned int *)(gpio_base+GPLEV0);
   
   *gpsel1 = 0x00; //Initialization
   *gpsel1 |= (1<<12); //14 pin(output)
   *gpsel1 |= (0<<15); // 15 pin(input)
   return 0;
}

int chat_release(struct inode *inode, struct file *filp){
   printk(KERN_ALERT "chat driver close!!\n");
   iounmap((void*)gpio_base);
   return 0;
}

int getLevel(void)
{
	if(((*gplev1) &( 1 << 15)) == 0) return 0;
	else return 1;
} //high: return1, low: return0

void setLevel(int input)
{
	if(input == 1) *gpset1 = (1<<14);
	else *gpclr1 = (1<<14);
} //input1 : set high, input0 : set low

void send_buf(int *sbuf)
{
	int i;
	for(i =0; i < 8; i++)
	{
		if(sbuf[i] == 0) 
		{
			setLevel(0);
			printk(KERN_ALERT "send[%d]: 0\n", i);
		}
		else 
		{
			setLevel(1);
			printk(KERN_ALERT "send[%d]: 1\n", i);
		}
		msleep(CLOCK);
	}
}

void receive_buf(int* sbuf)
{
	int i;
	for(i =0; i < 8; i++)
	{
		sbuf[i] = getLevel();
		printk(KERN_ALERT "receive[%d]: %d\n",i, sbuf[i]);
		msleep(CLOCK);
	}
}

void AscToStr(char cbuf, int *sbuf) 
{
   int i;
   for(i = 0; i<8; i++)
   {
      sbuf[i] = (cbuf&(0x80>>i))>>(7-i);
   }
}

int StrToAsc(int* str)
{
   int tmp=0;
   int factor = 1;
   int i;
   for(i=7;i>-1;i--)
   {
      tmp = tmp + factor*str[i];
      factor = factor*2;
   }
   return tmp;
}

void sender_request(void)
{
	while(1) 
	{
		if(getLevel() == 1) break;
		setLevel(0);
		setLevel(1);
	}
	while(1) 
	{
		if(getLevel() == 0) break;
		setLevel(0);
		setLevel(1);
	}

	
}

void receive_listen(void)
{
	while(1)
	{
		if(getLevel() == 1) break;
	}
	while(1)
	{
		if(getLevel() == 0) break;
	}
	
	setLevel(1);
	msleep(CLOCK);
	setLevel(0);
}

long chat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{	
	char cbuf = 0x00;
	int ibuf;
	int sbuf[8];
	int i ;
	
	switch(cmd) {
		case CHAT_SET:
			*gpsel1 = 0x00; //Initialization
			*gpsel1 |= (1<<12); //14 pin(output)
			*gpsel1 |= (0<<15); // 15 pin(input)
			is_exit = 0;
			//setLevel(1); //Initialization
			break;
		case CHAT_HAND_SHAKE:
			printk(KERN_ALERT "Hand shake start!\n");
			while(1)
			{
				setLevel(1);
				setLevel(0);
				if(getLevel() == 0) break;
			}
			printk(KERN_ALERT "Hand shake step 0\n");
			while(1)
			{
				setLevel(1);
				setLevel(0);
				if(getLevel() == 1) break;
			}
			printk(KERN_ALERT "Hand shake step 1\n");
			setLevel(1);
			msleep(5);
			setLevel(0);
			msleep(5);
			setLevel(1);
			msleep(5);
			setLevel(0);
			msleep(5);
			setLevel(1);
			printk(KERN_ALERT "Hand shake end 0\n");
			break;
		case CHAT_SEND_CHAR:	
			printk(KERN_ALERT "Send Case occur\n");
			copy_from_user(&cbuf, (const void*)arg, 1); //copy input from aplication.
			msleep(CLOCK);
			printk(KERN_ALERT "Send set level 0\n");
			setLevel(0);
			msleep(CLOCK);
			printk(KERN_ALERT "Send start send\n");
			AscToStr(cbuf, sbuf);
			printk(KERN_ALERT "Send cbuf: %c\n", cbuf);
			send_buf(sbuf);
			printk(KERN_ALERT "Send exit and level 1\n");
			setLevel(1);
			msleep(CLOCK);
			//for(i =0; i<8; i++) printk(KERN_ALERT "Send buffer: %d\n", sbuf[i]);
			//printk(KERN_ALERT "Send cbuf: %c\n", cbuf);
			break;
		case CHAT_RECEIVE_CHAR:
			printk(KERN_ALERT "Recive Case occur\n");
			while(1)
			{
				if(getLevel() == 0) break;
				if(is_exit == 1) break;
				msleep(1);
			}
			printk(KERN_ALERT "Recive Clock Start\n");
			msleep(CLOCK+1);
			printk(KERN_ALERT "Recive Clock End\n");
			receive_buf(sbuf);
			//for(i = 0; i<8; i++) printk(KERN_ALERT "Recive buffer: %d\n", sbuf[i]);
			
			cbuf = StrToAsc(sbuf);
			printk(KERN_ALERT "Recive cbuf: %c\n", cbuf);
			copy_to_user((const void*)arg, &cbuf, 1); //copy result of cbuf
			break;
		 case CHAT_EXIT:
			is_exit = 1;
			break;
		 default:
			return -2; //error: no case...
	}
	return 0;
}

static struct file_operations chat_fops = {
	.owner = THIS_MODULE,
	.open = chat_open,
	.release = chat_release,
	.unlocked_ioctl = chat_ioctl
};
	
int __init chat_init(void){
	if(register_chrdev(CHAT_MAJOR_NUMBER, CHAT_DEV_NAME, &chat_fops) < 0)
		printk(KERN_ALERT "chat driver initialization fail\n");
	else
		printk(KERN_ALERT "chat driver initialization success\n");
	
	return 0;
}

void __exit chat_exit(void){
	unregister_chrdev(CHAT_MAJOR_NUMBER, CHAT_DEV_NAME);
	printk(KERN_ALERT "chat driver exit done\n");
}

module_init(chat_init);
module_exit(chat_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jiwoong");
MODULE_DESCRIPTION("des");

	
