CC = gcc

IDIR = include
ODIR = obj
SDIR = src
JSMN = 3rd-party/jsmn/

CFLAGS = \
	 -g \
	 -pedantic \
	 -Werror \
	 -I$(IDIR)/ \
	 -I$(JSMN) \
	 -fsanitize=address \
	 -fno-omit-frame-pointer

_OBJ = \
      lemonCommunication.o \
      workspace.o \
      task-runner.o

_MAIN = main.o

DEPS = \
       $(IDIR)/lemonCommunication.h \
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
