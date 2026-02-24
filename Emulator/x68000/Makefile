# X68000 Human68k Cross Compile Makefile
# Toolchain: elf2x68k (m68k-xelf-gcc)

# Directories
TOOLCHAIN_DIR := $(CURDIR)/../../Toolchain/x68000/toolchain/m68k-xelf
SRC_DIR := src
BUILD_DIR := build

# Toolchain
export PATH := $(TOOLCHAIN_DIR)/bin:$(PATH)
CC := m68k-xelf-gcc
RUN68 := run68

# Compiler flags
CFLAGS := -O2 -Wall
LDFLAGS :=

# CPU target (68000, 68020, 68030, 68040, 68060)
CPU ?= 68000
CFLAGS += -m$(CPU)

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.c)
TARGETS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.x,$(SRCS))

# Default target
all: $(BUILD_DIR) $(TARGETS)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile .c to .x
$(BUILD_DIR)/%.x: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

# Run with run68 emulator
run: $(BUILD_DIR)/hello.x
	$(RUN68) $<

# Run specific target: make run-<name>
run-%: $(BUILD_DIR)/%.x
	$(RUN68) $<

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Show toolchain info
info:
	@echo "=== X68000 Cross Compile Environment ==="
	@echo "Toolchain: $(TOOLCHAIN_DIR)"
	@echo "Compiler:  $(CC)"
	@echo "CPU:       $(CPU)"
	@echo "Sources:   $(SRCS)"
	@echo "Targets:   $(TARGETS)"
	@echo ""
	@$(CC) --version | head -1
	@echo ""
	@$(RUN68) -h 2>&1 | head -3 || true

# Rebuild everything
rebuild: clean all

# List available examples
list:
	@echo "Available examples:"
	@echo ""
	@echo "Basic:"
	@echo "  hello          - Basic Hello World (printf)"
	@echo "  hello_iocs     - IOCS system calls"
	@echo "  hello_dos      - DOS system calls"
	@echo ""
	@echo "Graphics:"
	@echo "  hello_gfx      - Graphics drawing"
	@echo "  hello_sprite   - Sprite system"
	@echo "  hello_bg       - Background/tilemap"
	@echo "  hello_raster   - Raster interrupt effects"
	@echo "  hello_crtc     - CRTC screen modes"
	@echo ""
	@echo "Sound:"
	@echo "  hello_snd      - FM/PSG sound (OPM/PSG)"
	@echo "  hello_adpcm    - ADPCM playback"
	@echo "  hello_midi     - MIDI output"
	@echo ""
	@echo "Input:"
	@echo "  hello_key      - Keyboard input"
	@echo "  hello_mouse    - Mouse input"
	@echo "  hello_joy      - Joystick input"
	@echo ""
	@echo "Storage:"
	@echo "  hello_file     - File I/O"
	@echo "  hello_fdd      - Floppy disk access"
	@echo "  hello_scsi     - SCSI hard disk"
	@echo ""
	@echo "System:"
	@echo "  hello_mem      - Memory management"
	@echo "  hello_timer    - Timer/interrupt"
	@echo "  hello_dma      - DMA transfer"
	@echo "  hello_rtc      - Real-time clock"
	@echo ""
	@echo "Peripherals:"
	@echo "  hello_serial   - RS-232C serial"
	@echo "  hello_printer  - Printer output"
	@echo ""
	@echo "Game:"
	@echo "  hello_game     - Simple game example"

# Help
help:
	@echo "X68000 Human68k Build System"
	@echo ""
	@echo "Usage:"
	@echo "  make              - Build all programs"
	@echo "  make run          - Build and run hello.x"
	@echo "  make run-NAME     - Build and run NAME.x"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make rebuild      - Clean and rebuild"
	@echo "  make info         - Show toolchain info"
	@echo "  make list         - List all examples"
	@echo ""
	@echo "Options:"
	@echo "  CPU=68000         - Target CPU (68000/68020/68030/68040/68060)"
	@echo ""
	@echo "Examples:"
	@echo "  make                      - Build all examples"
	@echo "  make build/hello_gfx.x    - Build specific example"
	@echo "  make run-hello_gfx        - Run graphics example"
	@echo "  make run-hello_snd        - Run sound example"
	@echo "  make run-hello_crtc       - Run CRTC example"
	@echo "  make CPU=68030            - Build for 68030"

.PHONY: all run clean info rebuild help list
