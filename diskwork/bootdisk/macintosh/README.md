# Macintosh boot disks

Drop bootable Mac System floppies (System 6.x / 7.x) here. Formats accepted by snow:

- **DiskCopy 4.2** (`.image`, `.dc42`) — preferred for stock Apple distributions
- **Raw sector image** (`.img`, `.dsk`) — what `rdedisktool create -f mac_img --fs hfs` produces

Files in this directory are gitignored (see project `.gitignore`); only this README is tracked.

## Naming convention

`run_snow_mac.sh` defaults to `diskwork/bootdisk/macintosh/system.image`. Override with `BOOT_DISK=…`.

## Important: MOOF write-back asymmetry

snow saves any modified floppy as **MOOF**, even if you mounted a `.dc42` or raw `.img`. `rdedisktool` does not currently read MOOF. If you intend to keep the disk's modifications round-trippable through `rdedisktool`, **work on a copy** and treat snow as a one-way consumer of HFS / MFS images, not a writer back into them.
