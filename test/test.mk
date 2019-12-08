ODIR_TEST=$(ODIR)
TDIR=test
CFLAGS_TEST = \
	      -g \
	      -I$(IDIR) \
	      -I$(JSMN) \
	      -I3rd-party/Catch2/single_include/ \
	      -fsanitize=address \
	      -fno-omit-frame-pointer \
	      -pedantic \
	      -Werror \

TEST_LIB = \
	   -lgcov --coverage

_OBJ_TEST = \
	   test-workspace.o \
	   mock-symbol.o \
	   test-main.o

OBJ_TEST = $(patsubst %,$(ODIR_TEST)/%,$(_OBJ_TEST))

test: obj test-all gen-cov
test-ncov: obj test-all

test-all: CC_TEST = c++ 
test-all: _OBJ += c-api.o
test-all: CFLAGS += \
    -g \
    -fsanitize=address \
    -fno-omit-frame-pointer \
    -fprofile-arcs \
    -ftest-coverage


test-all: $(OBJ) $(OBJ_TEST)
	$(CC_TEST) -o $@ $^ $(CFLAGS_TEST) $(TEST_LIB)
	./$@

gen-cov:
	lcov --capture --rc lcov_branch_coverage=1 \
	    --directory obj --output-file coverage.info
	genhtml coverage.info --branch-coverage --output coverage

$(ODIR_TEST)/%.o: $(TDIR)/%.cpp $(DEPS)
	$(CC_TEST) -MMD -c -o $@ $< $(CFLAGS_TEST)

-include $(ODIR_TEST)/.*d
