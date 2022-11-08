obj-m += psvis.o

default:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	
install:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) module_install

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
