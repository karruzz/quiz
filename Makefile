CPPFLAGS = -Wall -std=c++11
LDFLAGS =
CC = g++
INCLUDES = -Iinclude -I/usr/include/pocketsphinx -I/usr/include/x86_64-linux-gnu/sphinxbase
BIN_DIR = bin
BUILD_DIR = build
LIBS = -lboost_filesystem -lboost_system -lncursesw -lpocketsphinx -lsphinxbase -lsphinxad
SOURCES = src/learn.cpp src/parser.cpp src/record.cpp src/view/ncurses/ncurses_screen.cpp src/view/ncurses/window.cpp src/view/ncurses/editor.cpp

dir_guard=@mkdir -p $(@D)

OUT_O_DIR = $(BUILD_DIR)/release
TARGET = $(BIN_DIR)/learn

ifdef debug
ifeq ($(debug),1)
	CPPFLAGS+=-DDEBUG -g -O0
	OUT_O_DIR = $(BUILD_DIR)/debug
	TARGET = $(BIN_DIR)/learnd
endif
endif

ifdef sanit
ifeq ($(sanit),1)
	CPPFLAGS+=-fsanitize=address
	LDFLAGS+=-fsanitize=address
endif
endif

OBJECTS = $(patsubst src/%.cpp, $(OUT_O_DIR)/%.o, $(SOURCES))

.PHONY: all, clean

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(dir_guard)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OUT_O_DIR)/%.o: src/%.cpp
	$(dir_guard)
	$(CC) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR)
