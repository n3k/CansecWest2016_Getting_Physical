obj-m += kernetix.o

EXTRA_CFLAGS := -I$(src)/../include

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
