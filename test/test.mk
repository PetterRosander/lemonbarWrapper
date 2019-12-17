CC_TEST = c++ 
ODIR_TEST=$(ODIR)
ODIR_UTILS=$(ODIR)
TDIR=test
UDIR=test/utils

COMMON_TEST_FLAGS = \
		    -g \
		    -pedantic \
		    -Werror \
		    -DUNIT_TEST \
		    -fno-omit-frame-pointer \
		    -fsanitize=address 

CFLAGS_TEST = \
	      -I$(IDIR) \
	      -I$(JSMN) \
	      -I$(UDIR) \
	      $(COMMON_TEST_FLAGS) \
	      -I3rd-party/Catch2/single_include/

TEST_LIB = \
	   -lgcov --coverage


DEPS_UTILS = \
	     $(UTILS)/mock-approvals.hpp \
	     $(UTILS)/mock-symbol.hpp

DEPS_TEST = \
	    $(TEST)/workspace-data.h


_OBJ_TEST = \
	   test-workspace.o \
	   test-lemonCommunication.o

_OBJ_UTILS = \
	   test-main.o \
	   mock-symbol.o \
	   mock-approvals.o


OBJ_TEST = $(patsubst %,$(ODIR_TEST)/%,$(_OBJ_TEST))
OBJ_UTILS = $(patsubst %,$(ODIR_UTILS)/%,$(_OBJ_UTILS))

test-ncov: obj test-all gen-cov
test: obj test-all

test-all: DEPS += \
    $(DEPS_TEST) \
    $(DEPS_UTILS)

test-all: CFLAGS += \
    $(COMMON_TEST_FLAGS) \


test-all: $(OBJ) $(OBJ_TEST) $(OBJ_UTILS)
	$(CC_TEST) -o $@ $^ $(CFLAGS_TEST) $(TEST_LIB)
	./$@

gen-cov:
	lcov --capture --rc lcov_branch_coverage=1 \
	    --directory obj --output-file coverage.info
	genhtml coverage.info --branch-coverage --output coverage

$(ODIR_TEST)/%.o: $(TDIR)/%.cpp $(DEPS)
	$(CC_TEST) -MMD -c -o $@ $< $(CFLAGS_TEST)

$(ODIR_UTILS)/%.o: $(UDIR)/%.cpp $(DEPS)
	$(CC_TEST) -MMD -c -o $@ $< $(CFLAGS_TEST)
