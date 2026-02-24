# Floppy Image Operation

## 글로벌 옵션

| 옵션 | 설명 |
|------|------|
| `-v, --verbose` | 상세 출력 활성화 |
| `-q, --quiet` | 불필요한 출력 억제 |
| `--bootdisk-mode <strict\|warn\|off>` | bootdisk 변경 보호 모드 (기본 `strict`) |
| `--force-bootdisk` | bootdisk 변경 정책을 의도적으로 우회 |
| `--force-system-file` | boot-critical 시스템 파일 삭제 시 확인 프롬프트 없이 강제 삭제 |
| `--bootdisk-profile <dos33\|prodos\|msxdos\|human68k\|unknown>` | bootdisk 프로파일 강제 지정 |
| `--keep-backup` | 이미지 저장 시 `.bak` 백업 보존 |
| `-h, --help` | 도움말 표시 |
| `-V, --version` | 버전 정보 표시 |

---

## 지원 포맷

### Apple II
| 포맷 | 확장자 | 설명 |
|------|--------|------|
| DOS Order | .do, .dsk | 표준 DOS 3.3 섹터 순서 |
| ProDOS Order | .po | ProDOS 섹터 순서 |
| Nibble | .nib | Raw nibblized 포맷 (6656 bytes/track) |
| WOZ | .woz | WOZ v1/v2 flux-level 포맷 |

### MSX
| 포맷 | 확장자 | 설명 |
|------|--------|------|
| DSK | .dsk | Raw 섹터 덤프 (720KB/360KB) |
| DMK | .dmk | IDAM 테이블 포함 DMK 포맷 |
| XSA | .xsa | XSA 압축 포맷 (LZ77 + Huffman, **읽기 전용**) |

> **참고**: XSA 포맷은 **읽기 전용**입니다. 파일 목록 조회 및 추출은 가능하지만, 파일 추가/삭제/수정은 불가능합니다. 수정이 필요한 경우 DSK/DMK로 변환 후 작업하세요.

### X68000
| 포맷 | 확장자 | 설명 |
|------|--------|------|
| XDF | .xdf | X68000 raw 섹터 덤프 (주로 1024 bytes/sector) |
| DIM | .dim | 헤더 포함 DIM 포맷 |

### 지원 파일 시스템

| 파일 시스템 | 플랫폼 | 서브디렉토리 | 비고 |
|-------------|--------|--------------|------|
| DOS 3.3 | Apple II | 미지원 | VTOC 기반 할당, 최대 140KB |
| ProDOS | Apple II | 지원 | 블록 기반 할당, 최대 32MB |
| MSX-DOS | MSX | 지원 | FAT12, MSX-DOS 1/2 호환 |
| Human68k | X68000 | 지원 | FAT12 기반, 8.3 파일명 |

---

## Apple II (AppleWin)

- AppleWin의 `source/CmdLine.cpp:128-303`에서 `--d1`/`--d2` 및 슬롯 지정(`-s5d1`, `-s6d1` 등) 커맨드라인 인자를 파싱해 지정된 플로피 이미지를 각 드라이브에 미리 장착함
- `help/CommandLine.html:14-48`에도 동일한 스위치가 문서화되어 있으며 `--d1` 입력 시 슬롯 6 드라이브 1에 장착되고 자동 전원이 켜지며, `--d2`는 드라이브 2에 해당
- 실행 예 (Linux): `sa2 --d1 ./diskwork/bootdisk/AppleII/dos33.dsk --d2 mydisk.do` → 지정한 이미지가 슬롯 6 드라이브 1, 2에 장착된 상태로 부팅됨 (`-s5d1` 등과 조합 가능)
- `--d1-disconnected`/`--d2-disconnected`로 각 드라이브를 비워 둔 채 시작할 수 있음
- 저장소 실행 스크립트:
  - `./run_applewin_dos33.sh`
  - `./run_applewin_prodos.sh`
  - 두 스크립트 모두 `APPLEWIN/BOOT_DISK/PROGRAM_DISK` 환경변수 오버라이드 + 자동 경로 탐색 지원

## openMSX

