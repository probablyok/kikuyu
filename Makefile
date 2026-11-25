# Directories
SRC_DIR  := src
INC_DIR  := include
OBJ_DIR  := obj
TEST_DIR := tests

# Compiler & linker flags
CC         := gcc
CFLAGS     := -std=c99 -Wall -Wextra -Wpedantic -O2 -I$(INC_DIR) -MMD -MP
DEBUG      := -g
CLI_LDLIBS := -lm
GUI_LDLIBS := -lraylib -lm

# Target executables
CLI_TARGET  := kikuyu_cli
GUI_TARGET  := kikuyu_gui
TEST_TARGET := run_tests

# Sources
ALL_SRCS      := $(wildcard $(SRC_DIR)/*.c)
CLI_ONLY_SRCS := $(SRC_DIR)/main_cli.c $(SRC_DIR)/cli_mode.c
GUI_ONLY_SRCS := $(SRC_DIR)/main_gui.c $(SRC_DIR)/gui_mode.c $(SRC_DIR)/layout_helper.c
CORE_SRCS     := $(filter-out $(CLI_ONLY_SRCS) $(GUI_ONLY_SRCS), $(ALL_SRCS))

CLI_SRCS := $(CORE_SRCS) $(CLI_ONLY_SRCS)
GUI_SRCS := $(CORE_SRCS) $(GUI_ONLY_SRCS)

# Objects
CORE_OBJS := $(CORE_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CLI_OBJS  := $(CLI_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
GUI_OBJS  := $(GUI_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Test sources + objects
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS := $(TEST_SRCS:$(TEST_DIR)/%.c=$(OBJ_DIR)/test_%.o)

TEST_JSON_REPO_URL := https://github.com/SingleStepTests/65x02.git
TEST_JSON_DIR      := tests/test_json
TEST_JSON_REPO_DIR := 6502/v1

.DEFAULT_GOAL := all
.PHONY: all cli gui debug test clean clean-tests fetch-tests

# Build CLI and or GUI applications
all: $(CLI_TARGET) $(GUI_TARGET)
cli: $(CLI_TARGET)
gui: $(GUI_TARGET)

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

# Build targets
$(CLI_TARGET): $(CLI_OBJS)
	$(CC) -o $@ $^ $(CLI_LDLIBS)

$(GUI_TARGET): $(GUI_OBJS)
	$(CC) -o $@ $^ $(GUI_LDLIBS)

$(TEST_TARGET): $(CORE_OBJS) $(TEST_OBJS)
	$(CC) -o $@ $^ $(CLI_LDLIBS)

# Suppress third-party raygui header warnings in main_gui.c
$(OBJ_DIR)/main_gui.o: CFLAGS += -Wno-unused-function -Wno-unused-parameter -Wno-format -Wno-implicit-fallthrough

# Compile standard C sources
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile test C sources
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create object directory
$(OBJ_DIR):
	mkdir -p $@

# Debug variants
debug:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) $(DEBUG)" all

# Clean artifacts
clean:
	rm -rf $(CLI_TARGET) $(GUI_TARGET) $(TEST_TARGET) $(OBJ_DIR)

# Dependencies include
-include $(CORE_OBJS:.o=.d) $(CLI_OBJS:.o=.d) $(GUI_OBJS:.o=.d) $(TEST_OBJS:.o=.d)
