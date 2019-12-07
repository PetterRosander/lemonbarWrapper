ODIR_TEST=$(ODIR)
TDIR=test
CFLAGS_TEST += \
	       $(CFLAGS) \
	       -I3rd-party/Catch2/single_include/ \
	       -Wl,wrap,write \
	       -Wl,wrap,read

	       

_OBJ_TEST = \
	   test-workspace.o

OBJ_TEST = $(patsubst %,$(ODIR_TEST)/%,$(_OBJ_TEST))

test: obj test-all

test-all: CC_TEST = c++ 
test-all: CFLAGS = \
    -I$(IDIR) \
    -I$(JSMN)

test-all: $(OBJ) $(OBJ_TEST)
	$(CC_TEST) -o $@ $^ $(CFLAGS)
	./$@

$(ODIR_TEST)/%.o: $(TDIR)/%.cpp $(DEPS)
	$(CC_TEST) -MMD -c -o $@ $< $(CFLAGS_TEST) 

-include $(ODIR_TEST)/.*d
