#include <linux/init.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/kernel.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
	#include <linux/config.h>
#else
	#include <linux/platform_device.h>
#endif
#include <linux/module.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/version.h>

#include <asm/cache.h>
#include <asm/io.h>
#include <asm/processor.h>	/* Processor type for cache alignment. */
#include <asm/uaccess.h>	/* for copy_from_user */

static unsigned long int pStart;
static unsigned long int pEnd;
static unsigned long int pOffset;

struct iomap_address{
	unsigned long int start;
	unsigned long int end;
};

struct write_wrapper{
	unsigned long int target;
	unsigned long int value;
};

struct read_wrapper{
	unsigned long int value;
	int offset;
};

//#define	IOCTL_SET_COMMAND	0x01
//#define	IOCTL_READ_COMMAND	0x02
//#define IOCTL_WRITE_COMMAND	0x03

#define IOC_MAGIC 'i'

#define IOCTL_SET_COMMAND    _IOW(IOC_MAGIC, 0, int)
#define IOCTL_READ_COMMAND   _IOR(IOC_MAGIC, 1, int)
#define IOCTL_WRITE_COMMAND  _IOW(IOC_MAGIC, 2, int)

//#define WRITE_BYTE 0x04
//#define WRITE_4_BYTE 0x08

#define DEVICE_NAME "iomem"	/* Dev name as it appears in /proc/devices   */
static int MajorNum;		/* Major number assigned to our device driver */

/**********************************************************************
 *  Function prototype...
 **********************************************************************/
//#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
//static int iomem_ioctl(struct inode *inode,	/* see include/linux/fs.h */
//		 struct file *file,	/* ditto */
//		 unsigned int ioctl_num,	/* number and param for ioctl */
//		 unsigned long ioctl_param);
//#else
static int iomem_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);
//#endif

static int iomem_open(struct inode *, struct file *);
static int iomem_release(struct inode *, struct file *f);
static ssize_t iomem_read(struct file *, char *, size_t, loff_t *);
static ssize_t iomem_write(struct file *, const char *, size_t, loff_t *);

/**********************************************************************
 *  Function Implementation...
 **********************************************************************/
