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

#include <readline/readline.h>
#include <readline/history.h>

/*
void error(char *message)
{
	printf("%s\n", message);
	exit(-1);
}
*/

void usage(void)
{
  printf("\n-- LKM -Client- -- \n");
  printf("Usage for reading: r <address> bytes-to-read\n");
  printf("Usage for writing: w <address> <value>\n");
  printf("Usage for CR3: cr3\n");  
  printf("Usage for reading as ring3: ru <address> bytes-to-read\n"); 
}

/*
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
*/

char **parse_arguments(char *command_line)
{
  const char delim[2] = " ";
  char **args = calloc(1, 0x40);  
  char *token;
  char **p = args;
  unsigned int argc = 1;
  
  p++;
  token = strtok(command_line, delim);
  while(token != NULL)
  {
	*p = token;
	token = strtok(NULL, delim);
	p++;
	argc++;    
  }  
  
  *(unsigned int *)&args[0] = argc;
  
  return args;  
}

//////////////////////////////////////////////////////////////////////////////////////////////

void print_memory ( unsigned long address , char *buffer , unsigned int bytes_to_print )
{
  unsigned int cont;
  unsigned int i;
  const unsigned short bytes = 16;

/* Print the lines */
  for ( cont = 0 ; cont < bytes_to_print ; cont = cont + bytes )
  {
    printf ( "%p | " , (void *)address );  
    address = address + bytes;    

    for ( i = 0 ; i < bytes ; i ++ )
    {    
      if ( i < ( bytes_to_print - cont ) )
      {
        printf ( "%.2x " , ( unsigned char ) buffer [ i + cont ] );
      }
      else
      {
        printf ( "   " );
      }
    }

    //Space between two columns
    printf ( "| " );

    //Print the characters
    for ( i = 0 ; i < bytes ; i ++ )
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
    printf ( "\n" );
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////

void execute_command(unsigned int argc, char **argv)
{
  int fd, ret_val;  
  char device_name[256];  
  unsigned long address;
  unsigned int cont;
  unsigned int size;
  char *buffer;
  
  //int myvar = 1;
  struct _kernel_read *kread = NULL;
  struct _kernel_write *kwrite = NULL;
  unsigned long cr3 = 0;
 
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
	  
	address = strtoul(argv[2], NULL, 16);	  	  
	//printf ("We 're sending %p\n", (int *)kread->address);	

    // Size to be read
	size = (unsigned int) strtoul(argv[3], NULL, 16);	  	  
    buffer = malloc ( size );

    for ( cont = 0 ; cont < size ; cont += sizeof ( unsigned long ) )
    { 
      // Reading next address
      kread->address = ( void * ) ( address + cont );

      if (!ioctl(fd, KERNETIX_ABR, kread))
      {		
        //printf("Content of %p --> %p\n", (int *)kread->address, (int *)kread->value);  
        * ( unsigned long * ) &buffer [ cont ] = ( unsigned long ) kread->value;
      }
      else
      {
        printf("Problem during IOCTL\n");		  
      }	  
    }

    // Printing memory
    print_memory ( ( unsigned long ) address , buffer , size );

    // Freeing memory
    free (buffer);
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
    if (!ioctl(fd, KERNETIX_CR3, &cr3))
	{		
	  printf("CR3 Value --> %p\n", (void *)cr3);
	}
	else
	{
	  printf("Problem during IOCTL\n");		  
	}	  	   
  }
  
  else if (strcmp(argv[1],"ru") == 0) 
  {   
    address = strtoul(argv[2], NULL, 16);  	  
    // Size to be read
	size = (unsigned int) strtoul(argv[3], NULL, 16);	  	  
    buffer = malloc ( size );
	
	for ( cont = 0 ; cont < size ; cont += sizeof ( unsigned long ) )
    {          
      *(unsigned long *) &buffer[cont] = (unsigned long) *(void **)(address + cont);     
    }

    // Printing memory
    print_memory ( ( unsigned long ) address , buffer , size );

    // Freeing memory
    free ( buffer );
	
  }  
	    
  close(fd);
}

int main(int argc, char **argv)
{
  char *command_line;
  char **args;
  int i;

  rl_bind_key('\t',rl_abort);//disable auto-complete
  
  usage();
  do {
 
	//command_line = read_command();
    command_line = readline("\n >> ");   
    add_history(command_line);
	
    args = parse_arguments(command_line);

    execute_command((unsigned int)args[0], args);
    free(command_line);
    free(args);
  } while (1);
  
  return 0;
}