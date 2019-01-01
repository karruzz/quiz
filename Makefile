GTEST_DIR = ../../Gtest/googletest

CPPFLAGS = -Wall -std=c++17 -pthread
LDFLAGS =
CC = g++
INCLUDES = -Iinclude -I/usr/include/pocketsphinx -I/usr/include/x86_64-linux-gnu/sphinxbase -isystem ${GTEST_DIR}/include -I${GTEST_DIR}
BIN_DIR = bin
BUILD_DIR = build
LIBS = -lncursesw -lpocketsphinx -lsphinxbase -lsphinxad -lstdc++fs
SOURCES = src/log.cpp src/quiz.cpp src/parser.cpp src/analyzer.cpp src/voice.cpp src/view/ncurses/ncurses_screen.cpp src/view/ncurses/window.cpp src/view/ncurses/editor.cpp src/utils.cpp

dir_guard=@mkdir -p $(@D)

OUT_O_DIR = $(BUILD_DIR)/release
TARGET = $(BIN_DIR)/quiz
ANALYZER_TEST=$(BIN_DIR)/analyzer-test

ifdef debug
ifeq ($(debug),1)
	CPPFLAGS+=-DDEBUG -g -O0
	OUT_O_DIR = $(BUILD_DIR)/debug
	TARGET = $(BIN_DIR)/quizd
endif
endif

ifdef sanit
ifeq ($(sanit),1)
	CPPFLAGS+=-fsanitize=address
	LDFLAGS+=-fsanitize=address
endif
endif

TEST_O_DIR = $(OUT_O_DIR)/test

OBJECTS = $(patsubst src/%.cpp, $(OUT_O_DIR)/%.o, $(SOURCES))

.PHONY: all, clean

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS) $(ANALYZER_TEST)
	$(dir_guard)
	./$(ANALYZER_TEST)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

$(OUT_O_DIR)/%.o: src/%.cpp
	$(dir_guard)
	$(CC) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

$(ANALYZER_TEST): $(TEST_O_DIR)/analyzer-test.o $(TEST_O_DIR)/libgtest.a $(OUT_O_DIR)/analyzer.o $(OUT_O_DIR)/utils.o
	$(dir_guard)
	$(CC) $(CPPFLAGS) -o $@ $^

$(TEST_O_DIR)/analyzer-test.o: test/analyzer-test.cpp
	$(dir_guard)
	$(CC) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

$(TEST_O_DIR)/libgtest.a: $(TEST_O_DIR)/gtest-all.o
	ar -rv $@ $^

$(TEST_O_DIR)/gtest-all.o: ${GTEST_DIR}/src/gtest-all.cc
	$(CC) $(CPPFLAGS) $(INCLUDES) -c -o $@ $^

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)
