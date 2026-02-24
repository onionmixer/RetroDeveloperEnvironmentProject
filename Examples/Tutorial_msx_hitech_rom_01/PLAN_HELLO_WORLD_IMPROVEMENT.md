# PLAN: Tutorial_msx_hitech_rom_01 Hello World Recovery

## 0. Final Proof Consolidation

본 문서는 `Tutorial_msx_hitech_rom_01`의 최종 기준 계획서입니다.

최종 목적:
- Hi-Tech C로 작성한 C 프로그램을 MSX ROM으로 변환하고,
- openMSX에서 `Hello World` 표시까지 반복 가능하게 재현한다.

최종 완료 기준(DoD):
1. 신규 사용자가 예제 README만으로 `build -> run -> Hello World 확인` 재현 가능
2. 증명 경로와 디버그 경로가 명확히 분리
   - 기본(증명): `main-pure` + 안정 대기
   - 디버그: `DEBUG_BORDER=1` + 색상 전환
3. 문서 간 모순 없음
   - `Examples/Tutorial_msx_hitech_rom_01/README.md`
   - `DEVELOPER_MSX_HOWTO.md`
   - 루트 `README.md`
4. 실행 검증 명령이 실제로 수행 가능

## 1. Goal

`Examples/Tutorial_msx_hitech_rom_01`의 최종 목표는
Hi-Tech C C 프로그램을 ROM으로 변환해 openMSX에서 `Hello World`를 재현 가능한 절차로 증명하는 것이다.

완료 기준:
- openMSX에서 BIOS 이후 화면에 `Hello World`가 눈으로 확인된다.
- `default-verified` / `fallback-verified` 둘 다 실행 가능 상태 유지.
- 기존 안정 부팅 경로(`AB init=0x4106`, `--no-loader`)는 깨지지 않는다.

## 2. Constraints

- Hi-Tech C 파이프라인(`cpp_new3/p1x3/cgen3/...`)의 불안정성(특히 최적화/파서)을 고려한다.
- 현재 프로젝트의 안정 기준선은 `ROM_ENTRY_MODE=main-pure` (default), `loop` (fallback)이다.
- 먼저 "출력 성공"을 달성하고, 그 다음 C 친화성(`main()` 중심 구현)을 확대한다.

## 3. Strategy

### Phase A: 출력 최소 경로 확보 (ROM 엔트리 주도, 히스토리)

상태: COMPLETED (2026-02-23)

참고:
- 본 Phase는 초기 안정화/관찰 단계 기록이다.
- 최종 운영 기준은 `main-pure` 증명 경로(Phase B 이후)이다.

목표:
- C 런타임 의존을 최소화하고 BIOS 문자 출력 루틴(`CHPUT`)으로 `Hello World`를 먼저 출력.

작업:
1. `compile.sh`의 ROM 엔트리(ASM)에서 고정 문자열 출력 루틴 추가
2. 출력 후 현재처럼 보더 루프 진입(관찰 가능성 유지)
3. `default` / `fallback` 둘 다 실행 가능 유지

테스트:
```bash
cd Examples/Tutorial_msx_hitech_rom_01
./test_openmsx_step.sh default-verified
./test_openmsx_step.sh fallback-verified
```

완료 기준:
- 화면에 `Hello World`가 출력되고, 이후 안정 대기 상태를 유지한다.
- 보더 변화 관찰은 선택 디버그 모드(`DEBUG_BORDER=1`)에서 가능하다.

실제 결과:
- `default(main-pure)`: `Hello World` 출력 확인, 화면 안정 유지
- `fallback(loop)`: `Hello World` 출력 확인, 화면 안정 유지
- 보더 변화는 `DEBUG_BORDER=1` 디버그 모드로 분리

---

### Phase B: main-pure 경로로 출력 책임 이동

상태: COMPLETED (2026-02-23)

목표:
- "문자 출력"을 ROM 엔트리가 아니라 `MAIN.C`의 `main()` 경로에서 수행.

작업:
1. `MAIN.C`에 BIOS `CHPUT` 기반 `puts` 수준 출력 루틴 구현(인라인 ASM 또는 얇은 래퍼)
2. `MAIN_PURE` 모드에서 `main()`이 문자열 출력 + 상태 유지 루프 수행
3. ROM 엔트리는 `_main` 호출 + 안전 fallback만 수행

테스트:
```bash
cd Examples/Tutorial_msx_hitech_rom_01
./compile.sh verify-main-pure
./test_openmsx_step.sh default-verified
```

완료 기준:
- `main()` 경로에서 `Hello World` 출력이 재현되고,
- `verify-main-pure` 통과 상태 유지.

실제 결과:
- `main-pure` 경로에서 `Hello World` 출력 확인
- `verify-main-pure` 지속 통과

---