- `doc/manual/user.html:855-904`에 따르면 `openmsx relax.dsk`처럼 디스크 이미지 파일을 직접 인수로 주면 드라이브 A에 자동 장착되며, 확장자를 인식하지 못하면 `openmsx -diska relax.di` 형식을 사용해 드라이브별로 지정 가능
- 동일 문서에서 `-ips` 옵션을 이미지 뒤에 붙여 패치를 적용하고 `openmsx -diska .`로 호스트 디렉터리를 디스크로 사용하는 방법도 설명됨
- `src/fdc/DiskImageCLI.cc:18-65`의 `DiskImageCLI` 구현이 `-diska`/`-diskb` 옵션과 `.dsk`, `.di1`, `.dmk`, `.xsa` 등 확장자를 파서에 등록해 커맨드라인 인자 해석 시 자동으로 해당 드라이브 명령(`diska`/`diskb`)을 호출하도록 처리함
- 저장소 실행 스크립트:
  - `./run_openmsx_msxdos2.sh`
  - `OPENMSX/OPENMSX_SHARE/BOOT_DISK/MACHINE` 오버라이드 + 자동 경로 탐색 지원

> **XSA 사용 시 주의**: openMSX는 XSA 파일을 직접 읽을 수 있지만, XSA는 압축 포맷이므로 에뮬레이터 내에서 디스크 쓰기가 필요한 경우 DSK로 변환 후 사용하세요.

## X68000 (px68k-onionmixer)

- 저장소 실행 스크립트:
  - `./run_px68k_humanos.sh`
- 기본 부팅 경로(자동 탐색 후보):
  - `./diskwork/bootdisk/x68000/HUMAN302.XDF`
  - `./Emulator/x68000/work.xdf`
- 환경변수 오버라이드:
  - `PX68K`, `IPL_ROM`, `CG_ROM`, `BOOT_DISK`, `FDD1_DISK`

---

## rdedisktool 워크플로우

`rdedisktool`을 사용하여 디스크 이미지를 준비하고 에뮬레이터에 장착하는 워크플로우입니다.

### 이미지 정보 확인
```bash
rdedisktool info game.dsk
rdedisktool info game.dsk -v   # Verbose 모드 - FAT/클러스터 정보 포함
rdedisktool list game.dsk
rdedisktool list game.dsk GAMES   # 서브디렉토리 목록 조회
```

MSX-DOS 디스크의 Verbose 출력 예시:
```
Cluster Information:
  Total Clusters:    713
  Used Clusters:     2
  Free Clusters:     711
  Cluster Size:      1024 bytes

FAT Cluster Map:
  Cluster 0: 0xFF9 (Media descriptor)
  Cluster 1: 0xFFF (Reserved)
  Cluster   2: EOF (0xFF8)
  Cluster   3: -> 4
  Cluster   4: FREE
  ...
```

`info -v`에서는 bootdisk 보호 진단도 함께 확인할 수 있습니다.
- `BootDisk`, `Profile`, `Confidence`, `ProtectionMode`, `Reason`
- BPB/메타데이터가 비정상이라 핸들러 초기화에 실패하면 `Reason: invalid_bpb_or_filesystem_init_failed`로 표시됩니다.

### XSA 압축/해제 후 에뮬레이터 장착
```bash
# XSA를 DSK로 변환 후 openMSX에 장착
rdedisktool convert game.xsa game.dsk -f msxdsk
openmsx -diska game.dsk

# XSA를 DMK로 변환
rdedisktool convert game.xsa game.dmk -f dmk

# DSK를 XSA로 압축 (배포/보관용)
rdedisktool convert game.dsk game.xsa

# DMK를 XSA로 압축
rdedisktool convert game.dmk game.xsa
```

**일반적인 압축 결과:**
| 원본 | 압축 후 | 압축률 |
|------|---------|--------|
| 720KB DSK | ~8KB XSA | ~99% |
| 360KB DSK | ~4KB XSA | ~99% |
| 1MB DMK | ~9KB XSA | ~99% |

