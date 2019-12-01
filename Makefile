CC ?= gcc

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
      i3query.o \
      main.o

DEPS = \
       $(IDIR)/commChannel.h \
       $(IDIR)/i3query.h \

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


all: obj lemonWrapper

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -MMD -c -o $@ $< $(CFLAGS) 

-include $(ODIR)/.*d

obj:
	mkdir -p $@
	
lemonWrapper: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean: 
	find . -name '*.o' -exec rm -rf {} \;
	rm -f lemonWrapper
