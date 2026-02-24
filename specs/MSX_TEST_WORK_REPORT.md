# MSX 테스트 상세 작업 문서

## 1. 문서 목적
- 이 문서는 현재 저장소에서 진행된 MSX 관련 테스트 작업을 별도 파일로 정리한 상세 기록이다.
- 대상 범위는 Phase6/Gate3 빌드-검증 파이프라인, openMSX 실행 검증, sharksym 라이브러리 컴파일 검증이다.
- 기준 실행환경 스크립트는 `run_openmsx_msxdos2.sh`를 따른다.

## 2. 기준 실행환경
- 프로젝트 루트: `/mnt/USERS/onion/DATA_ORIGN/Workspace/05_RetroDeveloperEnvironmentProject`
- openMSX 실행 기준:
`Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx`
- openMSX 시스템 데이터:
`Emulator/openMSX/share`
- 부트 디스크:
`diskwork/bootdisk/msx/msxdos23.dsk`
- 기준 머신:
`Panasonic_FS-A1GT`
- 참고 스크립트:
`run_openmsx_msxdos2.sh`

## 3. 테스트 스크립트 맵
- Gate3 통합 테스트:
`tools/msx/test_phase6_gate3.sh`
- Gate3 예제 매트릭스:
`tools/msx/test_phase6_gate3_matrix.sh`
- Phase6 툴체인 통합 테스트:
`tools/msx/test_phase6_tools.sh`
- sharksym 라이브러리 컴파일 테스트:
`tools/msx/test_sharksym_lib_compile.sh`
- openMSX 제어 채널 스모크:
`tools/msx/openmsx_control_smoke.sh`
- openMSX Phase6 실행 스모크:
`tools/msx/openmsx_phase6_optim_smoke.sh`
- openMSX GUI 사전점검:
`tools/msx/openmsx_gui_precheck.sh`

## 4. 테스트별 검증 포인트

### 4.1 Gate3 (`tools/msx/test_phase6_gate3.sh`)
- `run_phase6_gate3_app2.sh`를 호출해 bank00/bank01/(조건부 bank02) 링크 및 병합까지 실행한다.
- `/tmp/test_phase6_gate3.log`에서 `^[gate3] warning:` 패턴이 1건이라도 나오면 실패 처리한다.
- 필수 산출물:
`bank00.com`, `bank01.com`, `bank00.map`, `bank01_final.map`, `comm.json`, `gate3_report.json`, `out/blcaller.as`, `out/blcaller.obj`, `*_app2_merged.bin`, `*_app2_merged.bin.tbl`
- `comm.json`에서 `_main` cross-bank 경로를 확인한다.
- MAP 파일에서 `_main`, `start`, (조건부) `_sub` 심볼 존재를 확인한다.
- `gate3_report.json`의 `tool=phase6_gate3_app2`, `status=pass`를 강제한다.

### 4.2 Gate3 매트릭스 (`tools/msx/test_phase6_gate3_matrix.sh`)
- 기본 예제:
`2HELLO 2ASSERT 2LMEM 2BGM 2HANGUL 2HANIME 2TETRIS`
- 추가 변형:
`2ASSERTN (EXAMPLE=2ASSERT, EXAMPLE_VARIANT=NDEBUG)`
- 각 예제를 순차 실행하며 개별 PASS를 확인한다.

### 4.3 Phase6 툴체인 통합 (`tools/msx/test_phase6_tools.sh`)
- `run_phase6_smoke.sh` 실행 후 주요 산출물 존재를 검증한다.
- `comm.json`, `blcaller.as`, `optim_graph.json`, `subset_lib_manifest.json`, `optim_link.map`의 의미 검증은 테스트 스크립트 내부 assert(검증 전용)로 수행한다.
- strict negative 케이스:
`Toolchain/MSX/HITECH_TOOLCHAIN/bin/BANKING/BLOPTIM --strict-unresolved`가 `RC=2`로 종료되는지 확인한다.
- 옵션 기능:
`ENABLE_OPENMSX_SMOKE=1`, `ENABLE_OPENMSX_GUI_PRECHECK=1`, `ENABLE_SHARKSYM_LIB_COMPILE_TEST=1`

### 4.4 sharksym 라이브러리 컴파일 (`tools/msx/test_sharksym_lib_compile.sh`)
- 대상 소스:
`BL.C BLCRT.C BLGCM.C BLGFN.C BLGRC.C BLGRP.C BLSND.C`
- include 소문자 alias 디렉터리 생성 후 CPP/P1/CGEN/OPTIM/ZAS 파이프라인을 수행한다.
- `BLSND.C`는 `.asm` 대신 `.as` 직접 assemble 경로를 우선 사용한다.
- `BLGRP/BLCRT`는 `-32768 -> 32768` 정규화 후 assemble 안정성을 확보한다.
- 결과 요약:
`/tmp/sharksym_lib_compile_test/summary.tsv`
- `BLEXTREF`로 `_bl_snd_bgm_*` 핵심 심볼 정의 여부를 검증한다.

