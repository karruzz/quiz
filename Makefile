CPPFLAGS = -Wall -std=c++11
LDFLAGS =
CC = g++
INCLUDES = -Isrc/include -I/usr/include/pocketsphinx -I/usr/include/x86_64-linux-gnu/sphinxbase
OUT_DIR = output
OBJ_DIR = $(OUT_DIR)/objs
OUT_O_DIR = $(OBJ_DIR)/release
LIBS = -lboost_filesystem -lboost_system -lncursesw -lpocketsphinx -lsphinxbase -lsphinxad
SOURCES = src/learn.cpp src/parser.cpp src/ncurces_screen.cpp src/record.cpp
TARGET = learn

dir_guard=@mkdir -p $(@D)

ifdef debug
ifeq ($(debug),1)
	CPPFLAGS+=-DDEBUG -g -O0
	TARGET:=$(TARGET)d
	OUT_O_DIR = $(OBJ_DIR)/debug
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
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OUT_O_DIR)/%.o: src/%.cpp
	$(dir_guard)
	$(CC) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OUT_DIR) $(TARGET) $(TARGET)d