> **참고**: 압축률은 디스크 내용에 따라 다릅니다. 빈 공간이나 반복 데이터가 많을수록 압축률이 높습니다.

### 새 디스크 이미지 생성

```bash
# Apple II DOS 3.3 디스크 생성
rdedisktool create disk.do -f do --fs dos33

# Apple II ProDOS 디스크 생성 (볼륨명 지정)
rdedisktool create game.po -f po --fs prodos -n MYGAME

# MSX-DOS 디스크 생성 (볼륨명 지정)
rdedisktool create msx.dsk -f msxdsk --fs msxdos -n MSXDISK

# 커스텀 geometry로 생성 (트랙:사이드:섹터:바이트)
rdedisktool create custom.do -f do -g 40:1:16:256

# 빈 디스크 생성 (파일 시스템 없음)
rdedisktool create blank.po -f po

# 기존 파일 덮어쓰기 (--force 옵션)
rdedisktool create msx.dsk -f msxdsk --fs msxdos --force
```

**지원 포맷:**

| 플랫폼 | 포맷 코드 |
|--------|----------|
| Apple II | `do`, `po`, `nib`, `nb2`, `woz`, `woz1`, `woz2` |
| MSX | `msxdsk`, `dmk` |
| X68000 | `xdf`, `dim` |

> **참고**: 생성된 디스크는 부팅 불가능합니다 (부트 코드 미포함).
> **참고**: 도구 버전에 따라 `create --help` 출력 예시에는 X68000 생성 옵션이 누락되어 보일 수 있으나, 실제 바이너리에서 `xdf/dim` 생성이 지원될 수 있습니다. 항상 현재 바이너리로 `info/list/validate`까지 확인하세요.

### 디스크 이미지 수정 후 에뮬레이터 장착
```bash
# MSX: 파일 추가 후 openMSX에 장착
rdedisktool add game.dsk ./patch.bin PATCH.BIN
openmsx -diska game.dsk

# target_name 생략 시 호스트 파일명 사용
rdedisktool add game.dsk ./PATCH.BIN

# 기존 파일 덮어쓰기 (-f, --force 옵션)
rdedisktool add -f game.dsk ./updated.bin PATCH.BIN

# 서브디렉토리에 파일 추가
rdedisktool add game.dsk ./save.dat GAMES/SAVE.DAT

# Apple II: 파일 추가 후 AppleWin에 장착
rdedisktool add appleii.dsk ./newprog.bin NEWPROG
sa2 --d1 appleii.dsk
```

### bootdisk 보호 모드(권장)
기본값은 `strict`입니다.
- `delete/mkdir/rmdir`는 bootdisk에서 차단됩니다.
- `add`는 safe-add 검증(부트영역 보존 + 기존 파일 보존)을 통과할 때만 허용됩니다.
- boot-critical 시스템 파일 삭제 요청 시 yes/no 프롬프트가 표시됩니다.
  - `--force-system-file` 사용 시 프롬프트 없이 즉시 삭제됩니다(추가 확인 없음).

```bash
# 기본 strict 차단
rdedisktool add ./diskwork/bootdisk/msx/msxdos23.dsk ./PATCH.BIN PATCH.BIN

# 정책 확인
rdedisktool info ./diskwork/bootdisk/msx/msxdos23.dsk -v

# 의도적 override (주의해서 사용)
rdedisktool --force-bootdisk add ./diskwork/bootdisk/msx/msxdos23.dsk ./PATCH.BIN PATCH.BIN
```

전역 옵션:
- `--bootdisk-mode strict|warn|off`
- `--force-bootdisk`
- `--force-system-file`
- `--bootdisk-profile dos33|prodos|msxdos|human68k|unknown`
- `--keep-backup` (저장 시 `.bak` 유지)

### Bootdisk Disk-Add Smoke 테스트 (실환경)

아래 스크립트는 공통적으로 "bootdisk 복사본 생성 -> rdedisktool add -> 에뮬레이터 부팅" 흐름을 수행합니다.

```bash
./run_applewin_dos33_diskaddtest.sh
./run_applewin_prodos_diskaddtest.sh
./run_openmsx_msxdos2_diskaddtest.sh
./run_px68k_humanos_diskaddtest.sh
```

