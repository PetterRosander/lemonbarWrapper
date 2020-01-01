CC = gcc

IDIR = include
ODIR = obj
SDIR = src
JSMN = 3rd-party/jsmn/
HASHMAP = 3rd-party/hashmap/

CFLAGS = \
	 -g \
	 -pedantic \
	 -Werror \
	 -I$(IDIR)/ \
	 -I$(JSMN) \
	 -I$(HASHMAP) \
	 -fsanitize=address \
	 -fno-omit-frame-pointer 
LIBS = \

_OBJ = \
      lemonCommunication.o \
      workspace.o \
      task-runner.o \
      configuration-manager.o \
      plugins.o \
      jsmn.o \
      sys-utils.o

_MAIN = main.o

DEPS = \
       $(IDIR)/lemonCommunication.h \
       $(IDIR)/workspace.h \
       $(IDIR)/configuration-manager.h \
       $(IDIR)/plugins.h

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
MAIN = $(patsubst %,$(ODIR)/%,$(_MAIN))


include test/test.mk
include $(HASHMAP)/hashmap.mk

all: obj lemonWrapper

obj:
	mkdir -p $@
	
lemonWrapper: $(OBJ) $(MAIN) $(HASHMAP)hashmap.a
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean: 
	find . -name '*.o' -exec rm -rf {} \;
	find . -name '*.d' -exec rm -rf {} \;
	rm -rf lemonWrapper test-all coverage.info coverage


$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -MMD -c -o $@ $< $(CFLAGS)

-include $(ODIR)/.*d

.PHONY: lemonWrapper test
