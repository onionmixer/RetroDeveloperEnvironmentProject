# PLAN_rdedisktool

## 0. 목적, 범위, 완료조건

### 0.1 목적
- `RetroDeveloperEnvironmentDisktool`에서 bootdisk(`dos33`, `prodos`, `MSX-DOS`, `Human68k`) 쓰기 작업 시 부팅 실패를 유발하는 경로를 차단한다.
- 특히 `add/delete/mkdir/rmdir`가 boot 영역 또는 시스템 파일을 손상시키는 상황을 정책적으로 제어한다.

### 0.2 범위
- 포함
  - `rdedisktool`의 포맷/FS 감지, 파일 조작 명령, 저장 경로
  - bootdisk 자동 감지 + 수동 오버라이드
  - Apple II / MSX / X68000 디스켓 기반 테스트 루틴
- 제외
  - MSX ROM 시나리오
  - AppleWin/openMSX/px68k 에뮬레이터 바이너리 자체 수정

### 0.3 최종 DoD
- Apple II `prodos242.dsk`가 ProDOS로 안정 인식된다.
- bootdisk 보호 모드 기본값(`strict`)에서 위험 변이 명령 차단 + 안전한 `add` 허용이 동작한다.
- `info/list/extract/validate` 읽기성 명령 회귀가 없다.
- 저장은 임시파일 경유 + 교체 방식으로 동작한다.
- `diskwork/bootdisk`의 원본 이미지는 복사본 테스트로 보존된다.
- 4개 에뮬레이터 환경(Apple DOS33/Apple ProDOS/MSX-DOS2/Human68k) 부트 스모크가 재현 가능 형태로 정리된다.

---

## 1. 분석 대상 및 현황

### 1.1 코드 분석 대상(확인 완료)
- CLI/코어
  - `RetroDeveloperEnvironmentDisktool/src/cli/CLI.cpp`
  - `RetroDeveloperEnvironmentDisktool/include/rdedisktool/CLI.h`
  - `RetroDeveloperEnvironmentDisktool/src/core/FormatDetector.cpp`
  - `RetroDeveloperEnvironmentDisktool/src/core/DiskImageFactory.cpp`
- Apple II
  - `RetroDeveloperEnvironmentDisktool/src/apple/AppleDiskImage.cpp`
  - `RetroDeveloperEnvironmentDisktool/src/filesystem/apple/AppleDOS33Handler.cpp`
  - `RetroDeveloperEnvironmentDisktool/src/filesystem/apple/AppleProDOSHandler.cpp`
- MSX
  - `RetroDeveloperEnvironmentDisktool/src/filesystem/msx/MSXDOSHandler.cpp`
- X68000
  - `RetroDeveloperEnvironmentDisktool/src/filesystem/x68000/Human68kHandler.cpp`

### 1.2 bootdisk 이미지(확인 완료)
- `diskwork/bootdisk/AppleII/dos33.dsk`
- `diskwork/bootdisk/AppleII/prodos242.dsk`
- `diskwork/bootdisk/AppleII/ProDOS_2_4_3.po`
- `diskwork/bootdisk/msx/msxdos23.dsk`
- `diskwork/bootdisk/x68000/HUMAN302.XDF`

### 1.4 ProDOS 쓰기 가능 포맷 확인 결과
- `prodos242.dsk`
  - 포맷/FS: `Apple II DOS Order` + `ProDOS`
  - 결론: **포맷상 add 가능** (쓰기 가능), 단 원본은 `Free Space: 0 bytes`라 일반 크기 파일 추가는 실패
  - 확인: 복사본에서 공간 확보 후 소형 파일 add 성공 (`RDEDISK.TXT`)
- `ProDOS_2_4_3.po`
  - 포맷/FS: `Apple II ProDOS Order` + `ProDOS`
  - 결론: **포맷상 add 가능** 및 현재 여유공간으로 실제 add 성공 (`HELLO.NEW`)

### 1.3 에뮬레이터 실행 스크립트(확인 완료)
- Apple II
  - `run_applewin_dos33.sh`
  - `run_applewin_prodos.sh`
- MSX
  - `run_openmsx_msxdos2.sh`
- X68000
  - `run_px68k_humanos.sh`

---

## 2. 플랫폼별 "건드리면 안 되는" 영역