운영 기준:
- 테스트는 단일 드라이브만 사용하여 bootdisk 파일 제어 영향만 검증합니다.
- DOS 3.3 diskaddtest는 복사 bootdisk에서 비필수 파일 삭제 후 add/boot를 검증합니다.
- 최근 기준 결과: 4/4 통과.

### DOS 3.3 바이너리 파일 추가 (로드 주소 지정)

DOS 3.3 바이너리 파일은 `BRUN` 명령으로 실행할 때 로드 주소가 필요합니다. `--type`과 `--addr` 옵션으로 지정할 수 있습니다.

```bash
# 바이너리 파일 ($0803 로드 주소 - 일반 프로그램)
rdedisktool add disk.do ./HELLO.BIN HELLO --type B --addr 0x0803

# 바이너리 파일 ($4000 로드 주소 - Hi-Res 그래픽)
rdedisktool add disk.do ./PICTURE.BIN MYPIC -t B -a $4000

# 바이너리 파일 ($6000 로드 주소)
rdedisktool add disk.do ./GAME.BIN GAME --type B --addr 0x6000

# Applesoft BASIC 프로그램
rdedisktool add disk.do ./HELLO.BAS HELLO --type A

# 텍스트 파일
rdedisktool add disk.do ./README.TXT README --type T
```

**DOS 3.3 파일 타입:**
| 타입 | 코드 | 설명 |
|------|------|------|
| T | 0x00 | 텍스트 파일 |
| I | 0x01 | Integer BASIC 프로그램 |
| A | 0x02 | Applesoft BASIC 프로그램 |
| B | 0x04 | 바이너리 파일 (기계어) |
| S | 0x08 | S-type 파일 |
| R | 0x10 | 재배치 가능 객체 코드 |

> **참고**: **ProDOS** 디스크에 파일을 추가할 때, DOS 3.3 파일 타입 코드는 자동으로 ProDOS 등가물로 변환됩니다:
> | DOS 3.3 | ProDOS | ProDOS 코드 |
> |---------|--------|-------------|
> | T (0x00) | TXT | 0x04 |
> | I (0x01) | INT | 0xFA |
> | A (0x02) | BAS | 0xFC |
> | B (0x04) | BIN | 0x06 |
> | R (0x10) | REL | 0xFE |

**일반적인 로드 주소:**
| 주소 | 용도 |
|------|------|
| $0801 | Applesoft BASIC 프로그램 |
| $0803 | 바이너리 프로그램 (BASIC stub 이후) |
| $2000 | Hi-Res 그래픽 페이지 1 |
| $4000 | Hi-Res 그래픽 페이지 2 |
| $6000 | 일반 프로그램 영역 |
| $9600 | RWTS 버퍼 영역 |

> **참고**: `--addr` 옵션 사용 시 4바이트 헤더(로드 주소 + 길이)가 자동으로 추가됩니다. 파일에 이미 유효한 DOS 3.3 헤더가 있으면 중복 추가되지 않습니다.

### 포맷 변환
```bash
# Apple II: DO ↔ PO 변환
rdedisktool convert game.do game.po -f po

# MSX: DSK ↔ DMK 변환
rdedisktool convert game.dsk game.dmk -f dmk

# X68000: XDF ↔ DIM 변환
rdedisktool convert game.xdf game.dim -f dim
rdedisktool convert game.dim game.xdf -f xdf
```

**지원되는 포맷 변환:**
| From | To | 비고 |
|------|-----|------|
| DSK | XSA | ~99% 압축 |
| DMK | XSA | ~99% 압축 |
| XSA | DSK | Raw로 압축 해제 |
| XSA | DMK | DMK로 압축 해제 |
| DSK | DMK | 섹터 → DMK |
| DMK | DSK | DMK → 섹터 |
| DO | PO | Apple II 순서 변환 |
| PO | DO | Apple II 순서 변환 |
| XDF | DIM | X68000 포맷 변환 |
| DIM | XDF | X68000 포맷 변환 |

