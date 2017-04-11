EXECUTABLE1 := sign_and_verify
C_SOURCES1 := sign_and_verify.c
C_OBJECTS1 := ${C_SOURCES1:.c=.o}
OBJECTS1 := $(C_OBJECTS1)

EXECUTABLE2 := gen_key_files
C_SOURCES2 := gen_key_files.c
C_OBJECTS2 := ${C_SOURCES2:.c=.o}
OBJECTS2 := $(C_OBJECTS2)

INCLUDE_DIRS :=
LIBRARY_DIRS :=
LIBRARIES :=

#example: if using custom install location add the include and lib dir to the
#         INCLUDE_DIRS and LIBRARY_DIRS respectively

#INCLUDE_DIRS += /Users/khimes/work/testDir/wolf-install-dir-for-testing/include
#LIBRARY_DIRS += /Users/khimes/work/testDir/wolf-install-dir-for-testing/lib
LIBRARIES += wolfssl

CPPFLAGS += $(foreach includedir,$(INCLUDE_DIRS),-I$(includedir))
#CPPFLAGS += -Werror
#CPPFLAGS += -Weverything
LDFLAGS += $(foreach librarydir,$(LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(LIBRARIES),-l$(library))

.PHONY: all clean distclean

all: $(EXECUTABLE1) $(EXECUTABLE2)

$(EXECUTABLE1): $(OBJECTS1)
	$(LINK.cc) $(OBJECTS1) -o $(EXECUTABLE1)

$(EXECUTABLE2): $(OBJECTS2)
	$(LINK.cc) $(OBJECTS2) -o $(EXECUTABLE2)


$(EXECUTABLE1): $(OBJECTS1)
$(EXECUTABLE2): $(OBJECTS2)
clean:
	@- $(RM) $(EXECUTABLE1) $(EXECUTABLE2)
	@- $(RM) $(OBJECTS1) $(OBJECTS2)

distclean: clean
