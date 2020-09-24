CC := gcc -fsanitize=address
program_NAME := run
program_C_SRCS := $(wildcard *.c)
program_CXX_SRCS := $(wildcard *.cpp)
program_C_OBJS := ${program_C_SRCS:.c=.o}
program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}
program_OBJS := $(program_C_OBJS) $(program_CXX_OBJS)
program_INCLUDE_DIRS :=
program_LIBRARY_DIRS :=
program_LIBRARIES :=

#WOLF_INSTALL_DIR := /Users/kalebhimes/work/testDir/wolf-install-dir-for-testing
WOLF_INSTALL_DIR := /usr/local/lib
program_INCLUDE_DIRS += $(WOLF_INSTALL_DIR)/include
program_INCLUDE_DIRS += ./include
program_LIBRARY_DIRS += $(WOLF_INSTALL_DIR)/lib
program_LIBRARIES += wolfssl

CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
#CPPFLAGS += -Werror
#CPPFLAGS += -Weverything
#CPPFLAGS += -Wsign-conversion
#CPPFLAGS += -Wshorten-64-to-32 #NOTE: Not supported on most linux distros.
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))

.PHONY: all clean distclean

all: $(program_NAME)

$(program_NAME): $(program_OBJS)
	$(CC) $(CPPFLAGS) $(program_OBJS) -o $(program_NAME) $(LDFLAGS) 

clean:
	@- $(RM) $(program_NAME)
	@- $(RM) $(program_OBJS)

distclean: clean
