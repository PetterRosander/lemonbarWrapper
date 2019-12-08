CC = gcc

IDIR = include
ODIR = obj
SDIR = src
JSMN = 3rd-party/jsmn/

CFLAGS = \
	 -pedantic \
	 -Werror \
	 -I$(IDIR)/ \
	 -I$(JSMN)

_OBJ = \
      commChannel.o \
      workspace.o 

_MAIN = main.o

DEPS = \
       $(IDIR)/commChannel.h \
       $(IDIR)/workspace.h \

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
MAIN = $(patsubst %,$(ODIR)/%,$(_MAIN))

include test/test.mk

all: obj lemonWrapper

obj:
	mkdir -p $@
	
lemonWrapper: $(OBJ) $(MAIN)
	$(CC) -o $@ $^ $(CFLAGS)

clean: 
	find . -name '*.o' -exec rm -rf {} \;
	find . -name '*.d' -exec rm -rf {} \;
	rm -rf lemonWrapper test-all coverage.info coverage


$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -MMD -c -o $@ $< $(CFLAGS) 

-include $(ODIR)/.*d

.PHONY: lemonWrapper test
