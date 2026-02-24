Tutorial_apple_dos33_01 Build Guide

Purpose
- Build a DOS 3.3-ready Apple II binary from hello.c and keep the output as HELLO.

Prerequisites
- cc65 toolchain installed (cl65, ld65)

Files in this directory
- hello.c : source code
- HELLO   : DOS 3.3 insertion-ready raw binary payload (no AppleSingle wrapper)
- Tutorial_apple_dos33_01.do : sample DOS 3.3 disk image

Build Steps
1. Compile with cc65 apple2 target
   cl65 -t apple2 -O -o HELLO hello.c

2. Convert apple2 output to raw payload for DOS 3.3 insertion
   (cc65 apple2 output may include AppleSingle header)
   tail -c +59 HELLO > HELLO.tmp
   mv HELLO.tmp HELLO

3. Quick check (optional)
   file HELLO
   xxd -g 1 -l 16 HELLO

Using with rdedisktool (example)
- Add HELLO to DOS 3.3 disk as binary with load address:
  rdedisktool add Tutorial_apple_dos33_01.do ./HELLO HELLO --type B --addr 0x0803

Notes
- This tutorial keeps only one executable payload file name: HELLO.
- If you recompile, run step 2 again to keep HELLO as raw binary form.
