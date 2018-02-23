# Target programs
programs := \
	sem_count.x \
	sem_buffer.x \
	sem_prime.x \
	tps.x \
	tester.x

# User-level thread library
UTHREADLIB := libuthread
UTHREADPATH := ../$(UTHREADLIB)
libuthread := $(UTHREADPATH)/$(UTHREADLIB).a

# Default rule
all: $(libuthread) $(programs)

# Avoid builtin rules and variables
MAKEFLAGS += -rR

# Don't print the commands unless explicitely requested with `make V=1`
ifneq ($(V),1)
Q = @
V = 0
endif

# Current directory
CUR_PWD := $(shell pwd)

# Define compilation toolchain
CC	= gcc

# General gcc options
CFLAGS	:= -Wall -Werror
CFLAGS	+= -pipe
CFLAGS	+= -pthread
## Debug flag
ifneq ($(D),1)
CFLAGS	+= -O2
else
CFLAGS	+= -O0
CFLAGS	+= -g
endif

# Linker options
LDFLAGS := -L$(UTHREADPATH) -luthread

# Include path
INCLUDE := -I$(UTHREADPATH)

# Generate dependencies
DEPFLAGS = -MMD -MF $(@:.o=.d)

# Application objects to compile
objs := $(patsubst %.x,%.o,$(programs))

# Include dependencies
deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)

# Rule for libuthread.a
$(libuthread):
	@echo "MAKE	$@"
	$(Q)$(MAKE) V=$(V) D=$(D) -C $(UTHREADPATH)

# Generic rule for linking final applications
%.x: %.o $(libuthread)
	@echo "LD	$@"
	$(Q)$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

tester.x: tester.o $(libuthread)
	@echo "LD	$@"
	$(Q)$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -Wl,--wrap=mmap

tps%.x: tps%.o $(libuthread)
	@echo "LD	$@"
	$(Q)$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -Wl,--wrap=mmap -Wl,--wrap=munmap

# Generic rule for compiling objects
%.o: %.c
	@echo "CC	$@"
	$(Q)$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $< $(DEPFLAGS)

# Generic rule for markdown
%.html: %.md
	@echo "MKDN	$@"
	$(Q)pandoc -s --toc -o $@ $<

# Cleaning rule
clean:
	@echo "CLEAN	$(CUR_PWD)"
	$(Q)$(MAKE) V=$(V) D=$(D) -C $(UTHREADPATH) clean
	$(Q)rm -rf $(objs) $(deps) $(programs)

# Keep object files around
.PRECIOUS: %.o
.PHONY: clean $(libuthread)



