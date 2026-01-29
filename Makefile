# Top level makefile to build all projects
all:
	cd inet4.5 && make makefiles && $(MAKE)
	cd Simu5G && make makefiles && $(MAKE)
	cd tsnfivegcomm && make makefiles && $(MAKE)
