.PHONY: main all clean

#TEST_FILE := mytest
SEARCH_TEST_FILE := test_search
SEARCH_HEADER_FILE := search.h
STACKQUEUE_TEST_FILE := test_stackqueue
STACKQUEUE_HEADER_FILE := stack_queue.h
SEARCH_ALGORITHMS := linkedlist_copy linkedlist_coupling linkedlist_harris
SEARCH_ALGORITHMS += linkedlist_harris_opt linkedlist_lazy linkedlist_optik
SEARCH_ALGORITHMS += linkedlist_optik_gl linkedlist_pugh linkedlist_seq
SEARCH_ALGORITHMS += hashtable_harris hashtable_copy hashtable_java hashtable_optik
SEARCH_ALGORITHMS += hashtable_optik_gl hashtable_optik_arraymap hashtable_pugh
SEARCH_ALGORITHMS += array_map_optik
SEARCH_ALGORITHMS += skiplist_fraser skiplist_herlihy_lb skiplist_herlihy_lf
SEARCH_ALGORITHMS += skiplist_optik skiplist_seq
STACKQUEUE_ALGORITHMS := queue_ms_lb queue_ms_lf queue_ms_hybrid
STACKQUEUE_ALGORITHMS += stack_lock stack_treiber
SEARCH_AUX_FILES := key_hasher key_max_min
SEARCH_AUX_FILES += ll_array ll_locked ll_marked ll_simple ll_optik
SEARCH_AUX_FILES += conc_hashmap_segment
SEARCH_AUX_FILES += sl_simple sl_marked sl_locked
STACKQUEUE_AUX_FILES += stackqueue_node stackqueue_node_padded
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

SEARCH_HEADERS := $(patsubst %, $(SRC_DIR)/$(ALGORITHMS_SUBDIR)/%.h, $(SEARCH_ALGORITHMS))
SEARCH_HEADERS += $(patsubst %, $(SRC_DIR)/$(AUX_SUBDIR)/%.h, $(SEARCH_AUX_FILES))
STACKQUEUE_HEADERS := $(patsubst %, $(SRC_DIR)/$(ALGORITHMS_SUBDIR)/%.h, $(STACKQUEUE_ALGORITHMS))
STACKQUEUE_HEADERS += $(patsubst %, $(SRC_DIR)/$(AUX_SUBDIR)/%.h, $(STACKQUEUE_AUX_FILES))

include comp_flags.mk

all: main

$(BUILD_DIR)/$(SEARCH_TEST_FILE).o: $(SRC_DIR)/$(SEARCH_TEST_FILE).cc $(SEARCH_HEADERS) $(SRC_DIR)/$(SEARCH_HEADER_FILE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

search: $(BUILD_DIR)/$(SEARCH_TEST_FILE).o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(BUILD_DIR)/$(SEARCH_TEST_FILE).o $(LDLIBS) -o $(BIN_DIR)/$(SEARCH_TEST_FILE)

$(BUILD_DIR)/$(STACKQUEUE_TEST_FILE).o: $(SRC_DIR)/$(STACKQUEUE_TEST_FILE).cc $(STACKQUEUE_HEADERS) $(SRC_DIR)/$(STACKQUEUE_HEADER_FILE)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

stackqueue: $(BUILD_DIR)/$(STACKQUEUE_TEST_FILE).o
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(BUILD_DIR)/$(STACKQUEUE_TEST_FILE).o $(LDLIBS) -o $(BIN_DIR)/$(STACKQUEUE_TEST_FILE)

main: search stackqueue

clean:
	rm -r $(BUILD_DIR) $(BIN_DIR)
