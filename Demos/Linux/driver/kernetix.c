/*
** Kernetix Device Driver for fun and profit
** Based on https://github.com/euspectre/kedr/tree/master/sources/examples/sample_target
** and https://github.com/starpos/scull/tree/master/scull
**
** 
** IOCTL play
*/

#include <linux/init.h>
#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/mutex.h>
#include <linux/sched.h> // For accessing current process descriptor and mm_struct
#include <asm/uaccess.h>

#include "kernetix.h"
///////////////////////////////////////////////////////////////////////////

typedef struct _KERNETIX_DEVICE 
{
  unsigned char *data;
  unsigned long buffer_size; 
  unsigned long block_size;  
  struct mutex kernetix_mutex; 
  struct cdev cdev;
} KERNETIX_DEVICE, *PKERNETIX_DEVICE;

int kernetix_major =   KERNETIX_MAJOR;
int kernetix_minor =   0;

static int kernetix_ndevices = 1;

static PKERNETIX_DEVICE kernetix_device = NULL;
static struct class *device_class = NULL;

///////////////////////////////////////////////////////////////////////////

static int kernetix_open(struct inode *inodep, struct file *filep){   
   printk(KERN_INFO "Device has been opened\n");
   return 0;
}

static int kernetix_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "Device successfully closed\n");
   return 0;
}


static int kernetix_ioctl(struct file *file, unsigned int cmd, unsigned long carg )
{
	void __user * p = (void __user *)carg;
	
	struct _kernel_write *kwrite = NULL;
	struct _kernel_read *kread = NULL;	
	unsigned long _cr3;

    switch (cmd)
    {		
        case KERNETIX_ABW:
        {
          printk(KERN_ALERT "The IOCTL for ABW %#08x was called :)\n", KERNETIX_ABW);
		  kwrite = kzalloc(sizeof(struct _kernel_write), GFP_KERNEL);
		  
          if ( copy_from_user(kwrite, p, sizeof(struct _kernel_write)) != 0)
            return -EINVAL;  
		  /*
		  __asm__("pushl  %eax\n\t"				
                "mov    %cr0,%eax;\n\t"
                "and    $0xFFFEFFFF,%eax;\n\t"				
                "mov    %eax,%cr0;\n\t"  					
                "popl   %eax"
			);		
          */
		  printk(KERN_ALERT "Address: %p\n", kwrite->address );
		  printk(KERN_ALERT "Content: %p\n", *(void **)(kwrite->address) );
		  
		  printk(KERN_ALERT "Value of %p --> %p\n", kwrite->address, *(void **)(kwrite->address) );
		  
		  printk(KERN_ALERT "The new value is %p\n", kwrite->value);	
		  
		  put_user(*(void **)(kwrite->address), &(((struct _kernel_write *)p)->old_value));
		  
		  *(void **)(kwrite->address) = kwrite->value;	  
		  
		  kfree(kwrite);		  
          break;
        }
		case KERNETIX_ABR:
		{
		  printk(KERN_ALERT "The IOCTL for ABR %#08x was called :)\n", KERNETIX_ABR);  
		  kread = kzalloc(sizeof(struct _kernel_read), GFP_KERNEL);		  
		  
		  if (copy_from_user(kread, p, sizeof(struct _kernel_read)) != 0)
			return -EINVAL; 

		  printk(KERN_ALERT "Value of 0x%p --> 0x%p\n", kread->address, *(void **)(kread->address) );  
		
		  put_user(*(void **)(kread->address), &(((struct _kernel_read *)p)->value));
		  
		  kfree(kread);
		  break;
		}
        case KERNETIX_CR3:
		{
		  printk(KERN_ALERT "The IOCTL for CR3 %#08x was called :)\n", KERNETIX_CR3);
		  
		  //put_user( ((struct mm_struct *)current->mm)->pgd, (unsigned long **)p );
		  		  
		  printk(KERN_ALERT "Value of PGD Virtual Address --> %p\n", ((struct mm_struct *)current->mm)->pgd);
          
		  __asm__("mov %%cr3, %%rax" : "=a" (_cr3));			       
		  
		  printk(KERN_ALERT "Value of CR3 (Physical Address) --> %p\n", _cr3);
          put_user( _cr3, (unsigned long **)p );
		  
		  break;
		}
    }

    return 0;
}

