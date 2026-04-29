# Macintosh ROM files

snow (`Emulator/macintosh/snow`) emulates classic Macs (Mac 128K through SE/30) and **requires** a model-appropriate ROM file. ROMs are copyrighted by Apple and are **not** redistributed in this repository — drop your own copy here.

## Expected layout

```
resource/Macintosh/rom/
├── plus.rom        # Mac Plus            (128 KiB, sig 4D1F8172)
├── SE_FDHD.rom     # Mac SE FDHD         (256 KiB, sig B306E171)
├── Classic.rom     # Mac Classic         (512 KiB)
├── SE30.rom        # Mac SE/30           (256 KiB)
├── MacII.rom       # Mac II / IIx / IIcx (256 KiB)
└── 341-0868.bin    # Mac II Toby Frame Buffer (Display Card) ROM, required for Mac II
```

`run_snow_mac.sh` defaults to **Mac SE FDHD** (`MAC_MODEL=se_fdhd`) — SuperDrive supports both 800K and 1.44 MB media so System 6.x / 7.x master disks boot out of the box. Switch models with `MAC_MODEL=plus` (800K-only) / `classic` / `se30` / `macii`, or pin a path with `MAC_ROM=…`. The launcher accepts both lowercase (`se_fdhd.rom`) and the `macmade/Macintosh-ROMs` casing (`SE_FDHD.rom`).

## Verifying integrity

snow validates the ROM on load and shows the detected model in its title bar. If the file is wrong size / wrong checksum, snow refuses to boot — fix the ROM rather than working around it.

Sanity-check before placing a downloaded ROM: many archives prepend or append metadata that breaks the strict size check. The launcher prints the file size and the first 4 bytes (checksum signature) before invoking snow — known-good signatures: Plus = `4d1f8172`, SE FDHD = `b306e171`. If the size is wrong, truncate to the standard size with `truncate -s <bytes>`.

## Why these aren't committed

ROMs are matched by `resource/Macintosh/rom/*` in the project `.gitignore` (with this README explicitly allowed). Don't `git add -f` them.

## etc

https://github.com/macmade/Macintosh-ROMs
https://vintageapple.org/gamba2/os8_68030.html
