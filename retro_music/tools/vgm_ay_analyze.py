#!/usr/bin/env python3
"""Analyze an AY-3-8910/8912 VGM dump for retro replayer feasibility.

Reconstructs per-frame (50Hz) register states and measures the cost of
several candidate stream encodings:
  raw14   : 14 bytes/frame full dump
  maskval : 2-byte change bitmask + changed register values (+RLE idle frames)
  regval  : (reg,val) pairs + 1 frame terminator byte (MSX .PSG-log style)
  zlib    : zlib-compressed maskval stream (proxy for ZX0/Pletter)
  uniq    : unique full-state dictionary + per-frame index (AKY-ish upper bound)
"""
import struct, sys, zlib, collections

FRAME = 882  # 50 Hz in 44100 samples


def parse(path):
    d = open(path, 'rb').read()
    off = struct.unpack_from('<I', d, 0x34)[0] + 0x34
    loop_off = struct.unpack_from('<I', d, 0x1C)[0]
    loop_abs = loop_off + 0x1C if loop_off else None
    i = off
    t = 0
    writes = []  # (sample_time, reg, val)
    loop_time = None
    while i < len(d):
        if loop_abs is not None and i == loop_abs:
            loop_time = t
        c = d[i]
        if c == 0xA0:
            writes.append((t, d[i+1] & 0x7F, d[i+2])); i += 3
        elif c == 0x61:
            t += struct.unpack_from('<H', d, i+1)[0]; i += 3
        elif c == 0x62:
            t += 735; i += 1
        elif c == 0x63:
            t += 882; i += 1
        elif 0x70 <= c <= 0x7F:
            t += (c & 0xF) + 1; i += 1
        elif c == 0x66:
            break
        elif c == 0x67:
            sz = struct.unpack_from('<I', d, i+3)[0]; i += 7 + sz
        else:
            sys.exit(f"unhandled cmd {c:02x} at {i}")
    return writes, t, loop_time


def frames_of(writes, total):
    nf = (total + FRAME - 1) // FRAME
    state = [0]*16
    frames = []
    r13_hits = []
    wi = 0
    for f in range(nf):
        end = (f+1)*FRAME
        r13 = False
        while wi < len(writes) and writes[wi][0] < end:
            _, r, v = writes[wi]
            if r < 16:
                state[r] = v
                if r == 13:
                    r13 = True
            wi += 1
        frames.append(tuple(state[:14]))
        r13_hits.append(r13)
    return frames, r13_hits


def main(path):
    writes, total, loop_time = parse(path)
    frames, r13 = frames_of(writes, total)
    nf = len(frames)
    print(f"file: {path}")
    print(f"total samples {total} = {total/44100:.1f}s, frames(50Hz) {nf}, "
          f"AY writes {len(writes)}, loop@sample {loop_time}")

    # per-frame diffs
    prev = None
    changed_hist = collections.Counter()
    reg_changes = collections.Counter()
    maskval = bytearray()
    regval = bytearray()
    idle_run = 0
    for f, (st, retrig) in enumerate(zip(frames, r13)):
        if prev is None:
            diff = list(range(14))
        else:
            diff = [r for r in range(14) if st[r] != prev[r]]
            if retrig and 13 not in diff:
                diff.append(13)
        changed_hist[len(diff)] += 1
        for r in diff:
            reg_changes[r] += 1
        if not diff:
            idle_run += 1
        else:
            while idle_run > 0:
                run = min(idle_run, 255)
                maskval += bytes((0, 0, run))
                idle_run -= run
            m = 0
            for r in diff:
                m |= 1 << r
            maskval += struct.pack('<H', m) + bytes(st[r] for r in sorted(diff))
        for r in sorted(diff):
            regval += bytes((r, st[r]))
        regval += b'\xff'
        prev = st
    while idle_run > 0:
        run = min(idle_run, 255)
        maskval += bytes((0, 0, run)); idle_run -= run

    uniq = len(set(frames))
    raw14 = nf * 14
    uniq_cost = uniq*14 + nf*2
    z = len(zlib.compress(bytes(maskval), 9))
    print(f"\nwrites/frame hist (changes): "
          f"{dict(sorted(changed_hist.items()))}")
    avg = sum(k*v for k, v in changed_hist.items())/nf
    print(f"avg changed regs/frame {avg:.2f}, idle frames "
          f"{changed_hist[0]} ({changed_hist[0]*100/nf:.0f}%)")
    print(f"per-reg change counts: "
          f"{[reg_changes[r] for r in range(14)]}")
    print(f"unique full states: {uniq}")
    print("\nencoding sizes:")
    print(f"  raw14           : {raw14:7d} B")
    print(f"  maskval+RLE     : {len(maskval):7d} B")
    print(f"  regval pairs    : {len(regval):7d} B")
    print(f"  zlib(maskval)   : {z:7d} B   (LZ proxy)")
    print(f"  uniq dict+index : {uniq_cost:7d} B   ({uniq}*14 + {nf}*2)")


if __name__ == '__main__':
    main(sys.argv[1])
