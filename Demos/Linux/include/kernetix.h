/*
**
** Definitions for kernetix Driver
**
*/

#include <linux/ioctl.h>

#define KERNETIX_BLOCK_SIZE 0x200
#define KERNETIX_BUFFER_SIZE 0x1000
#define KERNETIX_DEVICE_NAME "KernetixDriver"

#define KERNETIX_MAJOR 0

extern int kernetix_major;

struct _kernel_read 
{
  void *address;
  void *value;
};

struct _kernel_write 
{
  void *address;
  void *value;
  void *old_value;  
};


/* Use 'n' as magic number */
#define KERNETIX_IOC_MAGIC  'n'

#define KERNETIX_ABW _IOR(KERNETIX_IOC_MAGIC,  0, struct _kernel_write)
#define KERNETIX_ABR _IOR(KERNETIX_IOC_MAGIC,  1, struct _kernel_read)
#define KERNETIX_CR3 _IOR(KERNETIX_IOC_MAGIC,  2, unsigned int)