### 2.1 Apple DOS 3.3 (`dos33.dsk`)
- 금지 메타 영역
  - Track 0: boot loader
  - Track 17: VTOC + catalog
- 금지 파일(대표)
  - `INTBASIC`, `FPBASIC`, `MASTER`, `COPY*`, `FID`, `BOOT13`

### 2.2 Apple ProDOS (`prodos242.dsk`, `ProDOS_2_4_3.po`)
- 금지 메타 영역
  - Block 0..1: 부트 블록
  - Block 2..5: volume directory chain
  - Block 6: volume bitmap
- 금지 파일(대표)
  - `PRODOS`, `BASIC.SYSTEM`, `QUIT.SYSTEM`

### 2.3 MSX-DOS (`msxdos23.dsk`)
- 금지 메타 영역(BPB 기준)
  - sector 0: boot sector
  - FAT 영역 전체
  - root directory 영역 전체
  - 현재 기준 실측 보호구간: sector 0..13
- 금지 파일
  - `MSXDOS2.SYS`, `COMMAND2.COM`

### 2.4 X68000 Human68k (`HUMAN302.XDF`)
- 금지 메타 영역(BPB 기준)
  - logical sector 0: boot/IPL/BPB
  - FAT 영역
  - root directory 영역
  - 현재 기준 실측 보호구간: sector 0..10
- 금지 파일
  - `HUMAN.SYS`, `COMMAND.X`, `CONFIG.SYS`, `AUTOEXEC.BAT`, 핵심 `*.SYS`

### 2.5 최소 부팅 파일 삭제 시 yes/no 확인 정책
- 공통
  - bootdisk 복사본에서 `delete` 수행 시 대상이 최소 부팅 파일이면 yes/no 확인 프롬프트 필수
  - 기본값은 `No`(취소)
  - 비대화형 실행은 기본 차단하고, 별도 강제 옵션(`--force-system-file`, 가칭)로만 허용
- 플랫폼별 최소 부팅 파일
  - Apple DOS 3.3:
    - 엄격 최소(부트 코드 관점): 카탈로그 파일 기준 필수 파일 없음(부팅 코드는 트랙에 저장)
    - 운영 최소(본 프로젝트 bootdisk 관점, 권장 프롬프트 대상): `INTBASIC`, `FPBASIC`, `MASTER`, `BOOT13`
  - Apple ProDOS: `PRODOS` (필수), `BASIC.SYSTEM`(일반적인 BASIC 부팅 환경 필수)
  - MSX-DOS2: `MSXDOS2.SYS`, `COMMAND2.COM`
  - X68000 Human68k: `HUMAN.SYS`, `COMMAND.X`

### 2.6 최소 부팅 파일 근거/신뢰도
- Apple DOS 3.3 (High)
  - DOS 3.3는 파일이 아닌 트랙에 저장되며 카탈로그에 DOS 파일이 나타나지 않음.
- Apple ProDOS (High)
  - `PRODOS`가 없으면 부팅 오류.
  - BASIC 프롬프트 부팅에는 `BASIC.SYSTEM` 필요.
- MSX-DOS2 (Medium-High)
  - MSX-DOS는 커널(`MSXDOS.SYS`) + 커맨드 프로세서(`COMMAND.COM`) 구조.
  - 본 프로젝트 부트디스크는 DOS2 대응 파일명(`MSXDOS2.SYS`, `COMMAND2.COM`) 사용.
- Human68k (Medium)
  - 커맨드 프롬프트 부팅에 `HUMAN.SYS`, `COMMAND.X` 필수로 동작.
  - `CONFIG.SYS`, `AUTOEXEC.BAT`은 설정/자동 실행 단계.
  - 자료 출처(커뮤니티): https://gamesx.com/wiki/doku.php?id=x68000:human68k

### 2.7 외부 근거 링크(검토 완료)
- Apple II DOS/ProDOS FAQ (Csa2): https://gswv.apple2.org.za/a2zine/faqs/Csa2DOSMM.html
- Apple2 DOS/ProDOS FAQ 미러: https://www.apple2.org/faq/FAQ.dos.prodos.html
- DOS Mini-Manual (ProDOS boot 요구사항): https://gswv.apple2.org.za/a2zine/GS.WorldView/Resources/ARTICLES/DOS.MiniManual.html
- MSX-DOS/Nextor boot 파일 설명(실무 참고): https://www.konamiman.com/msx/msx-e.html

