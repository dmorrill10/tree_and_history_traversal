.PHONY: default
default: all

# Build options
#==============

# Compile options
#----------------

DEBUG_SYMBOLS =-g -O1

PROFILING_SYMBOLS =-p $(DEBUG_SYMBOLS)

TO_OBJ = -c
TO_FILE = -o

# Enable print log
PRINT_LOG = -DDEBUG

# Preprocess out asserts
NO_ASSERTS = -DNDEBUG

OPT =-O3 -ffast-math -ftree-vectorize

WARNINGS =-Wall -Wextra -Wredundant-decls

### C
CC =gcc -std=gnu99
CFLAGS = -fPIC -march=native

### C++
CPPFLAGS := $(CFLAGS)
#CLANG_OPTIONS = -fsanitize=memory -fno-optimize-sibling-calls -fno-omit-frame-pointer -fsanitize-memory-track-origins=1
CLANG_OPTIONS =-fcolor-diagnostics
CPP =clang++ -std=c++11 $(CLANG_OPTIONS)


# Linking options
#----------------
LDLIBS = -lm -lutil


# Structure
#====================

# More precise but typically not necessary and more verbose
THIS_DIR := $(abspath $(CURDIR)/$(dir $(lastword $(MAKEFILE_LIST))))
#THIS_DIR := .

VENDOR_DIR := $(THIS_DIR)/vendor
$(VENDOR_DIR):
	-mkdir $(VENDOR_DIR)

# Vendor definitions
#-------------------

UTILITIES_DIR :=$(VENDOR_DIR)/utilities


VENDOR_INCLUDES :=-I$(VENDOR_DIR)

LIB_OBJ  =$(VENDOR_OBJS)

# Project
#--------
INCLUDES =$(VENDOR_INCLUDES)

### Directories
SRC_DIR := $(THIS_DIR)/src


### Source files
C_SRC = $(shell find $(SRC_DIR) -type f -name '*.c' 2>/dev/null)
C_HEADERS =$(shell find $(SRC_DIR) -type f -name '*.h' 2>/dev/null)
CPP_SRC = $(shell find $(SRC_DIR) -type f -name '*.cpp' 2>/dev/null)
CPP_HEADERS =$(shell find $(SRC_DIR) -type f -name '*.hpp' 2>/dev/null)

### Intermediate files
C_OBJ :=$(C_SRC:%.c=%.c.o)
CPP_OBJ :=$(CPP_SRC:%.cpp=%.cpp.o)

MAIN_OBJ :=$(filter %-main.cpp.o,$(CPP_OBJ))

C_LIB_OBJ :=$(filter-out %-main.c.o,$(C_OBJ))
CPP_LIB_OBJ :=$(filter-out %-main.cpp.o,$(CPP_OBJ))

LIB_OBJ +=$(C_LIB_OBJ) $(CPP_LIB_OBJ)

### Executables
TO_TARGET=$(patsubst %-main.cpp.o,%,$(notdir $1))
TARGETS  =$(call TO_TARGET,$(MAIN_OBJ))


SRC_INCLUDES :=-I$(SRC_DIR)
INCLUDES +=$(SRC_INCLUDES)


# Build rules
#============

# Base rules
#-----------

%.c.o: %.c
	@if [ ! -d $(@D) ]; then mkdir -p $(@D); fi
	@echo [CC] $@
	$(CC) -c $(CFLAGS) $(TO_FILE) $@ $^ $(INCLUDES)

%.cpp.o: %.cpp | $(UTILITIES_DIR)
	@if [ ! -d $(@D) ]; then mkdir -p $(@D); fi
	@echo [CPP] $@
	$(CPP) -c $(CPPFLAGS) $(TO_FILE) $@ $^ $(INCLUDES)

$(TARGETS): $(CPP_LIB_OBJ) $(C_LIB_OBJ) $(MAIN_OBJ)
	@if [ ! -d $(@D) ]; then mkdir -p $(@D); fi
	@echo [LD] $@
	$(CPP) $(CPPFLAGS) $(LDFLAGS) $(TO_FILE) $@ $^ $(LDLIBS)
	@chmod 755 $@


