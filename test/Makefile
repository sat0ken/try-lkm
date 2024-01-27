MODULEDIR = /lib/modules/`uname -r`/build
obj-m := test_module.o

modules:
	$(MAKE) -C $(MODULEDIR) M=`pwd` modules

clean:
	$(MAKE) -C $(MODULEDIR) M=`pwd` clean
