# diskwork/emptydisk/x68000

Empty / pre-formatted X68000 2HD floppy templates with Human68k
filesystem. 1024-byte sectors (X68000-specific, not the standard PC
512-byte). Clone and use as a starting point.

## Contents (regenerable)

| File | Format | Geometry | Filesystem |
|------|--------|----------|-----------|
| `empty_human68k.xdf` | XDF (raw 2HD) | 154 × 2 × 8 × 1024 | Human68k (FAT12) |
| `empty_human68k.dim` | DIM (256 B header + raw) | 154 × 2 × 8 × 1024 | Human68k (FAT12) |

Volume name is `EMPTY`. Both are non-bootable (no IPL); for a bootable
Human68k disk see `diskwork/bootdisk/x68000/HUMAN302.XDF`.

## Regenerating

```bash
RDE=./RetroDeveloperEnvironmentDisktool/build/rdedisktool
$RDE create diskwork/emptydisk/x68000/empty_human68k.xdf -f xdf --fs human68k -n EMPTY --force
$RDE create diskwork/emptydisk/x68000/empty_human68k.dim -f dim --fs human68k -n EMPTY --force
```