struct file_operations fops = {
	.owner =    THIS_MODULE,
	//.read =     kernetix_read,
	//.write =    kernetix_write,
	.unlocked_ioctl = kernetix_ioctl,
	.open =     kernetix_open,
	.release =  kernetix_release	
};

///////////////////////////////////////////////////////////////////////////
void driver_cleanup(void)
{
  if (kernetix_device)
  {
	device_destroy(device_class, MKDEV(kernetix_major,kernetix_minor));
	cdev_del(&kernetix_device->cdev);
	kfree(kernetix_device->data);
	mutex_destroy(&kernetix_device->kernetix_mutex);
	kfree(kernetix_device);
  }

  if (device_class)
  {
    class_destroy(device_class);
  }
  unregister_chrdev_region(MKDEV(kernetix_major,kernetix_minor), kernetix_ndevices);
}

static int driver_initialize(void)
{
  int err = 0;  
  struct device *device = NULL;
  dev_t dev_number = 0;
  
  
   
  err = alloc_chrdev_region(&dev_number, kernetix_minor, kernetix_ndevices, KERNETIX_DEVICE_NAME);
  if (err < 0) 
  {
    printk(KERN_WARNING "[target] alloc_chrdev_region() failed\n");
	return err;
  }
  kernetix_major = MAJOR(dev_number);
  
  /* Create device class (before allocation of the device) */
  device_class = class_create(THIS_MODULE, KERNETIX_DEVICE_NAME);
  if (IS_ERR(device_class)) 
  {
    err = PTR_ERR(device_class);
    goto fail;
  }
  
  kernetix_device = (PKERNETIX_DEVICE) kzalloc(sizeof(struct _KERNETIX_DEVICE), GFP_KERNEL);
  if (!kernetix_device)
  {
	err = -ENOMEM;
	goto fail;
  }
  
  kernetix_device->data = NULL;     
  kernetix_device->buffer_size = KERNETIX_BUFFER_SIZE;
  kernetix_device->block_size = KERNETIX_BLOCK_SIZE;
  mutex_init(&kernetix_device->kernetix_mutex);	
  cdev_init(&kernetix_device->cdev, &fops);
  kernetix_device->cdev.owner = THIS_MODULE;
  
  err = cdev_add(&kernetix_device->cdev, dev_number, 1);
  if (err)
  {
	printk(KERN_WARNING "[target] Error %d while trying to add %s%d", err, KERNETIX_DEVICE_NAME, MINOR(dev_number));
	goto fail;
  }

  
  device = device_create(device_class, NULL, dev_number, NULL, KERNETIX_DEVICE_NAME "%d", MINOR(dev_number));

  if (IS_ERR(device)) 
  {
	err = PTR_ERR(device);
	printk(KERN_WARNING "[target] Error %d while trying to create %s%d", err, KERNETIX_DEVICE_NAME, MINOR(dev_number));
	cdev_del(&kernetix_device->cdev);
	goto fail;
  }  
  
  return 0;
  
  fail:
    driver_cleanup();
    return err;
}

static int __init kernetix_init(void)
{
  printk(KERN_ALERT "Creating Device %s\n", KERNETIX_DEVICE_NAME);
  return driver_initialize();  
}

static void __exit kernetix_exit(void)
{
  printk(KERN_ALERT "Unloading %s\n", KERNETIX_DEVICE_NAME);
  driver_cleanup();  
}

module_init(kernetix_init);
module_exit(kernetix_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("n3k");
MODULE_DESCRIPTION("A test module for Arbitrary Write");
