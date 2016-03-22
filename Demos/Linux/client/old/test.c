/*
**  A test program for kernetix kernel module
** 
*/

#include "kernetix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>		
#include <unistd.h>		
#include <sys/ioctl.h>

void usage(char *programName)
{
  printf("-- LKM -Client- -- \n");
  printf("Usage for reading: %s -r <address>\n", programName);
  printf("Usage for writing: %s -w <address> <value>\n", programName);
  printf("Usage for CR3: %s -cr3\n", programName);
  exit(-1);
}

int main(int argc, char **argv)
{
  int fd, ret_val;  
  char device_name[256];  
  //unsigned long *address = NULL;
  
  //int myvar = 1;
  struct _kernel_read *kread = NULL;
  struct _kernel_write *kwrite = NULL;
  
  if (argc < 2) 
  {
	usage(argv[0]);
  }
  else 
  {   
    memset(device_name, 0x00, sizeof(device_name));
    sprintf(device_name, "/dev/%s0", KERNETIX_DEVICE_NAME);
  
    fd = open(device_name, 0);
    if (fd < 0) 
    {
      printf("Can't open device file: %s\n", device_name);
      exit(-1);
    }
 
	if (strcmp(argv[1],"-r") == 0) 
	{
	  if (argc < 3) usage(argv[0]);
	  
	  kread = (struct _kernel_read *) malloc(sizeof(struct _kernel_read));
	  memset(kread, 0x00, sizeof(struct _kernel_read));
	  
	  kread->address = (void *) strtoul(argv[2], NULL, 0);	  	  
	  printf ("We 're sending %p\n", (int *)kread->address);	
	  
	  if (!ioctl(fd, KERNETIX_ABR, kread))
	  {		
		printf("Content of %p --> %p\n", (int *)kread->address, (int *)kread->value);  
	  }
	  else
	  {
		printf("Problem during IOCTL\n");		  
	  }	  
	  free(kread);
	}
	
    else if (strcmp(argv[1],"-w") == 0) 
	{
	  if (argc < 4) usage(argv[0]);
	  //printf("Addres of myvar: %p - Value: %d\n", &myvar, myvar);
	  
	  kwrite = (struct _kernel_write *)malloc(sizeof(kwrite));
	  memset(kwrite, 0x00, sizeof(kwrite));
	  
	  //kwrite->address = &myvar; 
	  kwrite->address = (void *) strtoul(argv[2], NULL, 0);
	  kwrite->value = (void *)strtoul(argv[3], NULL, 0);
	  
	  printf("You 're about to patch %p with %p\n", (int *)kwrite->address, (int *)kwrite->value);   
	 	  
	  if (!ioctl(fd, KERNETIX_ABW, kwrite))
	  {		
		printf("Yay.. we're writing!...\n");
	    printf("The old value was: %p\n", (int *)kwrite->old_value);  
		//printf("Value of myvar: %d\n", myvar);
	  }
	  else
	  {
		printf("Problem during IOCTL\n");		  
	  }	  	  
	 
	  free(kwrite);	  
	}
	
	else if (strcmp(argv[1],"-cr3") == 0) 
	{   
	  if (!ioctl(fd, KERNETIX_CR3, NULL))
	  {		
		printf("Check syslog to view CR3 Content\n");
	  }
	  else
	  {
		printf("Problem during IOCTL\n");		  
	  }	  	  
	 
	}
	
	    
    close(fd);
  }
}