### Phase C: fallback(loop) 경로 동일 출력 보장

상태: COMPLETED (2026-02-23)

목표:
- `fallback`에서도 최소 동일 UX(`Hello World` + 동작 확인)가 되도록 정렬.

작업:
1. fallback 엔트리에 최소 텍스트 출력 경로 추가
2. default/fallback 출력 결과 차이를 문서에 명시

테스트:
```bash
cd Examples/Tutorial_msx_hitech_rom_01
./test_openmsx_step.sh fallback-verified
./test_openmsx_all_steps.sh
```

완료 기준:
- default/fallback 모두 `Hello World` 출력 확인 가능.

실제 결과:
- `default(main-pure)`: `Hello World` 출력 확인
- `fallback(loop)`: `Hello World` 출력 확인

---

### Phase D: 문서/운영 명령 고정

상태: COMPLETED (2026-02-23)

목표:
- 현재 목표(Hello World 출력) 기준으로 문서/스크립트 정합 완료.

작업:
1. `README.md` 실행 절차를 "Hello World 확인" 중심으로 갱신
2. `DEVELOPER_MSX_HOWTO.md` 결과 섹션 최신화
3. 관찰 체크리스트 추가:
   - 출력 문자열
   - 커서 위치/줄바꿈
   - 이후 상태(루프/대기)

테스트:
```bash
cd Examples/Tutorial_msx_hitech_rom_01
./test_openmsx_all_steps.sh
```

완료 기준:
- 신규 사용자도 문서만 보고 `Hello World` 확인까지 재현 가능.

실제 결과:
- `README.md`에 실행 절차/디버그 모드/관찰 체크리스트 반영
- `DEVELOPER_MSX_HOWTO.md`에 최신 실측 및 체크리스트 반영
- `default-verified`, `fallback-verified`, `DEBUG_BORDER=1` 사용자 실측 결과 정리 완료

최종 보강(2026-02-23):
- `fallback-verified`는 실제 정적 검증(`verify-fallback`)을 수행하도록 강화됨
- 사전 조건(Hi-Tech C/openMSX/ROM/DISPLAY) 문서화 완료
- openMSX foreground 실측으로 `default-verified`, `fallback-verified` 재확인 완료

## 4. Risk & Mitigation

리스크 1: Hi-Tech C 파서/최적화가 특정 C 문법에서 실패
- 대응: C를 최소화하고 출력 핵심은 ASM 래퍼로 분리
- 대응: `optim3` 경로 실패 시 기존 fallback 어셈블 경로 유지

리스크 2: BIOS 호출 타이밍/화면 모드 차이로 문자열 미표시
- 대응: 출력 전 화면 모드/커서 초기화 루틴 최소 추가
- 대응: 보더 변화 유지로 코드 생존 여부 즉시 판단

리스크 3: default/fallback 행동 불일치
- 대응: 두 경로 모두 동일 출력 루틴을 호출하도록 통합

## 5. Implementation Order (Strict)

1. Phase A
2. Phase B
3. Phase C
4. Phase D

각 Phase 완료 전 다음 Phase로 넘어가지 않는다.

## 6. Final Verification Scenarios

시나리오 A: 기본 증명 경로
```bash
cd Examples/Tutorial_msx_hitech_rom_01
./compile.sh verify-main-pure
DISPLAY=:1 OPENMSX_SYSTEM_DATA="$HOME/.openMSX/share" OPENMSX_DISABLE_SDL_JOYSTICK=1 SDL_AUDIODRIVER=dummy \
../../Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx \
-machine Panasonic_FS-A1GT \
-carta ./build/HELLO48_NONMAPPER.rom \
-romtype normal
```

시나리오 B: fallback 회귀 경로
```bash
cd Examples/Tutorial_msx_hitech_rom_01
./compile.sh verify-fallback
DISPLAY=:1 OPENMSX_SYSTEM_DATA="$HOME/.openMSX/share" OPENMSX_DISABLE_SDL_JOYSTICK=1 SDL_AUDIODRIVER=dummy \
../../Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx \
-machine Panasonic_FS-A1GT \
-carta ./build/HELLO48_NONMAPPER.rom \
-romtype normal
```

시나리오 C: 디버그 시각 진단
```bash
cd Examples/Tutorial_msx_hitech_rom_01
DEBUG_BORDER=1 ./compile.sh all
DISPLAY=:1 OPENMSX_SYSTEM_DATA="$HOME/.openMSX/share" OPENMSX_DISABLE_SDL_JOYSTICK=1 SDL_AUDIODRIVER=dummy \
../../Emulator/openMSX/derived/x86_64-linux-opt/bin/openmsx \
-machine Panasonic_FS-A1GT \
-carta ./build/HELLO48_NONMAPPER.rom \
-romtype normal
```
