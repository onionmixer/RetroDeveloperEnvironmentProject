# diskwork/emptydisk/msx

Empty / pre-formatted MSX disk templates **and** the bundled
`MSXDiskImage` GUI tool (Pascal/Delphi, originally at `diskwork/msxdiskimage/`
before the 2026-04-29 reshuffle).

## Empty disk template

| File | Format | Geometry | Filesystem |
|------|--------|----------|-----------|
| `empty_msxdos.dsk` | Raw 720 KB | 80 × 2 × 9 × 512 | MSX-DOS 1 |

Volume name `EMPTY`, no boot code. For a bootable MSX-DOS disk see
`diskwork/bootdisk/msx/msxdos23.dsk`.

```bash
RDE=./RetroDeveloperEnvironmentDisktool/build/rdedisktool
$RDE create diskwork/emptydisk/msx/empty_msxdos.dsk -f msxdsk --fs msxdos -n EMPTY --force
```

## MSXDiskImage tool (`Source/`, `MSXDiskImage`, `MSXDiskImage.exe`)

A Lazarus / Free Pascal GUI for constructing MSX disk images. Lives
here as a historical artifact and as a reference for MSX disk format
edge cases. Not used by our build pipeline (the rdedisktool CLI
handles everything we need); kept for occasional inspection of the
`MSXEngine/` source.

If you want to rebuild it, open `Source/MSXDiskImage.lpi` in Lazarus.
