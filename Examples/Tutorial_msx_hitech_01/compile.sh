#!/bin/bash
#
# compile.sh - MSX-DOS program compilation using Hi-Tech C native toolchain
#
# This script demonstrates compiling programs for MSX-DOS using
# Hi-Tech C Z80 cross compiler (native Linux version)
#

set -e

# === Configuration ===
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
HITECH="$PROJECT_ROOT/Toolchain/MSX/HITECH_TOOLCHAIN"
SHARKSYM="$HITECH/lib/CPMEMU_HI-TECH_C"
RDEDISKTOOL="$PROJECT_ROOT/RetroDeveloperEnvironmentDisktool/build/rdedisktool"
BOOT_DISK="$PROJECT_ROOT/diskwork/bootdisk/msx/msxdos23.dsk"

# Output files
OUTPUT_NAME="HELLO"
DISK_IMAGE="$BUILD_DIR/disk.dsk"

# === Functions ===

print_header() {
    echo "========================================"
    echo "$1"
    echo "========================================"
}

# Clean build directory
clean() {
    print_header "Cleaning build directory"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    echo "Done."
}

# Method 1: Pure assembly (working method)
# This method uses hardcoded addresses and produces working MSX-DOS executables
build_asm() {
    print_header "Building from assembly source"

    cd "$BUILD_DIR"

    # Create assembly source with hardcoded 0x100 base address
    cat > hello.as << 'EOF'
; hello.as - Hello world for MSX-DOS
; Uses BDOS function 9 to print string
; Addresses are hardcoded for 0x100 load address

    psect   text,global,pure

    global  start

start:
    ; msg is at file offset 0x0D, memory 0x10D
    ld      de, 10Dh    ; message address (0x100 + 0x0D)
    ld      c, 9        ; BDOS print string function
    call    5           ; BDOS entry point

    ld      c, 0        ; BDOS program terminate
    jp      5           ; Exit to DOS

msg:
    defm    'hello world'
    defb    13, 10, '$'

    end     start
EOF

    echo "Assembling hello.as..."
    "$HITECH/bin/zasx3" -j hello.as

    echo "Linking..."
    "$HITECH/bin/linq3" -Ptext=0 -ohello.out hello.obj

    echo "Converting to Intel HEX..."
    "$HITECH/bin/objtohex" hello.out hello.hex

    echo "Converting to binary..."
    # Extract hex data and convert to binary
    # HEX format: :LLAAAATT[DD...]CC
    grep "^:" hello.hex | head -n -1 | while read line; do
        # Remove : prefix, length (2), address (4), type (2), checksum (2)
        data="${line:9:-2}"
        echo -n "$data"
    done | xxd -r -p > "$OUTPUT_NAME.COM"

    echo "Created: $OUTPUT_NAME.COM ($(stat -c%s "$OUTPUT_NAME.COM") bytes)"
}

# Method 2: C source compilation pipeline (reference)
# Note: This produces relocatable code that needs proper CRT setup
build_c() {
    print_header "Building from C source (reference)"

    cd "$BUILD_DIR"

    # Create C source
    cat > hello.c << 'EOF'
/* hello.c - Simple hello world for MSX-DOS */
int main(void)
{
#asm
    ld de, msg
    ld c, 9
    call 5
#endasm
    return 0;
}

#asm
    psect data
msg:
    defm 'hello world'
    defb 13, 10, '$'
#endasm
EOF

    echo "Step 1: Preprocessing (cpp_new3)..."
    "$HITECH/bin/cpp_new3" -DCPM -DMSX -I"$SHARKSYM" hello.c hello.i

    echo "Step 2: Parsing (p1x3)..."
    "$HITECH/bin/p1x3" hello.i hello.p1

    echo "Step 3: Code generation (cgen3)..."
    "$HITECH/bin/cgen3" hello.p1 hello.as

    echo "Step 4: Optimization (optim3)..."
    "$HITECH/bin/optim3" hello.as hello.opt

    echo "Step 5: Assembly (zasx3)..."
    "$HITECH/bin/zasx3" -j hello.opt
    mv hello.obj hello_c.obj

    echo "C compilation pipeline completed."
    echo "Note: Linking requires proper CRT setup for address relocation."
    echo "      See sharksym CRT.O or create custom CRT for MSX-DOS."
}

# Create disk image with compiled program
create_disk() {
    print_header "Creating disk image"

    cd "$BUILD_DIR"

    if [ ! -f "$OUTPUT_NAME.COM" ]; then
        echo "Error: $OUTPUT_NAME.COM not found. Run build_asm first."
        exit 1
    fi

    echo "Creating MSX-DOS disk image..."
    "$RDEDISKTOOL" create "$DISK_IMAGE" -f msxdsk --fs msxdos --force
    if ! "$RDEDISKTOOL" info "$DISK_IMAGE" | rg -q "File System: MSX-DOS"; then
        echo "Error: created image is not recognized as MSX-DOS."
        "$RDEDISKTOOL" info "$DISK_IMAGE" || true
        exit 1
    fi

    echo "Adding $OUTPUT_NAME.COM to disk..."
    "$RDEDISKTOOL" add "$DISK_IMAGE" "$OUTPUT_NAME.COM"

    echo "Disk contents:"
    "$RDEDISKTOOL" list "$DISK_IMAGE"

    echo ""
    echo "Disk image created: $DISK_IMAGE"
}

# Run in openMSX emulator
run_emulator() {
    print_header "Running in openMSX"

    OPENMSX="$PROJECT_ROOT/Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx"

    if [ ! -f "$OPENMSX" ]; then
        echo "Error: openMSX not found at $OPENMSX"
        exit 1
    fi

    if [ ! -f "$BOOT_DISK" ]; then
        echo "Error: Boot disk not found at $BOOT_DISK"
        exit 1
    fi

    echo "Starting openMSX with MSX-DOS 2..."
    echo "Boot disk: $BOOT_DISK"
    echo ""
    echo "After MSX-DOS boots:"
    echo "  1. Change disk to: $DISK_IMAGE"
    echo "  2. Run: $OUTPUT_NAME"
    echo ""

    "$OPENMSX" -machine Panasonic_FS-A1GT -diska "$BOOT_DISK" &
}

# Show usage
usage() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  clean       - Clean build directory"
    echo "  asm         - Build from assembly source (recommended)"
    echo "  c           - Build from C source (reference only)"
    echo "  disk        - Create disk image with compiled program"
    echo "  run         - Run openMSX emulator"
    echo "  all         - Clean, build asm, create disk"
    echo ""
    echo "Example workflow:"
    echo "  $0 all      # Build everything"
    echo "  $0 run      # Test in emulator"
}

# === Main ===

case "${1:-all}" in
    clean)
        clean
        ;;
    asm)
        mkdir -p "$BUILD_DIR"
        build_asm
        ;;
    c)
        mkdir -p "$BUILD_DIR"
        build_c
        ;;
    disk)
        create_disk
        ;;
    run)
        run_emulator
        ;;
    all)
        clean
        build_asm
        create_disk
        echo ""
        print_header "Build complete!"
        echo "Run '$0 run' to test in openMSX emulator"
        ;;
    *)
        usage
        exit 1
        ;;
esac
