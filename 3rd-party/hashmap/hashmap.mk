CC ?= gcc

HASHMAP-FLAGS = \
    -g \
    -c \
    -fPIC

$(HASHMAP)hashmap.a:
	$(CC) $(HASHMAP-FLAGS) $(HASHMAP)hashmap.c -o $@
