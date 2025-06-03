obj-m += kfetch_mod_411551005.o 
 
PWD := $(CURDIR) 
 
all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 
 
clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	insmod kfetch_mod_411551005.ko

unload:
	rmmod kfetch_mod_411551005
