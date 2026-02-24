# DEVELOPER COMMON HOWTO

이 문서는 Apple II, MSX, X68000 개발에 공통으로 적용되는 작업 원칙과 운영 절차를 정리합니다.
플랫폼별 상세 컴파일/실행 방법은 전용 문서를 사용합니다.

- Apple II: `DEVELOPER_AppleII_HOWTO.md`
- MSX: `DEVELOPER_MSX_HOWTO.md`
- X68000: `DEVELOPER_X68000_HOWTO.md`

---

## 1. 문서 범위

이 문서는 아래 항목만 다룹니다.

- 공통 사전 점검 (도구/경로/실행 파일 존재 확인)
- 공통 빌드 순서
- `rdedisktool` 기반 디스크 이미지 작업 표준 흐름
- 루트 실행 스크립트 사용 규칙
- `rdemonitor` 기반 디버그 모니터링
- 공통 트러블슈팅

플랫폼별 코드 예제, 플랫폼 전용 메모리 맵, 플랫폼별 런타임 커맨드는 각 전용 HOWTO에 위임합니다.

---

## 2. 공통 디렉터리/아티팩트

공통 원칙(중요):
- MSX 컴파일/빌드/패킹은 C 네이티브 도구 경로를 기본으로 사용합니다.
- Python 스크립트는 fallback 또는 검증(parity/report) 용도로만 사용합니다.

### 2.1 루트 주요 경로

- 디스크 도구: `RetroDeveloperEnvironmentDisktool/`
- 모니터: `RetroDeveloperEnvironmentMonitor/`
- 에뮬레이터: `Emulator/`
- 툴체인: `Toolchain/`
- 라이브러리/외부 자산: `Library/`
- 예제: `Examples/`
- 부팅 디스크: `diskwork/bootdisk/`
- 스펙 문서: `specs/`

부팅 디스크 하위 경로:
- Apple II: `diskwork/bootdisk/AppleII/`
- MSX: `diskwork/bootdisk/msx/`
- X68000: `diskwork/bootdisk/x68000/`

Apple II 라이브러리 참고:
- `Library/AppleII/apple2flat` (실험적 cc65 프레임워크)
- `Library/AppleII/souless_apple2` (Apple II 프로젝트 자산/소스)

플랫폼별 라이브러리 참고:
- `Library/MSX/ubox-msx-lib-1.2.0`
- `Library/x68000/X68KTutorials`

### 2.2 대표 튜토리얼 (최신)

- Apple II:
  - `Examples/Tutorial_apple_dos33_01`
  - `Examples/Tutorial_apple_prodos_01`
- MSX:
  - `Examples/Tutorial_msx_z88dk_01` (z88dk MSX-DOS2)
  - `Examples/Tutorial_msx_hitech_01` (Hi-Tech C MSX-DOS2)
  - `Examples/Tutorial_msx_hitech_rom_01` (Hi-Tech C ROM, native 변환)
  - `Examples/Tutorial_msx_hitech_rom_02` (Hi-Tech C ROM, BLMKRULE 기반, openMSX Hello World 검증 완료)
  - `Examples/Tutorial_msx_z88dk_rom_01` (z88dk ROM)
- X68000:
  - `Examples/Tutorial_x68000_01` (run68 자동 검증)

### 2.2-a MSX 도구 경로(재배치 반영)

