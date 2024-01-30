MODULEDIR = /lib/modules/`uname -r`/build
obj-m := sw.o

modules:
	$(MAKE) -j4 -C $(MODULEDIR) M=`pwd` modules

clean:
	$(MAKE) -C $(MODULEDIR) M=`pwd` clean

modules_install:
	$(MAKE) -C $(MODULEDIR) M=`pwd` INSTALL_MOD_PATH=/ modules_install
