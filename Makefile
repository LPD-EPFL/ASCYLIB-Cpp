.PHONY: main all clean

#TEST_FILE := mytest
TEST_FILE := test
SEARCH_HEADER_FILE := search.h
ALGORITHMS := linkedlist_copy linkedlist_coupling linkedlist_harris
ALGORITHMS += linkedlist_harris_opt linkedlist_lazy linkedlist_optik
ALGORITHMS += linkedlist_optik_gl linkedlist_pugh linkedlist_seq
ALGORITHMS += hashtable_harris hashtable_copy hashtable_java hashtable_optik_gl
ALGORITHMS += hashtable_optik_arraymap hashtable_pugh
ALGORITHMS += array_map_optik
AUX_FILES := key_hasher key_max_min
AUX_FILES += ll_array ll_locked ll_marked ll_simple ll_optik conc_hashmap_segment
GC := 1

SRC_DIR := src
BIN_DIR := bin
BUILD_DIR := build
ALGORITHMS_SUBDIR := algorithms
AUX_SUBDIR := aux

CPPFLAGS += -Wall -fno-strict-aliasing
CPPFLAGS += -Iinclude/ -Iexternal/include/
CPPFLAGS += -I$(SRC_DIR)/ -I$(SRC_DIR)/$(ALGORITHMS_SUBDIR)/ -I$(SRC_DIR)/aux
CPPFLAGS += -I$(SRC_DIR)/atomic_ops

LDFLAGS += -Lexternal/lib/
LDLIBS += -lpthread -lrt -lm

$(shell [ -d "$(BUILD_DIR)" ] || mkdir -p $(BUILD_DIR))
$(shell [ -d "$(BIN_DIR)" ] || mkdir -p $(BIN_DIR))

#SRC := $(patsubst %, $(SRC_DIR)/$(ALGORITHMS_SUBDIR)/%.cpp, $(ALGORITHMS))
#OBJ := $(patsubst %, $(BUILD_DIR)/%.o, $(ALGORITHMS))
HEADERS := $(patsubst %, $(SRC_DIR)/$(ALGORITHMS_SUBDIR)/%.h, $(ALGORITHMS))
HEADERS += $(patsubst %, $(SRC_DIR)/$(AUX_SUBDIR)/%.h, $(AUX_FILES))

include comp_flags.mk

all: main

#$(BUILD_DIR)/%.o: $(SRC_DIR)/$(ALGORITHMS_SUBDIR)/%.cc $(SRC_DIR)/$(ALGORITHMS_SUBDIR)/%.h $(SRC_DIR)/$(SEARCH_HEADER_FILE)
	#$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(TEST_FILE).o: $(SRC_DIR)/$(TEST_FILE).cc $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

main: $(BUILD_DIR)/$(TEST_FILE).o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(BUILD_DIR)/$(TEST_FILE).o $(LDLIBS) -o $(BIN_DIR)/$(TEST_FILE)

clean:
	rm -r $(BUILD_DIR) $(BIN_DIR)