---

## 3. 핵심 원인 및 조치 상태

### 3.1 ProDOS `.dsk` 오인식
- 원인
  - DO order 이미지에서 ProDOS block2 검사에 인터리브 변환이 필요함.
- 조치 상태: 완료
  - `FormatDetector.cpp`에서 PO/DO 시그니처 구분 및 보강
  - `AppleDiskImage.cpp`에서 DO-order ProDOS block 접근 보정

### 3.2 bootdisk 보호 정책 부재
- 원인
  - mutate 명령에 사전 정책 체크 없음.
- 조치 상태: 완료
  - `BootDiskPolicy` 신규 도입
  - CLI에서 `add/delete/mkdir/rmdir` 전 가드 적용

### 3.3 저장 중 원본 손상 리스크
- 원인
  - 단일 save 경로, 실패 복구 불충분.
- 조치 상태: 완료
  - temp 저장 + 교체 + 옵션 백업(`--keep-backup`) 경로 반영

### 3.4 BPB fallback 기반 위험 쓰기
- 원인
  - BPB 파싱 실패 시 기본값으로 계속 진행 가능한 코드 경로 존재 가능.
- 조치 상태: 완료
  - `MSXDOSHandler::parseBPB()` / `Human68kHandler::parseBPB()`에서 fallback 제거, strict fail-fast 적용

---

## 4. 현재 구현 반영사항(2026-02-24 기준)

### 4.1 신규 정책 컴포넌트
- `RetroDeveloperEnvironmentDisktool/include/rdedisktool/BootDiskPolicy.h`
- `RetroDeveloperEnvironmentDisktool/src/core/BootDiskPolicy.cpp`

### 4.2 CLI 전역 옵션 추가
- `--bootdisk-mode strict|warn|off`
- `--force-bootdisk`
- `--bootdisk-profile dos33|prodos|msxdos|human68k|unknown`
- `--keep-backup`

### 4.3 info 출력 보강
- `info -v`에 `BootDisk/Profile/Confidence/ProtectionMode/Reason` 출력

### 4.4 저장 보호
- 저장 시 `*.tmp.rdedisktool` 경유
- `--keep-backup` 설정 시 `*.bak` 생성

---

## 5. 남은 보강 계획

### 5.1 BPB strict 모드 도입(필수)
- 대상
  - `RetroDeveloperEnvironmentDisktool/src/filesystem/msx/MSXDOSHandler.cpp`
  - `RetroDeveloperEnvironmentDisktool/src/filesystem/x68000/Human68kHandler.cpp`
- 목표
  - bootdisk 모드 `strict/warn`에서 BPB 불일치 시 mutate 차단
- 구현안
  - BPB 파싱 실패를 명시 오류로 승격
  - `BootDiskPolicy` reason에 `invalid_bpb` 계열 사유 연결
- 상태
  - BPB strict fail-fast는 적용 완료. (`initialize` 실패 경유로 변이 경로 차단)
  - `invalid_bpb_or_filesystem_init_failed` reason을 `info -v`에 노출하도록 반영 완료

### 5.2 시스템 파일 비강제 보호 레벨 분리(권장)
- 현재
  - `--force-bootdisk`로 override 가능
- 보강안
  - `--force-bootdisk`는 일반 파일 add만 허용
  - 핵심 시스템 파일 삭제/덮어쓰기는 별도 `--force-system-file` 없이는 불가

### 5.3 safe add 검증(적용 완료)
- bootdisk에서 `--force-bootdisk` 없이 `add` 실행 시:
  - 부트영역 섹터 스냅샷(프로필별)
  - 기존 파일(서브디렉터리 포함 재귀) 해시/크기 스냅샷
  - write 후 부트영역/기존파일 무결성 검증
  - 검증 통과 시에만 저장

### 5.3 문서 반영(필수)
- `RetroDeveloperEnvironmentDisktool/README.md`
- `RetroDeveloperEnvironmentDisktool/TESTCASE.md`
- 루트의 `FLOPPY_IMAGE_OPERATION.md`

---

## 6. 테스트 루틴(원본 보존 + 파일 추가 + 부트 검증)