### 4.5 openMSX 제어/실행 검증
- `tools/msx/openmsx_control_smoke.sh`
`openmsx -control stdio`가 timeout 상태(`rc=124`)로 실행 유지되면 스모크 통과로 간주한다.
- `tools/msx/openmsx_phase6_optim_smoke.sh`
입력 COM을 부트 디스크에 주입 후 openMSX 제어 실행한다.
- GT BIOS ROM 사전검사:
`fs-a1gt_firmware.rom` 미설치 시 `status=skip`, `reason=missing_machine_rom_precheck`.
- 보고서:
`/tmp/openmsx_phase6_optim/openmsx_phase6_optim_report.json`
- `tools/msx/openmsx_gui_precheck.sh`
바이너리/share/boot disk/ROM/display/runtime 조건을 점검해 JSON 보고서를 생성한다.

## 5. 실제 실행 기록 (2026-02-22 04:00:09 KST 기준)

### 5.1 실행 명령
```bash
./tools/msx/test_phase6_gate3_matrix.sh
./tools/msx/test_phase6_tools.sh
ENABLE_OPENMSX_GUI_PRECHECK=1 ENABLE_SHARKSYM_LIB_COMPILE_TEST=1 ./tools/msx/test_phase6_tools.sh
./tools/msx/openmsx_control_smoke.sh
./tools/msx/openmsx_phase6_optim_smoke.sh /tmp/sharksym_phase6_smoke/optim_link.com
```

### 5.2 결과 요약
- `test_phase6_gate3_matrix.sh`: PASS
- `test_phase6_tools.sh`: PASS
- `ENABLE_OPENMSX_GUI_PRECHECK=1 ENABLE_SHARKSYM_LIB_COMPILE_TEST=1 test_phase6_tools.sh`: PASS
- `openmsx_control_smoke.sh`: `rc=124` (timeout-running, 스모크 기준 통과)
- `openmsx_phase6_optim_smoke.sh`: SKIP (`missing_machine_rom_precheck`)
- GUI precheck 보고: SKIP (`missing_machine_rom`)

### 5.3 라이브러리 컴파일 요약값
- `summary.tsv` 7개 소스 모두 `obj=1` 확인.
- `optim` 컬럼에서 `124`가 일부 발생해도 fallback assemble 경로로 최종 OBJ 생성 성공.
- `BLSND` 핵심 심볼 검증 PASS.

## 6. 주요 산출물 경로
- Gate3 기본:
`/tmp/sharksym_phase6_gate3`
- Gate3 예제별:
`/tmp/sharksym_phase6_gate3_2ASSERT`,
`/tmp/sharksym_phase6_gate3_2LMEM`,
`/tmp/sharksym_phase6_gate3_2BGM`,
`/tmp/sharksym_phase6_gate3_2HANGUL`,
`/tmp/sharksym_phase6_gate3_2HANIME`,
`/tmp/sharksym_phase6_gate3_2TETRIS`,
`/tmp/sharksym_phase6_gate3_2ASSERTN`
- Phase6 스모크:
`/tmp/sharksym_phase6_smoke`
- 라이브러리 컴파일:
`/tmp/sharksym_lib_compile_test`
- openMSX phase6 보고:
`/tmp/openmsx_phase6_optim/openmsx_phase6_optim_report.json`

## 7. 실패/스킵 판단 기준
- FAIL:
필수 산출물 누락, 심볼 검증 실패, gate3 warning 검출, JSON report 상태 불일치.
- SKIP:
openMSX GT BIOS ROM 미설치, GUI display 환경 미설정 등 환경 의존 사유.
- PASS:
스크립트 종료코드와 내부 assert/report 조건을 모두 만족.

## 8. 재실행 권장 순서
```bash
# 1) Gate3 전체 예제 검증
./tools/msx/test_phase6_gate3_matrix.sh

# 2) Phase6 핵심 파이프라인 검증
./tools/msx/test_phase6_tools.sh

# 3) GUI 사전점검 + 라이브러리 컴파일 포함
ENABLE_OPENMSX_GUI_PRECHECK=1 ENABLE_SHARKSYM_LIB_COMPILE_TEST=1 ./tools/msx/test_phase6_tools.sh

# 4) openMSX 제어 채널 스모크
./tools/msx/openmsx_control_smoke.sh

# 5) openMSX Phase6 실행 스모크
./tools/msx/openmsx_phase6_optim_smoke.sh /tmp/sharksym_phase6_smoke/optim_link.com
```
