SUBDIRS:= `ls | egrep -v '^(CVS)$$'`
COPYDIR:= $(CURDIR)/include/
COPYLIB:= $(CURDIR)/lib/
all:
	@for i in $(SUBDIRS); do if test -e $$i/Makefile ; then $(MAKE) -C $$i || { exit 1;} fi; done;
clean:
	@for i in $(SUBDIRS); do if test -e $$i/Makefile ; then $(MAKE)  -C $$i clean || { exit 1;} fi; done;
install:
	@for i in $(SUBDIRS); do if test -e $$i/Makefile ; then $(MAKE)  -C $$i install || { exit 1;} fi; done;
move:
	@if [ -d $(COPYDIR) ] ; then continue; else mkdir $(COPYDIR)|| { exit 1;} fi;
	@if [ -d $(COPYLIB) ] ; then continue; else mkdir $(COPYLIB)|| { exit 1;} fi;
	@for i in $(SUBDIRS); \
	do if test -d $$i/include/; \
	then  cp $$i/include/* $(COPYDIR)|| { exit 1;} fi; \
	if test -d $$i/lib/; \
	then  cp $$i/lib/* $(COPYLIB)|| { exit 1;} fi; \
	done;	
	