### 6.1 공통 준비
```bash
cd RetroDeveloperEnvironmentDisktool
cmake -S . -B build_local -DCMAKE_BUILD_TYPE=Release
cmake --build build_local -j

mkdir -p /tmp/rdedisktool_guard
cp ../diskwork/bootdisk/AppleII/dos33.dsk /tmp/rdedisktool_guard/
cp ../diskwork/bootdisk/AppleII/prodos242.dsk /tmp/rdedisktool_guard/
cp ../diskwork/bootdisk/AppleII/ProDOS_2_4_3.po /tmp/rdedisktool_guard/
cp ../diskwork/bootdisk/msx/msxdos23.dsk /tmp/rdedisktool_guard/
cp ../diskwork/bootdisk/x68000/HUMAN302.XDF /tmp/rdedisktool_guard/
```

### 6.2 플랫폼별 추가 파일 선정(Examples 기반)
- Apple DOS 3.3
  - 소스 파일: `Examples/Tutorial_apple_01/HELLO.BIN`
  - 추가 대상: `HELLONEW`
  - 권장 옵션: `--type B --addr 0x0803`
- Apple ProDOS 2.4.3 (`ProDOS_2_4_3.po`, 추가 테스트용)
  - 소스 파일: `Examples/Tutorial_apple_prodos_01/HELLO`
  - 추가 대상: `HELLO.NEW`
  - 대상 이미지: `ProDOS_2_4_3.po` 복사본 (직접 add 가능)
- Apple ProDOS 2.4.2 (`prodos242.dsk`, 용량제약 테스트용)
  - 소스 파일: `Examples/Tutorial_apple_01/HELLO.BIN` (소형 파일)
  - 추가 대상: `HELLO42.BIN`
  - 대상 이미지: `prodos242.dsk` 복사본
  - 사전 단계: 복사본에서 비핵심 파일 1개 삭제로 최소 여유공간 확보 후 add
- MSX-DOS
  - 소스 파일: `Examples/Tutorial_msx_z88dk_01/HELLO.COM`
  - 추가 대상: `HELLO.COM`
- X68000 Human68k
  - 소스 파일: `Examples/Tutorial_x68000_01/hello.x`
  - 추가 대상: `HELLO.X`

### 6.3 이미지별 파일 추가 절차
```bash
# Apple DOS 3.3: safe-add
./build_local/rdedisktool --bootdisk-mode strict add \
  /tmp/rdedisktool_guard/dos33.dsk \
  ../Examples/Tutorial_apple_01/HELLO.BIN HELLONEW --type B --addr 0x0803

# Apple ProDOS 2.4.3: safe-add
./build_local/rdedisktool --bootdisk-mode strict add \
  /tmp/rdedisktool_guard/ProDOS_2_4_3.po \
  ../Examples/Tutorial_apple_prodos_01/HELLO HELLO.NEW

# Apple ProDOS 2.4.2: 용량 확인용(Free space 0으로 추가 실패 가능성 높음)
./build_local/rdedisktool info /tmp/rdedisktool_guard/prodos242.dsk -v
./build_local/rdedisktool --bootdisk-mode strict add \
  /tmp/rdedisktool_guard/prodos242.dsk \
  ../Examples/Tutorial_apple_prodos_01/HELLO HELLO.NEW
# 위 명령은 용량 부족으로 실패 가능성이 높음(관찰 로그 기록)

# Apple ProDOS 2.4.2: 쓰기 가능 포맷 검증(복사본에서 공간 확보 후 소형 파일 추가)
./build_local/rdedisktool --bootdisk-mode off delete \
  /tmp/rdedisktool_guard/prodos242.dsk MAKE.SMALL.P8
./build_local/rdedisktool --bootdisk-mode strict add \
  /tmp/rdedisktool_guard/prodos242.dsk \
  ../Examples/Tutorial_apple_01/HELLO.BIN HELLO42.BIN

# MSX-DOS2: safe-add
./build_local/rdedisktool --bootdisk-mode strict add \
  /tmp/rdedisktool_guard/msxdos23.dsk \
  ../Examples/Tutorial_msx_z88dk_01/HELLO.COM HELLO.COM

# X68000 Human68k: safe-add
./build_local/rdedisktool --bootdisk-mode strict add \
  /tmp/rdedisktool_guard/HUMAN302.XDF \
  ../Examples/Tutorial_x68000_01/hello.x HELLO.X
```

