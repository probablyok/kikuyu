# Directories
SRC_DIR  := src
INC_DIR  := include
OBJ_DIR  := obj
TEST_DIR := tests

# Compiler & linker flags
CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -O2 -I$(INC_DIR) -MMD -MP
DEBUG   := -g
LDLIBS  := -lraylib -lm

# Target executable and src/obj mappings
TARGET      := kikuyu
TEST_TARGET := run_tests

# Sources
SRCS     := $(wildcard $(SRC_DIR)/*.c)
LIB_SRCS := $(filter-out $(SRC_DIR)/main.c, $(SRCS))

# Objects
LIB_OBJS := $(LIB_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
MAIN_OBJ := $(OBJ_DIR)/main.o

# Test sources + objects
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS := $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/test_%.o)

TEST_JSON_REPO_URL := https://github.com/SingleStepTests/65x02.git
TEST_JSON_DIR      := tests/test_json
TEST_JSON_REPO_DIR := 6502/v1

.DEFAULT_GOAL := all
.PHONY: all debug test clean clean-tests fetch-tests

# Build main application
all: $(TARGET)

# Get JSON tests from remote repo
fetch-tests:
	@if [ ! -d "$(TEST_JSON_DIR)" ]; then \
		echo "Fetching JSON test cases..."; \
		git clone --depth 1 --filter=blob:none --sparse $(TEST_JSON_REPO_URL) $(TEST_JSON_DIR); \
		git -C $(TEST_JSON_DIR) sparse-checkout set $(TEST_JSON_REPO_DIR); \
	else \
		echo "JSON test cases present. Update with 'git -C $(TEST_JSON_DIR) pull'."; \
	fi

clean-tests:
	rm -rf $(TEST_JSON_DIR)

# Build and run the test suite
test: fetch-tests $(TEST_TARGET)

$(TARGET): $(LIB_OBJS) $(MAIN_OBJ)
	$(CC) -o $@ $^ $(LDLIBS)

$(TEST_TARGET): $(LIB_OBJS) $(TEST_OBJS)
	$(CC) -o $@ $^ $(LDLIBS)

# Compile standard C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test C sources
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create object directory
$(OBJ_DIR):
	mkdir -p $@

# Build variants
debug: CFLAGS += $(DEBUG)
debug: clean $(TARGET)

# Clean artifacts
clean:
	rm -rf $(TARGET) $(TEST_TARGET) $(OBJ_DIR)

# Dependencies include
-include $(LIB_OBJS:.o=.d) $(MAIN_OBJ:.o=.d) $(TEST_OBJS:.o=.d)