//#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
//static int iomem_ioctl(struct inode *inode,	/* see include/linux/fs.h */
//		 struct file *file,	/* ditto */
//		 unsigned int ioctl_num,	/* number and param for ioctl */
//		 unsigned long ioctl_param)
//#else
static int iomem_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
//#endif
{
	printk(KERN_ALERT "iomem_ioctl\n");
	/* 
	 * Switch according to the ioctl called 
	 */
	void __user *arg = (void __user *)ioctl_param;

	switch(ioctl_num){
		case IOCTL_SET_COMMAND:
		{
			printk(KERN_ALERT "IOCTL_SET_COMMAND\n");
			struct iomap_address *iomap;

			iomap = kmalloc(sizeof(struct iomap_address), GFP_KERNEL);
                	if (!iomap){
                		printk(KERN_ALERT "No Memory for struct iomap_address *iomap\n");
                       		return -ENOMEM;
                	}

			if (copy_from_user(iomap, arg, sizeof(struct iomap_address)))
               			return -EFAULT;

			pStart = iomap->start;
			pEnd = iomap->end;
			pOffset = pEnd - pStart + 1;

			/*if (!request_mem_region(pStart, pOffset, "iomem")){
				printk(KERN_ALERT "ERROR: Could not request mem region, for iomem function\n");
				return -ENOMEM;
			}

			iobase = ioremap_nocache(pStart, pOffset);
			if (iobase == NULL){
				printk(KERN_ALERT "Unable to map iomem function\n");
				return -ENOMEM;
			}*/
			
			//printk(KERN_ALERT "Start IO Address: 0x%08lX\n",(long unsigned int)(pStart));
			//printk(KERN_ALERT " End  IO Address: 0x%08lX\n",(long unsigned int)(pEnd));
			//printk(KERN_ALERT "    Offset      : 0x%08lX\n",(long unsigned int)(pOffset));
			break;
		}
		case IOCTL_READ_COMMAND:
		{
			printk(KERN_ALERT "IOCTL_READ_COMMAND\n");
			unsigned int i;
			struct read_wrapper *wrap;
			wrap = kmalloc(sizeof(struct read_wrapper), GFP_KERNEL);
			if(!wrap){
				printk(KERN_ALERT "No Memory for struct read_wrapper *wrap\n");
                       		return -ENOMEM;
			}
			if (copy_from_user(wrap, arg, sizeof(struct read_wrapper)))
               			return -EFAULT;

			i = wrap->offset;
			//wrap->value = *((u32 *)(IO_BASE + pStart) + i);
			wrap->value = *((u32 *)(pStart) + i);

			if (copy_to_user(arg, wrap, sizeof(struct read_wrapper)))
                		return -EFAULT;
			break;
			
			/*unsigned long int *readValueArray;
			int offset = (pEnd - pStart + 1) / 4;
			if( ((pEnd - pStart + 1) % 4) != 0 )
				offset++;

			readValueArray = kmalloc(offset * sizeof(unsigned long int), GFP_KERNEL);
			if(!readValueArray){
				printk(KERN_ALERT "No Memory for unsigned long int *readValueArray\n");
                       		return -ENOMEM;
			}
			for( i = 0 ; i < offset ; i++ )
				readValueArray[i] = *((u32 *)(IO_BASE + pStart) + i);
			if (copy_to_user(arg, readValueArray, offset * sizeof(unsigned long int)))
                		return -EFAULT;
			break;*/
		}
		case IOCTL_WRITE_COMMAND:
		{
			//unsigned long int offset;
			struct write_wrapper *wrap;

			wrap = kmalloc(sizeof(struct write_wrapper), GFP_KERNEL);
                	if (!wrap){
                		printk(KERN_ALERT "No Memory for struct write_wrapper *wrap\n");
                       		return -ENOMEM;
                	}
			if (copy_from_user(wrap, arg, sizeof(struct write_wrapper)))
               			return -EFAULT;

			//printk(KERN_ALERT "Target Write Address: 0x%08lX\n",wrap->target);
			//printk(KERN_ALERT "Target Write  Value : 0x%08lX\n",wrap->value);

			//offset = wrap->target % 4;
			//if( offset == 0 ){
				//*(u32 *)(IO_BASE + wrap->target) = (u32)wrap->value;
				*(u32 *)(wrap->target) = (u32)wrap->value;
			/*}
			else{	
				u32 value;

				value = wrap->value << (8 * offset);
				// *(u32 *)(IO_BASE + (wrap->target - offset)) = (u32)value;
				*(u32 *)((wrap->target - offset)) = (u32)value;

				value = wrap->value >> (8 * (4 - offset));
				// *((u32 *)(IO_BASE + (wrap->target - offset)) + 1) = (u32)value;
				*((u32 *)((wrap->target - offset)) + 1) = (u32)value;
			}
			break;*/
	
			/*printk(KERN_ALERT "(u8)((wrap->value & 0xFF000000) >> 24) = 0x%02X\n",(u8)((wrap->value & 0xFF000000) >> 24));	
			printk(KERN_ALERT "(u8)((wrap->value & 0x00FF0000) >> 16) = 0x%02X\n",(u8)((wrap->value & 0x00FF0000) >> 16));	
			printk(KERN_ALERT "(u8)((wrap->value & 0x0000FF00) >>  8) = 0x%02X\n",(u8)((wrap->value & 0x0000FF00) >>  8));	
			printk(KERN_ALERT "(u8)((wrap->value & 0x000000FF) >>  0) = 0x%02X\n",(u8)((wrap->value & 0x000000FF) >>  0));	
			*((u8 *)(IO_BASE + wrap->target) + 0) = (u8)((wrap->value & 0xFF000000) >> 24);
			*((u8 *)(IO_BASE + wrap->target) + 1) = (u8)((wrap->value & 0x00FF0000) >> 16);
			*((u8 *)(IO_BASE + wrap->target) + 2) = (u8)((wrap->value & 0x0000FF00) >>  8);
			*((u8 *)(IO_BASE + wrap->target) + 3) = (u8)((wrap->value & 0x000000FF) >>  0);	
			break;*/
			
			/*if(wrap->flag && WRITE_4_BYTE)
				*(u32 *)(IO_BASE + wrap->target) = (u32)wrap->value;
			else{
				int offset = wrap->target % 4;
				u32 value = wrap->value << (8 * (3 - offset));
				if( offset == 0 )
					*(u32 *)(IO_BASE + wrap->target) = (u32)value;	
				else
					*(u32 *)(IO_BASE + (wrap->target - offset)) = (u32)value;	
			}*/	
			//printk(KERN_ALERT "IO_BASE + wrap->target: 0x%08lX\n",IO_BASE + wrap->target);
			//printk(KERN_ALERT "(u32 *)(IO_BASE + wrap->target): 0x%08lX\n",(u32 *)(IO_BASE + wrap->target));
			//printk(KERN_ALERT "*(u32 *)(IO_BASE + wrap->target): 0x%08lX\n",*(u32 *)(IO_BASE + wrap->target));
			//__raw_writel(wrap->value, iobase + wrap->target);
			//iounmap(iobase);
			//release_mem_region(pStart, pEnd - pStart + 1);
			//*(u32 *)(IO_BASE + wrap->target) = (u32)wrap->value;
		}
	}
	return 0;
}

static int iomem_open(struct inode *inode, struct file *file)
{
	try_module_get(THIS_MODULE);
	return 0;

}

static int iomem_release(struct inode *inode, struct file *file)
{
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t iomem_read(struct file *filp,	/* see include/linux/fs.h   */
			   char *buffer,	/* buffer to fill with data */
			   size_t length,	/* length of the buffer     */
			   loff_t * offset)
{
	printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
	return -EINVAL;
}

static ssize_t
iomem_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
	printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
	return -EINVAL;
}

static struct file_operations fops = {
	.read = iomem_read,
//#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35)
//	.ioctl = iomem_ioctl,
//#else
        .unlocked_ioctl = iomem_ioctl,
//#endif
	.write = iomem_write,
	.open = iomem_open,
	.release = iomem_release
};

static int __init iomem_init(void)
{
	//u32 data;
	//ulong i;
	printk(KERN_ALERT "IOMEM init\n");

	MajorNum = register_chrdev(0, DEVICE_NAME, &fops);

	if (MajorNum < 0) {
	  printk(KERN_ALERT "Registering char device failed with %d\n", MajorNum);
	  return MajorNum;
	}

	printk(KERN_ALERT "I was assigned major number %d. To talk to\n", MajorNum);
	printk(KERN_ALERT "the driver, create a dev file with\n");
	printk(KERN_ALERT "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, MajorNum);
	printk(KERN_ALERT "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_ALERT "the device file.\n");
	printk(KERN_ALERT "Remove the device file and module when done.\n");

	return 0;
	/*if (!request_mem_region(0x00086000, 0x00086040 - 0x00086000 + 1, "iomem")){
		printk(KERN_ALERT "ERROR: Could not request mem region, for iomem function\n");
		return -ENOMEM;
	}

	iobase = ioremap_nocache(0x00086400, 0x00086440 - 0x00086400 + 1);
	if (iobase == NULL){
		printk(KERN_ALERT "Unable to map iomem function\n");
		release_mem_region(pStart, pOffset);
		return -ENOMEM;
	}*/

	/* Test Form */
	/*pOffset = pEnd - pStart + 1;
	for( i = 0 ; i < pOffset ; i++)
	{
		data = *((u32 *)(IO_BASE + pStart) + i);
		printk(KERN_ALERT "IO Address: [0x%08lX]\n",(long unsigned int)(IO_BASE + pStart + i));
		printk(KERN_ALERT "  Value   : [0x%08lX]\n",(long unsigned int)data); 
	}
	printk(KERN_ALERT "IOMEM exit\n");*/

	//printk(KERN_ALERT "IOMEM exit\n");
	//iounmap(iobase);
	//release_mem_region(pStart, pEnd - pStart + 1);
	//return 0;
	//return -1;
}

static void __exit iomem_exit(void)
{
	/* 
	 * Unregister the device 
	 */
	//iounmap(iobase);
	//release_mem_region(pStart, pOffset);
	unregister_chrdev(MajorNum, DEVICE_NAME);
}

/**********************************************************************
 *  OEM Module Code Start...
 **********************************************************************/
MODULE_AUTHOR("KNight Weng (dolamo0415@gmail.com)");
MODULE_DESCRIPTION("Universal SoC Register Dumper driver");
MODULE_LICENSE("Proprietary");

module_init(iomem_init);
module_exit(iomem_exit);