### 6.4 추가 후 무결성/존재 확인
```bash
./build_local/rdedisktool list /tmp/rdedisktool_guard/dos33.dsk | rg "HELLONEW"
./build_local/rdedisktool list /tmp/rdedisktool_guard/ProDOS_2_4_3.po | rg "HELLO.NEW"
./build_local/rdedisktool list /tmp/rdedisktool_guard/prodos242.dsk | rg "HELLO42.BIN"
./build_local/rdedisktool list /tmp/rdedisktool_guard/msxdos23.dsk | rg "HELLO.COM"
./build_local/rdedisktool list /tmp/rdedisktool_guard/HUMAN302.XDF | rg "HELLO.X"
```

### 6.5 에뮬레이터 부트 스모크
```bash
# Apple II DOS 3.3 (파일 추가본)
BOOT_DISK=/tmp/rdedisktool_guard/dos33.dsk ./run_applewin_dos33.sh

# Apple II ProDOS
BOOT_DISK=/tmp/rdedisktool_guard/prodos242.dsk ./run_applewin_prodos.sh

# Apple II ProDOS (파일 추가본)
BOOT_DISK=/tmp/rdedisktool_guard/ProDOS_2_4_3.po ./run_applewin_prodos.sh

# MSX-DOS2 (파일 추가본)
BOOT_DISK=/tmp/rdedisktool_guard/msxdos23.dsk ./run_openmsx_msxdos2.sh

# X68000 Human68k (파일 추가본)
BOOT_DISK=/tmp/rdedisktool_guard/HUMAN302.XDF ./run_px68k_humanos.sh
```

### 6.6 결과 판정 기준
- 부트 프롬프트 진입 성공
- 시스템 커맨드 실행 가능(예: 디렉터리 listing)
- 추가 파일이 부팅 후 환경에서 인식 가능(디렉터리 목록으로 확인)
- 에뮬레이터가 부트 직후 hang/reboot loop 없이 안정 동작
- 각 실행 로그는 `results/boot_smoke/` 하위에 날짜별로 저장

### 6.7 최소 부팅 파일 삭제 yes/no 테스트
```bash
# Apple ProDOS
./build_local/rdedisktool delete /tmp/rdedisktool_guard/ProDOS_2_4_3.po PRODOS

# MSX-DOS2
./build_local/rdedisktool delete /tmp/rdedisktool_guard/msxdos23.dsk COMMAND2.COM

# X68000 Human68k
./build_local/rdedisktool delete /tmp/rdedisktool_guard/HUMAN302.XDF HUMAN.SYS
```
- 기대 결과
  - 공통 yes/no 확인 프롬프트 출력
  - 기본 엔터(No) 시 삭제 취소
  - 강제 옵션 미사용 시 자동/비대화형 삭제 차단
- 구현 상태
  - `delete` 경로에 최소 부팅 파일 yes/no 프롬프트 적용 완료
  - `--force-system-file` 옵션으로 의도적 강제 삭제 지원 완료
  - 자동화: `tests/test_system_file_delete_prompt.sh` 추가 및 통합 스위트 포함

---

## 7. 실행 체크리스트(업데이트)

### 코드
- [x] ProDOS `.dsk` 인식 패치
- [x] BootDiskPolicy 추가
- [x] CLI 옵션/가드 적용
- [x] BPB strict 분기
- [x] save 트랜잭션 저장

### 테스트
- [x] 기본 인식 테스트(로컬 CLI)
- [x] strict 차단/force override 테스트(로컬 CLI)
- [x] invalid BPB 차단 테스트(자동화 스크립트)
- [x] boot smoke용 diskaddtest 스크립트 실행(Apple DOS33/Apple ProDOS/MSX/X68000) 4/4 통과
- [ ] boot smoke 테스트 결과 로그/스크린샷 정리(`results/boot_smoke/`)
- [ ] non-bootdisk 회귀 테스트 시나리오 자동화

### 문서
- [ ] README 반영
- [ ] TESTCASE 반영
- [ ] FLOPPY_IMAGE_OPERATION 반영

---

## 8. 다음 실행 순서
1. 4개 diskaddtest 성공 결과를 `results/boot_smoke/` 문서와 스크린샷으로 정리
2. `prodos242.dsk`는 \"기본 add 실패(용량)\"와 \"공간 확보 후 소형 add 성공(포맷 쓰기 가능)\"을 실행 로그로 분리 기록
3. non-bootdisk 회귀 테스트 시나리오 자동화
