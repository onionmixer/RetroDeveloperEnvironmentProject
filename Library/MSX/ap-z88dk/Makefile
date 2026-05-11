Z80ASM := /opt/z88dk/bin/z80asm
LIB := lib/ap.lib
SRC_DIR := src
SOURCES := $(wildcard $(SRC_DIR)/*.asm)
OBJS := $(SOURCES:.asm=.o)

all: $(LIB)

$(LIB): $(OBJS)
	mkdir -p lib
	$(Z80ASM) -x$(LIB) $(OBJS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.asm
	$(Z80ASM) $<

clean:
	rm -f $(SRC_DIR)/*.o $(SRC_DIR)/*.lis $(SRC_DIR)/*.sym $(LIB)

.PHONY: all clean
