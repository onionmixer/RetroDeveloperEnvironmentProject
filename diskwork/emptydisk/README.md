# diskwork/emptydisk

Per-platform **empty / pre-formatted** disk image templates. Mirrors the
layout of `diskwork/bootdisk/` (per-platform subdir) but holds blank
volumes you can clone as starting points for new disk work, instead of
the bootable System disks under `bootdisk/`.

| Platform | Subdir | Templates |
|----------|--------|-----------|
| Apple II | `AppleII/` | `empty_dos33.dsk` (DOS 3.3 140 KB), `empty_prodos.po` (ProDOS 140 KB) |
| MSX | `msx/` | `empty_msxdos.dsk` (MSX-DOS 720 KB) + `MSXDiskImage` GUI tool |
| X68000 | `x68000/` | `empty_human68k.xdf`, `empty_human68k.dim` (Human68k 2HD) |
| Macintosh | `macintosh/` | `empty_hfs_1440.img`, `empty_hfs_800.img` |

All templates are reproducible via `rdedisktool create` — see each
subdir's `README.md` for the exact command.

## Workflow

```bash
RDE=./RetroDeveloperEnvironmentDisktool/build/rdedisktool

# 1. Pick a template, copy it to your work area
cp diskwork/emptydisk/macintosh/empty_hfs_1440.img /tmp/work.img

# 2. Add files to it
$RDE add /tmp/work.img myfile.txt
$RDE add --macbinary /tmp/work.img MyApp.bin     # Mac apps with resource fork

# 3. Mount in the appropriate emulator (snow, openMSX, AppleWin, px68k)
```

## Where to put non-empty disks

- Bootable System disks (canonical) → `diskwork/bootdisk/<platform>/`
- Per-example derived/test disks → inside the example dir
- ROMs (copyrighted) → `resource/<Platform>/rom/`
