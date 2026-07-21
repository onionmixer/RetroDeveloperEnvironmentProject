#!/usr/bin/env python3
"""vgm2ym — convert an AY-3-8910/8912 VGM dump to YM5 (interleaved).

The YM file can then be fed to Arkos Tracker 3 (GUI or SongToAkg/Akm/Aky
command-line tools), which accept YM as song input.  This bridges
"VGM register log" -> "AT3 note/pattern model" -> AKY/AKG/AKM exports.

Usage: vgm2ym.py input.vgm output.ym [--rate 50|60]
"""
import struct, sys, argparse

FRAME50 = 882   # samples per frame @44100, 50 Hz
FRAME60 = 735


def parse_vgm(d):
    if d[:4] != b'Vgm ':
        sys.exit("not a VGM file (gzip .vgz? gunzip first)")
    ver = struct.unpack_from('<I', d, 8)[0]
    data_off = struct.unpack_from('<I', d, 0x34)[0] + 0x34 if ver >= 0x150 else 0x40
    ay_clock = struct.unpack_from('<I', d, 0x74)[0] if len(d) > 0x78 else 0
    loop_off = struct.unpack_from('<I', d, 0x1C)[0]
    loop_abs = loop_off + 0x1C if loop_off else None
    i, t = data_off, 0
    writes = []
    loop_time = 0
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
            sys.exit(f"unhandled VGM cmd {c:02x} at offset {i}")
    return writes, t, ay_clock, loop_time


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('vgm'); ap.add_argument('ym')
    ap.add_argument('--rate', type=int, default=50, choices=(50, 60))
    a = ap.parse_args()
    d = open(a.vgm, 'rb').read()
    if d[:2] == b'\x1f\x8b':
        import gzip
        d = gzip.decompress(d)
    writes, total, clock, loop_time = parse_vgm(d)
    if not clock:
        clock = 1789773
    frame = FRAME50 if a.rate == 50 else FRAME60
    nf = (total + frame - 1) // frame
    # build per-frame register matrix; reg13 = 0xFF when not rewritten (YM rule)
    state = [0]*16
    rows = []
    wi = 0
    for f in range(nf):
        end = (f+1)*frame
        r13 = 0xFF
        while wi < len(writes) and writes[wi][0] < end:
            _, r, v = writes[wi]
            state[r] = v
            if r == 13:
                r13 = v
            wi += 1
        row = list(state[:16])
        row[13] = r13          # 0xFF = "do not retrigger envelope"
        row[14] = row[15] = 0  # no digidrums
        rows.append(row)
    loop_frame = loop_time // frame
    hdr = b'YM5!' + b'LeOnArD!'
    hdr += struct.pack('>I', nf)          # frame count
    hdr += struct.pack('>I', 1)           # attributes: bit0 = interleaved
    hdr += struct.pack('>H', 0)           # digidrum count
    hdr += struct.pack('>I', clock)       # master clock
    hdr += struct.pack('>H', a.rate)      # player rate
    hdr += struct.pack('>I', loop_frame)  # loop frame
    hdr += struct.pack('>H', 0)           # additional data size
    hdr += b'converted\x00' + b'vgm2ym\x00' + b'\x00'   # name/author/comment
    body = bytearray()
    for r in range(16):                   # interleaved: reg-major
        for row in rows:
            body.append(row[r])
    out = hdr + bytes(body) + b'End!'
    open(a.ym, 'wb').write(out)
    print(f"{a.ym}: {nf} frames @{a.rate}Hz, clock {clock}, "
          f"loop frame {loop_frame}, {len(out)} bytes")


if __name__ == '__main__':
    main()
