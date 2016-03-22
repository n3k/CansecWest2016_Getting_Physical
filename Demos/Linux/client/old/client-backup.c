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

void error(char *message)
{
	printf("%s\n", message);
	exit(-1);
}

void usage(void)
{
  printf("-- LKM -Client- -- \n");
  printf("Usage for reading: r <address> bytes-to-read\n");
  printf("Usage for writing: w <address> <value>\n");
  printf("Usage for CR3: cr3\n");  
}

char *read_command(void)
{
  char c;
  int i = 0;
  char *buffer = calloc(1, 256);
  if (!buffer)
	error("read_command failed allocation");
  
  while(i < 255)
  {
	c = getchar();
    if (c == EOF || c == '\n') 
	{      
      return buffer;	
	}
	buffer[i] = c;
	i++;
  }	
  
  return buffer;
}

char **parse_arguments(char *command_line)
{
  const char delim[2] = " ";
  char **args = calloc(1, 32);  
  char *token;
  char **p = args;
  int argc = 1;
  
  p++;
  token = strtok(command_line, delim);
  while(token != NULL)
  {
	*p = token;
	token = strtok(NULL, delim);
	p++;
	argc++;    
  }  
  
  args[0] = (char *)argc;
  
  return args;  
}

//////////////////////////////////////////////////////////////////////////////////////////////

#define BYTES 16

void print_memory ( unsigned int address , char *buffer , unsigned int bytes_to_print )
{
  unsigned int cont;
  unsigned int i;

/* Print the lines */
  for ( cont = 0 ; cont < bytes_to_print ; cont = cont + BYTES )
  {
  /* Print memory address */
    printf ( "%.8x | " , address );

  /* increment address */
    address = address + BYTES;
    
  /* print in hex */
    for ( i = 0 ; i < BYTES ; i ++ )
    {
    /* print as much as bytes_to_print */  
      if ( i < ( bytes_to_print - cont ) )
      {
        printf ( "%.2x " , ( unsigned char ) buffer [ i + cont ] );
      }
      else
      {
        printf ( "   " );
      }
    }

  /* Space between two columns */
    printf ( "| " );

  /* Print the characters */  
    for ( i = 0 ; i < BYTES ; i ++ )
    {
      if ( i < ( bytes_to_print - cont ) )
      {
        printf ( "%c" , ( isgraph ( buffer [ i + cont ] ) ) ? buffer [ i + cont ] : '.' );
      }
      else
      {
        printf ( " " );
      }
    }

  /* endline */
    printf ( "\n" );
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////

void execute_command(int argc, char **argv)
{
  int fd, ret_val;  
  char device_name[256];  
  unsigned int address;
  unsigned int cont;
  unsigned int size;
  char *buffer;
  
  //int myvar = 1;
  struct _kernel_read *kread = NULL;
  struct _kernel_write *kwrite = NULL;  
 
  memset(device_name, 0x00, sizeof(device_name));
  sprintf(device_name, "/dev/%s0", KERNETIX_DEVICE_NAME);
  
  fd = open(device_name, 0);
  if (fd < 0) 
  {
    printf("Can't open device file: %s\n", device_name);
    exit(-1);
  }
 
  if (strcmp(argv[1],"r") == 0) 
  {
	if (argc < 4)
	{
	  usage();    
      return;	  
	}
	  
	kread = (struct _kernel_read *) malloc(sizeof(struct _kernel_read));
	memset(kread, 0x00, sizeof(struct _kernel_read));
	  
	address = (unsigned int) strtoul(argv[2], NULL, 16);	  	  
	//printf ("We 're sending %p\n", (int *)kread->address);	

    // Size to be read
	size = (unsigned int) strtoul(argv[3], NULL, 0);	  	  
    buffer = malloc ( size );

    for ( cont = 0 ; cont < size ; cont += sizeof ( unsigned int ) )
    { 
      // Reading next DWORD
      kread->address = ( void * ) ( address + cont );

      if (!ioctl(fd, KERNETIX_ABR, kread))
      {		
        //printf("Content of %p --> %p\n", (int *)kread->address, (int *)kread->value);  
        * ( unsigned int * ) &buffer [ cont ] = ( unsigned int ) kread->value;
      }
      else
      {
        printf("Problem during IOCTL\n");		  
      }	  
    }

    // Printing memory
    print_memory ( ( unsigned int ) address , buffer , size );

    // Freeing memory
    free ( buffer );
	free(kread);
  }
	
  else if (strcmp(argv[1],"w") == 0) 
  {
	if (argc < 4)
	{
	  usage();    
      return;	  
	}
	//printf("Addres of myvar: %p - Value: %d\n", &myvar, myvar);
	  
	kwrite = (struct _kernel_write *)malloc(sizeof(kwrite));
	memset(kwrite, 0x00, sizeof(kwrite));
	  
	//kwrite->address = &myvar; 
	kwrite->address = (void *) strtoul(argv[2], NULL, 16);
	kwrite->value = (void *)strtoul(argv[3], NULL, 16);
	  
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
	
  else if (strcmp(argv[1],"cr3") == 0) 
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

int main(int argc, char **argv)
{
  char *command_line;
  char **args;
  int i;

  usage();
  do {
    printf(">> ");
    command_line = read_command();
    args = parse_arguments(command_line);

	execute_command((int)args[0], args);

    free(command_line);
    free(args);
  } while (1);
  
  return 0;
}