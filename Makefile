CPPFLAGS = -g -Wall -std=c++11
CC = g++
INCLUDES = src/include
OUT_DIR = output/
OUT_O_DIR = $(OUT_DIR)objs/
LIBS = -lboost_filesystem -lboost_system -lncurses
SOURCES = src/learn.cpp src/parser.cpp src/viewer.cpp src/ncurces_screen.cpp
HEADERS=src/include/parser.h src/include/viewer.h
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
	$(CC) $(CPPFLAGS) -I$(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OUT_DIR) $(TARGET) $(TARGET)d