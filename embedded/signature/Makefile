CC = gcc
#CC = clang
SRCROOT = .
ECCSRCDIRS := $(shell ls -d $(SRCROOT)/ecc_*)
RSASRCDIRS := $(shell ls -d $(SRCROOT)/rsa_*)

# All examples share the wolfCrypt objects in WOLFROOT, so build serially.
.NOTPARALLEL:

all: ecc rsa

ecc:
	@for d in $(ECCSRCDIRS); do echo $$d ; $(MAKE) -C $$d CC=$(CC) || exit 1; done

rsa:
	@for d in $(RSASRCDIRS); do echo $$d ; $(MAKE) -C $$d CC=$(CC) || exit 1; done

clean: FORCE
	@for d in $(ECCSRCDIRS); do echo $$d ; $(MAKE) -C $$d clean; done
	@for d in $(RSASRCDIRS); do echo $$d ; $(MAKE) -C $$d clean; done

FORCE:
.PHONY: FORCE