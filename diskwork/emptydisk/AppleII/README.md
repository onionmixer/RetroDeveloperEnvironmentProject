# diskwork/emptydisk/AppleII

Empty / pre-formatted Apple II 5.25" floppy templates (140 KB each).
Clone and use as a starting point; add files via `rdedisktool add`
with the appropriate `--type` / `--addr`.

## Contents (regenerable)

| File | Order | Filesystem |
|------|-------|-----------|
| `empty_dos33.dsk` | DOS-order | DOS 3.3 |
| `empty_prodos.po` | ProDOS-order | ProDOS (volume name `EMPTY`) |

Both are 140 KB (35 tracks × 16 sectors × 256 bytes). DOS 3.3 disks
are not bootable — copy a DOS image's first 3 tracks (or use one of
the boot disks under `diskwork/bootdisk/AppleII/`) if you need the
disk to boot.

## Regenerating

```bash
RDE=./RetroDeveloperEnvironmentDisktool/build/rdedisktool
$RDE create diskwork/emptydisk/AppleII/empty_dos33.dsk -f do --fs dos33 --force
$RDE create diskwork/emptydisk/AppleII/empty_prodos.po -f po --fs prodos -n EMPTY --force
```
