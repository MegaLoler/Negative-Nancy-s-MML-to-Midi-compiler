# paths
SRC_PATH	 ::= src
BUILD_PATH	 ::= build
EXAMPLE_PATH ::= examples
INSTALL_PATH ::= /usr/bin

# programs
CC           ::= g++ -I$(SRC_PATH) -Wall -Og -g
DEBUGGER     ::= gdb
PLAYER       ::= timidity

# targets
TARGET       ::= build/nnmmlc



all: $(TARGET)

$(TARGET): $(BUILD_PATH)/main.o
	$(CC) $(BUILD_PATH)/main.o -o $(TARGET)

$(BUILD_PATH)/main.o: $(BUILD_PATH) $(SRC_PATH)/main.cpp
	$(CC) -c $(SRC_PATH)/main.cpp -o $(BUILD_PATH)/main.o

.PHONY:
$(BUILD_PATH):
	mkdir $(BUILD_PATH)

.PHONY:
clean: $(BUILD_PATH)
	rm -rf $(BUILD_PATH)

.PHONY:
test: $(TARGET)
	$(TARGET) < $(EXAMPLE_PATH)/twinkle.mml > $(BUILD_PATH)/twinkle.midi
	$(PLAYER) $(BUILD_PATH)/twinkle.midi

.PHONY:
debug: $(TARGET)
	$(DEBUGGER) $(TARGET)
