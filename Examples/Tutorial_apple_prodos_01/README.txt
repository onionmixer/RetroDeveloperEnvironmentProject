Tutorial_apple_prodos_01 Build Guide

Purpose
- Build a ProDOS-ready Apple II binary from hello.c and keep the output as HELLO.

Prerequisites
- cc65 toolchain installed (cl65, ld65)

Files in this directory
- hello.c : source code
- HELLO   : ProDOS-ready raw binary payload (no AppleSingle wrapper)
- Tutorial_apple_prodos_01.po : sample ProDOS disk image

Build Steps
1. Compile with cc65 apple2 target
   cl65 -t apple2 -O -o HELLO hello.c

2. Convert apple2 output to raw payload for ProDOS disk insertion
   (cc65 apple2 output may include AppleSingle header)
   tail -c +59 HELLO > HELLO.tmp
   mv HELLO.tmp HELLO

3. Quick check (optional)
   file HELLO
   xxd -g 1 -l 16 HELLO

Using with rdedisktool (example)
- Add HELLO to a ProDOS disk as BIN with load address:
  rdedisktool add Tutorial_apple_prodos_01.po ./HELLO HELLO --type B --addr 0x0803

Notes
- This tutorial keeps only one executable payload file name: HELLO.
- If you recompile, run step 2 again to keep HELLO as raw binary form.
