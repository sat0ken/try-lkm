MODULEDIR = /lib/modules/`uname -r`/build
obj-m := test_module.o test_call.o

modules:
	$(MAKE) -C $(MODULEDIR) M=`pwd` modules

clean:
	$(MAKE) -C $(MODULEDIR) M=`pwd` clean

modules_install:
	$(MAKE) -C $(MODULEDIR) M=`pwd` INSTALL_MOD_PATH=/ modules_install