### 디렉토리 관리 (ProDOS, MSX-DOS, Human68k)

> **참고**: DOS 3.3은 서브디렉토리를 지원하지 않습니다.

```bash
# 디렉토리 생성
rdedisktool mkdir mydisk.dsk GAMES
rdedisktool mkdir mydisk.dsk GAMES/RPG

# 디렉토리 삭제 (비어 있어야 함)
rdedisktool rmdir mydisk.dsk GAMES/RPG
```

### 디스크 검증
```bash
# 디스크 이미지 무결성 검증
rdedisktool validate mydisk.dsk
```

검증 항목:
- 디스크 이미지 구조 무결성
- 파일 시스템 메타데이터 일관성
- 섹터 할당 비트맵 검증

### 섹터 덤프
```bash
# Apple II 디스크 섹터 덤프
rdedisktool dump disk.do -t 17 -s 0

# MSX 디스크 섹터 덤프 (사이드 1)
rdedisktool dump disk.dsk --track 0 --sector 0 --side 1
```

**삭제된 파일 마커:**

`dump`로 디렉토리 섹터를 검사할 때 다음 마커 바이트가 삭제된 파일을 나타냅니다:

| 파일 시스템 | 마커 | 위치 | 설명 |
|-------------|------|------|------|
| MSX-DOS/FAT12 | `0xE5` | 파일명 첫 바이트 | 삭제된 파일 엔트리 |
| DOS 3.3 | `0xFF` | T/S 리스트 트랙 필드 | 삭제된 카탈로그 엔트리 |
| ProDOS | `0x00` | 스토리지 타입 니블 | 삭제된 엔트리 |
| Human68k | `0xE5` | 파일명 첫 바이트 | 삭제된 파일 엔트리 |

> **참고**: 이러한 마커는 정상이며, 디스크 공간은 재사용 가능합니다.

---

## 서브디렉토리 작업 예제

### MSX-DOS 서브디렉토리
```bash
# 새 MSX-DOS 디스크 생성
rdedisktool create mydisk.dmk -f dmk --fs msxdos

# 디렉토리 구조 생성
rdedisktool mkdir mydisk.dmk GAMES
rdedisktool mkdir mydisk.dmk GAMES/RPG
rdedisktool mkdir mydisk.dmk GAMES/ACTION

# 서브디렉토리에 파일 추가
rdedisktool add mydisk.dmk ./dragon.com GAMES/RPG/DRAGON.COM
rdedisktool add mydisk.dmk ./shooter.com GAMES/ACTION/SHOOTER.COM

# 서브디렉토리 목록 조회
rdedisktool list mydisk.dmk GAMES
rdedisktool list mydisk.dmk GAMES/RPG

# 서브디렉토리에서 파일 추출
rdedisktool extract mydisk.dmk GAMES/RPG/DRAGON.COM ./dragon_backup.com

# 서브디렉토리에서 파일 삭제
rdedisktool delete mydisk.dmk GAMES/ACTION/SHOOTER.COM

# 빈 디렉토리 삭제
rdedisktool rmdir mydisk.dmk GAMES/ACTION
```

### ProDOS 서브디렉토리
```bash
# 새 ProDOS 디스크 생성
rdedisktool create mydisk.po -f po --fs prodos -n MYDISK

# 디렉토리 구조 생성
rdedisktool mkdir mydisk.po DOCS
rdedisktool mkdir mydisk.po DOCS/MANUAL

# 서브디렉토리에 파일 추가
rdedisktool add mydisk.po ./readme.txt DOCS/README.TXT
rdedisktool add mydisk.po ./chapter1.txt DOCS/MANUAL/CHAPTER1.TXT

# 서브디렉토리 목록 조회
rdedisktool list mydisk.po DOCS
rdedisktool list mydisk.po DOCS/MANUAL

# 파일 추출 및 삭제
rdedisktool extract mydisk.po DOCS/MANUAL/CHAPTER1.TXT
rdedisktool delete mydisk.po DOCS/MANUAL/CHAPTER1.TXT
rdedisktool rmdir mydisk.po DOCS/MANUAL
```
