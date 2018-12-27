# middleµÄÖ÷Makefile
#
# created by vermin	2011-05-16
#

DIRS = base sys api src

all:
	@for dir in	$(DIRS); do	make -C	$$dir; echo; done

clean:
	@for dir in	$(DIRS); do	make -C	$$dir clean; echo; done

install:
	@for dir in	$(DIRS); do	make -C	$$dir install; echo; done
