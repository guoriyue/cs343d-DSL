
SRC_DIR := src
BIN_DIR := bin
INC_DIR := include
TEST_DIR := tests

CXXFLAGS := -g -O3 -std=c++17 -I ./$(INC_DIR)/

# gather any object files we need from the VM
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(SRC_FILES))

.PHONY: all clean test


all: $(BIN_DIR)/compiler

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# this will build all objects needed from source code in src/
$(OBJ_FILES): $(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp $(INC_DIR)/%.h | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -c -o $@

$(BIN_DIR)/compiler: compiler.cpp $(INC_DIR)/* $(OBJ_FILES)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $< $(filter %.o,$^) -o $@

clean:
	rm -rf $(BIN_DIR)

$(BIN_DIR)/test%: $(TEST_DIR)/test%.cpp $(INC_DIR)/* $(OBJ_FILES) FORCE
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $< $(filter %.o,$^) -o $@
	./$@

# empty, forces tests to always rebuild
FORCE:
