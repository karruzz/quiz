CPPFLAGS = -g -Wall -std=c++11
CC = g++
INCLUDES = -Isrc/include
OUT_DIR = output/
OUT_O_DIR = $(OUT_DIR)objs/
LIBS = -lboost_filesystem -lboost_system -lncurses
SOURCES = src/learn.cpp src/parser.cpp src/viewer.cpp
OBJECTS = $(patsubst src/%.cpp, $(OUT_O_DIR)%.o, $(SOURCES))
TARGET = $(OUT_DIR)learn

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
	rm -rf $(OUT_DIR)