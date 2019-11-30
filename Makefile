CC ?= gcc

IDIR = include
ODIR = obj
SDIR = src

CFLAGS = \
	 -pedantic \
	 -Werror \
	 -I$(IDIR)/

_OBJ = \
      commChannel.o \
      main.o

_DEPS = commChannel.h


DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


all: obj lemonWrapper

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS) 
	$(CC) $(CFLAGS) -c -o $@ $< 

obj:
	mkdir -p $@
	
lemonWrapper: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean: 
	find . -name '*.o' -exec rm -rf {} \;
	rm -f lemonWrapper
