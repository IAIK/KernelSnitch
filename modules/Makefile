# kernel module
obj-m := helper.o
KVERSION := $(shell uname -r)

all: modules remove insert changeperm

init: modules insert changeperm

insert:
	sudo insmod helper.ko

changeperm:
	sudo chmod 666 /proc/timer_list && sudo chmod 666 /dev/helper

remove:
	sudo rmmod helper.ko

modules:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(shell pwd) clean
