CC_PREFIX   = 
CC          = $(CC_PREFIX)gcc
STRIP       = $(CC_PREFIX)strip
LINKER      = $(CC)
SRCDIR      = ./src
INCDIR      = ./inc
TESTDIR     = ./test

SRC_FILES   = $(shell find $(SRCDIR) -name *.c)
LIB_OBJS    = $(patsubst %.c, %.o, $(SRC_FILES))

TEST_FILES  = $(shell find $(TESTDIR) -name *.c)
APP_OBJS    = $(patsubst %.c, %.o, $(TEST_FILES))

CFLAGS      = -Wall -I$(INCDIR) -O2 -fvisibility=hidden
TARGET      = mmp4m

.PHONY: shared test

all: shared test
shared: $(LIB_OBJS)
	$(LINKER) -o lib$(TARGET).so -shared -O3 $(LIB_OBJS)

test: $(APP_OBJS) $(LIB_OBJS)
	$(CC) -o $(TARGET) $(APP_OBJS) $(LIB_OBJS)

%.o : %.c
	$(CC) $(CFLAGS) $(CPP_FLAG) -c $< -o $@

clean:
	rm -f $(LIB_OBJS) $(APP_OBJS) *.a *.so *.o *~
	rm -f lib$(TARGET).so $(TARGET)

pack:
	$(MAKE) clean
	rm -rf $(TARGET).tar.bz2
	tar -cjvf $(TARGET).tar.bz2 Makefile inc src test