format: $(CPP_SRC) $(CPP_HEADERS)
	@echo [format] $^
	@clang-format $^ -i

# Versions
#---------
.PHONY: release
release: CFLAGS +=$(OPT) $(WARNINGS) $(NO_ASSERTS)
release: CPPFLAGS +=$(OPT) $(WARNINGS) $(NO_ASSERTS)
release: $(TARGETS)

.PHONY: all
all: CFLAGS +=$(OPT) $(WARNINGS)
all: CPPFLAGS +=$(OPT) $(WARNINGS)
all: $(TARGETS)

.PHONY: debug
debug: CFLAGS +=$(DEBUG_SYMBOLS) $(WARNINGS)
debug: CPPFLAGS +=$(DEBUG_SYMBOLS) $(WARNINGS)
debug: $(TARGETS)


# Utilities
#==========

# Clean
#-------
.PHONY: clean
clean:
	-rm $(THIS_DIR)/bin/* $(THIS_DIR)/test/bin/* core.* $(TARGETS) $(CPP_OBJ) $(C_OBJ)

.PHONY: cleandep
cleandep:
	-rm $(DEP)


# Debugging
#----------
print-%:
	@echo $* = $($*)


# Testing
#===========

# Definitions
#------------
CATCH_DIR :=$(VENDOR_DIR)/Catch/single_include
TEST_DIR :=$(abspath $(THIS_DIR)/test)
TEST_SUPPORT_DIR :=$(TEST_DIR)/support
TEST_SRC_EXTENSION =.cpp
TEST_EXTENSION =.out
TEST_PREFIX =test_
TEST_EXECUTABLE_DIR :=$(TEST_DIR)/bin
TEST_SUBDIRS :=$(filter-out $(TEST_SUPPORT_DIR) $(TEST_DIR)/data,$(wildcard $(TEST_DIR)/*))
TESTS        :=$(shell find $(TEST_SUBDIRS) -type f -name '$(TEST_PREFIX)*$(TEST_SRC_EXTENSION)' 2>/dev/null)
TEST_EXES_TEMP   :=$(TESTS:%$(TEST_SRC_EXTENSION)=%$(TEST_EXTENSION))
TEST_EXES :=$(abspath $(TEST_EXES_TEMP:$(TEST_DIR)%=$(TEST_EXECUTABLE_DIR)%))
TEST_SUPPORT_SRC :=$(TEST_SUPPORT_DIR)/test_helper.cpp
TEST_SUPPORT_OBJ :=$(TEST_SUPPORT_DIR)/test_helper.cpp.o


# Rules
#------
$(TEST_EXECUTABLE_DIR):
	@echo [mkdir] $@
	@mkdir -p $@

$(TEST_SUPPORT_DIR):
	@echo [mkdir] $@
	@mkdir -p $@

$(TEST_SUPPORT_SRC): | $(TEST_SUPPORT_DIR)
	@echo [touch] $@
	@touch $@

T = $(abspath $(TEST_EXECUTABLE_DIR))/$(TEST_PREFIX)
$(T)%$(TEST_EXTENSION): $(TEST_DIR)/$(TEST_PREFIX)%.cpp.o $(CPP_LIB_OBJ) $(C_LIB_OBJ) $(TEST_SUPPORT_OBJ) | $(UTILITIES_DIR)
	@if [ ! -d $(@D) ]; then mkdir -p $(@D); fi
	@echo [LD] $@
	$(CPP) $(CPPFLAGS) $(TO_FILE) $@ $^ $(VENDOR_OBJS) $(LDLIBS)
	@chmod 755 $@


.PHONY: test
test: OPT =-O1
test: CFLAGS +=$(OPT) $(WARNINGS)
test: CPPFLAGS +=$(OPT) $(WARNINGS)
test: INCLUDES +=-I$(CATCH_DIR) -I$(TEST_SUPPORT_DIR)
test: $(TEST_EXES)
	@for test in $^; do echo ; echo [TEST] $$test; \
		echo "===============================================================\n";\
		$$test | grep -v -P '^\s*\#'; done

.PHONY: cleantest
cleantest:
	-rm $(TEST_EXECUTABLE_DIR)/* $(TEST_DIR)/*.o
