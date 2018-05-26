CPPFLAGS = -g -Wall -std=c++11
CC = g++
INCLUDES = -Isrc/include -I/usr/include/pocketsphinx -I/usr/include/x86_64-linux-gnu/sphinxbase
OUT_DIR = output/
OUT_O_DIR = $(OUT_DIR)objs/
LIBS = -lboost_filesystem -lboost_system -lncursesw -lpocketsphinx -lsphinxbase -lsphinxad
SOURCES = src/learn.cpp src/parser.cpp src/ncurces_screen.cpp src/record.cpp
OBJECTS = $(patsubst src/%.cpp, $(OUT_O_DIR)%.o, $(SOURCES))
TARGET = learn

dir_guard=@mkdir -p $(@D)

ifdef debug
ifeq ($(debug),1)
CPPFLAGS+=-DDEBUG
TARGET:=$(TARGET)d
endif
endif

.PHONY: all, clean

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

$(OUT_O_DIR)%.o: src/%.cpp
	$(dir_guard)
	$(CC) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OUT_DIR) $(TARGET) $(TARGET)d