- BANKING 실행 엔트리:
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMKRULE`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLEXTREF`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLCOMM`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLCALLER`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLOPTIM`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMERGE`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLLIST`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/bltoolc`
- PACKING 실행 엔트리:
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/PACKING/msx_hitech_nonmapper_pack.py`
  - `Toolchain/MSX/HITECH_TOOLCHAIN/bin/PACKING/gen_min_ab16_rom.py`
  - 기본 C 패킹 경로: `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/bltoolc pack-nonmapper`
- MSX 자동화 스크립트:
  - `tools/msx/*.sh`

### 2.3 핵심 실행 파일

- `RetroDeveloperEnvironmentDisktool/build/rdedisktool`
- `RetroDeveloperEnvironmentMonitor/rdemonitor`
- `Emulator/AppleWin/build/sa2`
- `Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx`
- `Emulator/x68000/px68k-onionmixer/px68k-onionmixer`

### 2.3-a MSX ROM mode(요약)

- ROM mode(`app-mode=0`, `rom-mode=1|2`)의 Linux 뱅킹툴/패커 상세는 `DEVELOPER_MSX_HOWTO.md`의 ROM 모드 섹션을 참고합니다.
- 권장 예제 경로:
  - `Examples/Tutorial_msx_hitech_rom_02`
- 핵심 체크:
  - BL_ROM 시그니처(`ROM `), 카트리지 헤더(`AB`), `__Lbss` RAM 배치, openMSX 예제별 화면 출력 확인

### 2.4 루트 실행 스크립트

- `run_applewin_dos33.sh`
- `run_applewin_prodos.sh`
- `run_openmsx_msxdos2.sh`
- `run_openmsx_rom.sh`
- `run_px68k_humanos.sh`
- `run_applewin_dos33_diskaddtest.sh`
- `run_applewin_prodos_diskaddtest.sh`
- `run_openmsx_msxdos2_diskaddtest.sh`
- `run_px68k_humanos_diskaddtest.sh`

---

## 3. 공통 사전 점검 체크리스트

작업 시작 전에 아래를 확인합니다.

```bash
# 컴파일러/툴체인
cl65 --version
/opt/z88dk/bin/zcc --version

# 프로젝트 도구
./RetroDeveloperEnvironmentDisktool/build/rdedisktool --version
./RetroDeveloperEnvironmentMonitor/rdemonitor --help

# 에뮬레이터
./Emulator/AppleWin/build/sa2 --help
./Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx --version
./Emulator/x68000/px68k-onionmixer/px68k-onionmixer --help

# 런처 스크립트 문법
bash -n ./run_applewin_dos33.sh
bash -n ./run_applewin_prodos.sh
bash -n ./run_openmsx_msxdos2.sh
bash -n ./run_openmsx_rom.sh
bash -n ./run_px68k_humanos.sh
bash -n ./run_applewin_dos33_diskaddtest.sh
bash -n ./run_applewin_prodos_diskaddtest.sh
bash -n ./run_openmsx_msxdos2_diskaddtest.sh
bash -n ./run_px68k_humanos_diskaddtest.sh
```

권장: 위 검증 결과를 작업 로그에 기록하고, 누락 항목은 즉시 보완 후 개발을 진행합니다.

---

## 4. 공통 빌드 순서

플랫폼별 세부 빌드보다 먼저 공통 인프라를 안정화합니다.

1. `rdedisktool` 빌드
2. `rdemonitor` 빌드
3. 대상 에뮬레이터 빌드 (AppleWin/openMSX/px68k)
4. 대상 플랫폼 코드 빌드
5. 디스크 이미지 반영
6. 에뮬레이터 실행 + 동작 확인

예시:

```bash
# 1) rdedisktool
cd RetroDeveloperEnvironmentDisktool
mkdir -p build && cd build
cmake ..
make -j"$(nproc)"

# 2) rdemonitor
cd ../../RetroDeveloperEnvironmentMonitor
make
```

---

## 5. 디스크 이미지 작업 표준 절차

### 5.1 기본 원칙

- 원본 부팅 디스크는 수정하지 않습니다.
- 개발용 프로그램 디스크를 별도로 생성/유지합니다.
- 변경 전 `info`, 변경 후 `list`/`validate`를 실행합니다.

### 5.2 공통 명령 패턴

```bash
# 1) 현재 이미지 상태 확인
./RetroDeveloperEnvironmentDisktool/build/rdedisktool info <image>
./RetroDeveloperEnvironmentDisktool/build/rdedisktool list <image>

# 2) 파일 추가/교체
./RetroDeveloperEnvironmentDisktool/build/rdedisktool add <image> <host_file> <target_name>

# 3) 무결성 점검
./RetroDeveloperEnvironmentDisktool/build/rdedisktool validate <image>
```

### 5.3 포맷별 참고

- Apple II: `do`, `po`, `nib`, `woz`
- MSX: `msxdsk`, `dmk`, `xsa(read-only)`
- X68000: `xdf`, `dim`

세부 포맷/변환/파일시스템 제약은 `FLOPPY_IMAGE_OPERATION.md`를 기준으로 합니다.

---

## 6. 실행 스크립트 운용 규칙

### 6.1 공통 규칙

- 직접 에뮬레이터 커맨드를 입력하기보다 루트 스크립트를 우선 사용합니다.
- 스크립트가 자동 탐색한 경로를 신뢰하되, CI/재현 테스트에서는 환경변수로 고정합니다.

### 6.2 환경변수 오버라이드

- AppleWin DOS33/ProDOS: `APPLEWIN`, `BOOT_DISK`, `PROGRAM_DISK`
- openMSX MSX-DOS2: `OPENMSX`, `OPENMSX_SHARE`, `BOOT_DISK`, `MACHINE`
- px68k Human68k: `PX68K`, `IPL_ROM`, `CG_ROM`, `BOOT_DISK`, `FDD1_DISK`

### 6.3 실행 예시

```bash
# Apple II DOS 3.3
./run_applewin_dos33.sh

# Apple II ProDOS
./run_applewin_prodos.sh

# MSX-DOS2
./run_openmsx_msxdos2.sh

# X68000 Human68k
./run_px68k_humanos.sh
```

---

## 7. 공통 디버그 모니터링

`rdemonitor`는 AppleWin/openMSX 디버그 스트림을 모니터링합니다.

```bash
# 기본 실행
./RetroDeveloperEnvironmentMonitor/rdemonitor

# 디버그 포트 지정
./RetroDeveloperEnvironmentMonitor/rdemonitor --debug_port=65505

# 전체 로그 저장
./RetroDeveloperEnvironmentMonitor/rdemonitor --log_all=true
```

출력 스펙:
- `specs/RetroDeveloperEnvironmentProject_OUTPUT_SPEC_V01.md`

권장 운영:
- 재현이 어려운 이슈는 `--log_all=true`로 로그를 남기고, 문제 발생 시점의 입력/스크립트/디스크 상태를 같이 기록합니다.

---

## 8. 공통 품질 게이트

릴리즈/공유 전 최소 통과 기준:

1. 빌드 성공 (도구/타겟 모두)
2. 디스크 반영 성공 (`add`, `list`, `validate`)
3. 에뮬레이터 부팅 성공
4. 대상 프로그램 최소 1회 실행 성공
5. 필요 시 디버그 로그 확보

권장 추가 기준:

- 동일 절차 재실행 시 동일 결과 재현
- 실행 스크립트 환경변수 오버라이드 경로에서도 동일 동작
- 튜토리얼 산출물 시그니처/헤더 검증
  - MSX z88dk ROM: `AB` 시그니처
  - MSX Hi-Tech ROM: `ROM ` 시그니처
- bootdisk 보호 회귀 검증
  - `cd RetroDeveloperEnvironmentDisktool && tests/test_bootdisk_guard_all.sh`
  - `cd RetroDeveloperEnvironmentDisktool && tests/test_system_file_delete_prompt.sh`
- 실환경 disk-add smoke 4종 통과
  - `./run_applewin_dos33_diskaddtest.sh`
  - `./run_applewin_prodos_diskaddtest.sh`
  - `./run_openmsx_msxdos2_diskaddtest.sh`
  - `./run_px68k_humanos_diskaddtest.sh`

---

## 9. 공통 트러블슈팅

### 9.1 `rdedisktool` 관련

- `Not enough space`:
  - 이미지 포맷/파일시스템 상태를 `info`로 확인
  - 불필요 파일 삭제 또는 새 작업 디스크 생성
- `Unknown filesystem`:
  - 초기화되지 않은 이미지 가능성
  - 포맷 생성 방식과 대상 에뮬레이터 호환성 재확인

### 9.2 에뮬레이터 실행 실패

- 바이너리 미존재/권한 오류:
  - 각 실행 파일 경로와 실행 권한 확인
- ROM/부팅 디스크 누락:
  - 스크립트 출력의 resolved path 기준으로 파일 존재 확인

### 9.3 MSX/openMSX 특이사항

- 실제 MSX-DOS 부팅은 머신/ROM 조건이 맞아야 동작합니다.
- 조이스틱 초기화 충돌 환경에서는 런처 기본값(`OPENMSX_DISABLE_SDL_JOYSTICK=1`) 유지 권장

### 9.4 X68000/px68k 특이사항

- `iplrom.dat`, `cgrom.dat`, `diskwork/bootdisk/x68000/HUMAN302.XDF` 누락 시 부팅 불가
- `FDD1_DISK`를 사용할 경우 파일 존재를 먼저 검증

---

## 10. 권장 문서 사용 순서

1. `REQUIREMENTS.md`
2. `DEVELOPER_COMMON_HOWTO.md` (현재 문서)
3. 대상 플랫폼 HOWTO
   - `DEVELOPER_AppleII_HOWTO.md`
   - `DEVELOPER_MSX_HOWTO.md`
   - `DEVELOPER_X68000_HOWTO.md`
4. 디스크 작업 상세
   - `FLOPPY_IMAGE_OPERATION.md`
5. 프로토콜/검증 스펙
   - `specs/RetroDeveloperEnvironmentProject_OUTPUT_SPEC_V01.md`

---

## 11. 변경 메모

- 기존 문서에 포함된 Apple II/MSX/X68000 상세 튜토리얼은 각 전용 HOWTO로 역할을 분리했습니다.
- 본 문서는 공통 운영 가이드로 축소/정렬하고, 실제 저장소 경로 기준으로 최신화했습니다.

---

## 12. MSX generated Makefile 검증(공통 게이트)

MSX Hi-Tech C 경로에서 CFG 기반 generated `.MK` 파이프라인 회귀는 아래 스크립트로 수행합니다.

```bash
./tools/msx/test_blmkrule_mk_pipeline.sh
```

이 테스트는 `Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLMKRULE`로 생성한 `.MK`의 `build/cleanup` 동작을 검증하며,
app-mode2(`2HELLO`)는 Gate3 경로 연동 여부까지 확인합니다.
