# VGM → MSX/Apple IIe PSG 음악 파이프라인 검토 및 설계

작성: 2026-07-19 (Claude 조사 + codex 교차검토 반영, §11 질의응답 추가)
대상 리소스: `/home/onion/Workspace/04_Game/Resource/89_OuterResource/Music/Ospaggi/`
관련 프로젝트: `DKFS_retro/prototype_20_AppleII`, `DKFS_retro/prototype_20_MSX_ROM_MSXDOS`

---

## 0. 결론 요약 (TL;DR)

1. **커스텀 VGM→어셈블리 변환기는 만들지 않는 것을 권고한다.** 실측 결과 VGM
   레지스터 로그 방식은 77초 곡 기준 27.7KB(비압축 delta+RLE)로, Apple II LC
   예산(~8KB)에 원천적으로 들어가지 않고 MSX에서도 낭비다. 이 접근이 도달하려는
   목표물(레지스터 스트림 + 초경량 플레이어)은 **Arkos Tracker 3의 AKY 포맷이
   이미 공식 구현**하고 있다 — 패턴 재사용 압축으로 데이터 ~3KB 수준, 공식
   6502(Apple II Mockingboard)/Z80(MSX) 플레이어 제공.
2. **원본은 VGM이 아니라 AT3 프로젝트(.aks)다.** 작곡가(Ospaggi)는 AT3로 작곡하며
   이미 AKG asm, AKM 데이터(Z80/6502 ACME/6502 MADS/68000), AY VGM/VGZ를 함께
   배포하고 있다. VGM은 AT3의 렌더 산출물일 뿐이므로, 변환기의 올바른 입력은
   `.aks`이고 변환기는 AT3가 공식 제공하는 CLI(`SongToAky` 등)로 이미 존재한다.
3. **CPU 부하 절감이 목표라면 AKG→AKY 전환이 유일하게 검증된 경로다.**
   단, 절감폭은 "디코드 비용"에 한정된다(레지스터 쓰기 비용은 동일). Apple II
   에서 이득이 가장 크고, MSX는 SFX 통합 문제 때문에 전환 이득 대비 리스크가
   커서 선택 사항이다(§6).
4. **OPL3 전용 곡(Caving, Field_3/4 상위 폴더 vgm)은 기계 변환이 불가능하다.**
   YMF262(18ch FM) → AY(3ch 구형파)는 편곡 문제다. AY 버전(.vgz + AKM asm)이
   이미 병행 배포되고 있으므로 AY 산출물을 받는 것이 정답이다.
5. 최악 fallback은 현행 AKG 유지 — 이미 동작하며, playerconfig 최적화로
   개선 여지도 남아 있다(§7 O-1).

---

## 1. 리소스 인벤토리 (실측)

`Ospaggi/` 폴더 분석 결과:

| 자산 | 칩 | 비고 |
|---|---|---|
| `Caving_OPL3.vgm`, `Field_3_OPL3.vgm`, `Field_4_OPL3.vgm` | YMF262(OPL3) | **AY 변환 불가(편곡 필요)** |
| `roll/MSX_APPLE2/Let's roll!.vgm` | **AY-3-8912 @1.79MHz** | 76.8s, 50Hz, 3,840프레임, 전곡 루프 |
| `roll/MSX_APPLE2/Let's roll!.asm` + `_playerconfig.asm` | — | **AT3 AKG 내보내기** (데이터 ~1,100B) |
| `hub/hub_Z80.asm`, `hub_6502_ACME.a`, `hub_6502_MADS.ASM`, `hub_68000.s` | — | **AT3 AKM(minimalist) 데이터** 내보내기, 4개 CPU 문법 (곡 "new world", 559B) |
| `fieldddd.zip` → FIELD3/FIELD4 | — | 각각 AY `.vgz` + AKM asm(Z80/6502×2/68000) + OPL2/OPL3/TANDY vgm + MIDI |

핵심 관찰: **작곡가는 AT3에서 CPU별 데이터 내보내기를 이미 수행하고 있다.**
즉 `.aks` 원본이 존재하며, "VGM을 변환"할 필요 없이 원하는 포맷을 요청하거나
`.aks`를 받아 우리 빌드에서 직접 뽑으면 된다.

playerconfig 확인 사항: 두 곡 모두 `PLY_CFG_NoSoftNoHard`/`PLY_CFG_SoftOnly`
계열만 사용 — **하드웨어 엔벌로프 미사용** (§8 R-1 호환성 리스크를 줄여 줌).

## 2. 현행 DKFS_retro 음악 구조 (비교 기준)

| 항목 | MSX | Apple IIe |
|---|---|---|
| 플레이어 | AT3 공식 Z80 AKG (MSXGL 생성판) | 자체 포팅 65C02 AKG 디코더 (`akg_player.s`) |
| 플레이어 크기 | **3,982B** (`PLAYER_CODE_PLAYER.bin`, $CB00 상주) | **2,146B** (LC bank2 $D400) + period table |
| 곡 데이터 | 329~1,103B/곡 (매퍼 세그/뱅크 상주) | 동일 곡, LC common $E000 영역(서비스 코드와 ~8KB 공유, 현행 3곡 1,765B) |
| 호출 | 60Hz ISR + Bresenham 5/6 (50Hz 보정) | 50Hz 보정 tick (music M2 구조) |
| SFX | **AKG 통합 SFX 사용** (AKG_MUSIC_CAVEATS.md 참조) | 음악만 (SFX 없음) |

## 3. 실측: VGM 레지스터 로그 방식의 비용

`retro_music/tools/vgm_ay_analyze.py`로 "Let's roll!" AY VGM을 50Hz 프레임으로
재구성해 측정:

```
3,840 프레임(76.8s), AY 쓰기 49,921회
프레임당 평균 5.21개 레지스터 변경, 무변경 프레임 0%
레지스터별 변경 횟수: R0 3800 / R2 3760 / R10 3415 … (톤·볼륨이 상시 변동)
고유 전체상태(14reg tuple): 1,608종

인코딩별 크기 (77초, 루프 1회분):
  raw 14B/frame        53,760 B
  변경마스크+값+RLE     27,700 B   ← "단순 변환기"의 현실적 하한
  (reg,val)쌍+구분자    43,880 B
  zlib(마스크스트림)     6,255 B   ← LZ 스트리밍 해제 필요(6502에서 비현실적)
  고유상태 사전+인덱스   30,192 B
```

판정: 이 곡은 아르페지오/피치 이펙트가 상시 도는 스타일이라 idle 프레임이
0%이고, 단순 delta 스트림은 **27.7KB/곡**이다. Apple II는 곡 3개에 8KB 예산이
전부이므로 불가. LZ를 붙이면 6.3KB까지 줄지만 6502에서 프레임당 스트리밍
해제 비용·윈도 버퍼·복잡도가 커져 "가볍게"라는 원래 목적과 모순된다.
**AKY가 정확히 이 문제(레지스터 스트림의 구조적 압축 + 초경량 플레이어)의
공식 해답이므로 자작 변환기는 열위 대안이다.**

## 4. AT3 플레이어 매트릭스 (공식 소스/배포판 확인)

AT3 3.6 배포판(`players/`)과 Bitbucket 저장소(`hardware/`) 직접 확인 결과:

| 플레이어 | CPU 지원 | 속도 | 크기 특성 | SFX | 비고 |
|---|---|---|---|---|---|
| AKG | **Z80만** | 중간 | 데이터 최소급 (기준) | 통합 지원 | 현행 사용 중. A2는 자체 포팅으로 해결한 상태 |
| AKM | **Z80만** | **AKG보다 느림** (CPC 최대 45 스캔라인) | 플레이어·데이터 모두 소형 | 지원 | 메모리 최적화용. **CPU 절감 목적에는 부적합** — 작곡가의 6502 AKM 내보내기는 데이터일 뿐, 공식 6502 AKM 플레이어는 없음 |
| **AKY** | **Z80 + 6502(Apple II/Oric) + 68000** | **빠름** (CPC 12 스캔라인 ≈ 프레임의 3.8%) | 데이터 ~2.6–3.2배 (실측 §5) | Z80 MultiPsg 변형만 | 레지스터 스트림 방식. 6502판은 GROUiK(French Touch) v0.10, Mockingboard slot4($C400) 기본 |
| FAP | Z80(CPC 전용) | 최고속(실측 620 NOPs) | 소형 + decrunch 버퍼 2,882B | 없음 | **YM 입력을 직접 수용**(우리 vgm2ym 검증에 사용). CPC 전용이라 채택 불가 |

CLI 자동화: 배포판 `tools/`의 `SongToAkg/SongToAkm/SongToAky`가
`--sourceProfile z80/6502acme/6502mads/68000`, `--exportPlayerConfig`,
`--labelPrefix`, 바이너리 출력(`-bin --encodingAddress`)을 지원 →
**`.aks`만 있으면 compile.sh에서 완전 자동화 가능.**

입력 포맷 제약: `SongToAky` 등은 AT3가 로드 가능한 곡 포맷(.aks, AT2,
Starkos, VT2, WYZ, Chipnsfx, MOD, MIDI)만 받는다. **VGM/YM은 곡 입력이 안
된다**(SongToFap만 YM 수용 — 레지스터 로그 계열이라 가능).

## 5. 동일 곡 데이터 크기 실측 (AT3 저장소 예제)

| 곡 | AKG(Z80) | AKM(Z80) | AKY(6502) | AKY/AKG 배율 |
|---|---|---|---|---|
| A Harmless Grenade | 1,362B | 1,043B | 3,563B | ×2.6 |
| Sarkboteur | — | — | 7,572B(6502)/7,742B(Z80) | (장곡) |
| Let's roll! (현행) | 1,100B | — | 미산출(.aks 필요) | 추정 ~2.9–3.5KB |
| new world (hub) | — | 559B | — | — |

Apple II 적용 추정: 현행 3곡 1,765B → AKY 시 **~5.3KB**. LC common 예산(서비스
코드·곡버퍼 공유 ~8KB)에 "수치상" 들어가지만, codex 지적대로 **플레이어 교체분·
정렬·로더 오버헤드를 포함한 실배치 검증이 선행돼야 한다**(§8 R-3).

## 6. PSG 쓰기 소유권 문제 (codex 핵심 지적)

AY는 mixer(R7)·noise(R6)·envelope(R11-13)가 **채널 간 공유 레지스터**라서,
음악 플레이어와 SFX 플레이어가 독립적으로 쓰면 last-writer-wins로 깨진다.
현행 MSX AKG는 플레이어 내부에서 SFX를 통합 중재하므로 이 문제가 없다.

| 플랫폼 | 현행 | AKY 전환 시 |
|---|---|---|
| Apple II | 음악만 → **충돌 없음. 전환 최적 대상** | 그대로 안전 |
| MSX | AKG 통합 SFX 사용 중 | (a) AKY MultiPsg + SFX 변형(공식, Z80) PoC 검증, (b) 단일 최종 쓰기 소유자(레지스터 섀도) 구현, (c) **AKG 유지(권고 기본값)** |

MSX는 Z80 3.58MHz라 AKG 부하 여력이 상대적으로 크므로, **1단계에서는 Apple II만
AKY로 전환하고 MSX는 AKG 유지**가 리스크 대비 최적이다. MSX 전환은 실측 후
별도 판단(§9 Phase 3).

## 7. 검토했으나 기각/보류한 대안

- **O-1. AKG playerconfig 공격적 최적화 (보류-병행 가능, 저위험):** 곡이 쓰지
  않는 기능(하드 엔벌로프 등)을 playerconfig로 제거하면 현행 플레이어에서
  코드·CPU를 일부 회수할 수 있다("최대 1.5KB + 수 스캔라인" — AT3 공식 문서).
  포맷 전환 없이 얻는 공짜 이득이므로 AKY PoC와 무관하게 시도 가치 있음.
- **O-2. 게임 디스어셈블 드라이버 계열 (기각):** MetalGear(Konami,
  `sound/bgmdriver.asm` 1,328라인)와 u4remasteredA2(MBSM, 패턴+소프트웨어 ADSR,
  `src/patchedgame/program/MBSM.s` 804라인)를 분석했다. 둘 다 "노트 드라이버"
  클래스로 데이터는 최소지만 **전용 작곡 툴체인이 없다** — VGM/AT3에서 이들
  포맷으로 가는 변환기를 만드는 것은 AT3 내보내기를 재발명하는 것과 같다.
  참고 가치: 저부하 ISR 구조, MBSM의 DOS 틈새 메모리 배치 기법.
- **O-3. 자작 VGM 레지스터 스트림 + 초소형 플레이어 (기각):** §3 실측으로 기각.
  유일한 우위 시나리오는 "AY VGM만 존재하고 .aks를 영영 못 받는 곡"인데, 현
  작곡가 워크플로상 발생하지 않는 상황이다.
- **O-4. VGM→YM→AT3 역임포트 (부분 기각):** `vgm2ym.py`(제작·검증 완료)로
  YM5는 만들 수 있으나 AT3는 YM을 곡으로 임포트하지 않는다(FAP 제외).
  YM→노트 역추출은 편곡 수준의 손실 변환이라 비권장. vgm2ym은 분석·FAP 실험·
  레퍼런스 오디오 생성용으로만 유지.

## 8. 리스크 목록 (codex 교차검토 반영)

- **R-1. 6502 AKY 플레이어 세대 차이 (게이트 리스크):** 6502판은 AT2 시절
  v0.10(2019)이고 AKY 포맷은 v1.1에서 하드웨어 엔벌로프 인코딩이 깨지는 변경이
  있었다. 현 곡들은 SoftOnly라 이론상 안전하지만, **AT3 버전 고정 + 전체 루프
  WAV 비교 회귀(레퍼런스 렌더 vs sa2/openMSX 캡처)** 를 통과해야 채택.
  AT3 3.6 배포판이 6502판을 동봉·예제 내보내기를 함께 제공하므로 정합 가능성은
  높다.
- **R-2. CPU 절감폭 과대 추정 금지:** 절감은 디코드 측에만 발생한다. Apple II
  IRQ 전체(진입·VIA ACK·레지스터 쓰기·복귀)를 기준으로 전/후 사이클을 실측할
  것. 6522 경유 AY 쓰기(레지스터당 수십 사이클 × ~11-14개)는 양쪽 공통 비용.
- **R-3. Apple II LC 배치:** ~5.3KB는 배율 추정치다. 실제 AKY 내보내기 3곡 +
  6502 플레이어(크기 미실측 — ACME 어셈블 필요) + 서비스 코드·트램폴린·ZP
  ($06-$09/$FA-$FD 기본, 현행 ZP 맵과 충돌 여부 확인)까지 실배치로 검증.
- **R-4. 프레임 스킵 안전성:** AKY는 호출 1회=논리 1프레임 소비이므로 스킵해도
  스트림이 깨지지 않고 "한 프레임 유지"로 나타난다(템포 지터만 발생). 현행
  50Hz 보정 구조(A2 50Hz tick, MSX Bresenham 5/6)와 호환.
- **R-5. PSG 클록 차이:** AKY 데이터는 **내보내기 시점의 PSG 주파수로 주기값이
  구워진다**(헤더에 psgFrequency). MSX 1,789,773Hz / Apple II Mockingboard
  1,022,727Hz로 **플랫폼별 별도 내보내기 필수** (.aks의 PSG 설정을 바꿔 2회
  내보내기 — 스크립트화 가능). 현행 A2 AKG 포팅이 자체 period table로 해결하던
  부분이 데이터 측으로 이동하는 것.
- **R-6. 통합 회귀:** 화면 전환 PSG cleanup(AKG_MUSIC_CAVEATS.md §3), MSX
  SFX 중첩, A2 scene-swap/LC 뱅크 복원, 디스크 I/O 중 타이머 정지 등 기존
  유의사항 전체를 AKY에서 재검증해야 한다.

## 9. 권고 로드맵

- **Phase 0 — 소스 확보(선행 조건):** 작곡가에게 `.aks` 원본(또는 곡별 AKY
  내보내기: MSX/Apple II 각 PSG 주파수 2종) 요청. OPL3 전용 곡은 AY 편곡판 요청.
- **Phase 1 — PoC (코드 통합 없이 검증):**
  1. AT3 3.6 CLI를 `retro_music/at3/`에 고정 설치(버전 고정, R-1).
  2. ACME 설치 → 6502 AKY 플레이어 + 예제 곡 어셈블 → **플레이어 실크기 측정**.
  3. sa2(Mockingboard WAV 캡처, 기존 검증법)와 openMSX로 재생 → AT3 레퍼런스
     WAV와 비교(회귀 스크립트화). 전체 IRQ 사이클 실측(전/후, R-2).
  4. Apple II 실배치 시뮬: 3곡 AKY 크기 + 플레이어를 현 LC 맵에 배치해 FIT 판정.
- **Phase 2 — Apple II 전환:** PoC 통과 시 `akg_player.s`(2,146B) →
  AKY 6502 플레이어 교체(Mockingboard 슬롯 상수·ZP·LC $D400 재배치, 50Hz tick
  유지). SFX 없음 → 소유권 문제 없음.
- **Phase 3 — MSX (선택):** 현행 AKG 유지가 기본. AKG 부하 실측 후 필요 시
  AKY MultiPsg+SFX 변형 PoC(단일 PSG 동작 및 SFX 중재 확인) 또는 레지스터 섀도
  소유자 설계를 거쳐 전환. O-1(playerconfig 최적화)은 즉시 적용 가능.

## 10. 산출물 및 도구

- `retro_music/tools/vgm_ay_analyze.py` — AY VGM 프레임 재구성·인코딩 비용 실측기
  (사용: `python3 vgm_ay_analyze.py <file.vgm>`)
- `retro_music/tools/vgm2ym.py` — VGM→YM5 브리지 (AT3 SongToFap 입력 검증 완료;
  분석·레퍼런스용. `.vgz`는 gzip 자동 해제)
- AT3 3.6: https://www.julien-nevo.com/arkostracker/release/3.6/linux64/
  (zip 안에 `tools/SongTo*` CLI와 `players/playerAky/sources/6502/apple2_oric/
  PlayerAKY_6502.a` 동봉)
- 참고 저장소: AT3 소스 https://bitbucket.org/JulienNevo/arkostracker3 ·
  MetalGear 디스어셈블 `resource/MSX/MetalGear/sound/` ·
  u4remasteredA2 `fixed` 브랜치 `src/patchedgame/program/MBSM.s`

---

## 11. 질의응답 (2026-07-19)

### Q1. `.aks`를 받으면 Apple IIe/MSX에서 바로 사용할 수 있도록 각각 변환해 쓸 수 있다는 뜻인가?

**변환 자체는 즉시 가능하다(CLI 실행 확인 완료).** AT3 3.6 동봉 CLI로
헤드리스 변환이 된다:

```bash
# Apple IIe용 AKY (ACME 문법 — 빌드 통합 시 ACME 추가 또는 문법 변환 필요)
tools/SongToAky --sourceProfile 6502acme --exportPlayerConfig song.aks song_a2.a
# MSX용 AKY
tools/SongToAky --sourceProfile z80 --exportPlayerConfig song.aks song_msx.asm
# 바이너리 직접 출력
tools/SongToAky -bin --encodingAddress 0xA000 song.aks song.bin
```

다만 "바로"의 범위를 두 단계로 나눠야 한다:

1. **현행 AKG 경로 = 진짜 즉시 사용 가능.** `.aks`에서 `SongToAkg`로 뽑으면
   지금 MSX/A2에 이미 들어 있는 플레이어가 그대로 재생한다(현행 곡 추가와
   동일한 절차). `.aks` 확보만으로 곡 파이프라인은 완성된다.
2. **AKY 저부하 경로 = Phase 1~2 통합 후 사용 가능.** 변환 데이터는 AKY
   플레이어가 필요하므로, Apple II는 자체 AKG 디코더($D400)→공식 6502 AKY
   플레이어 교체 + LC 배치 FIT 검증(§9)이 선행돼야 한다. MSX는 SFX 소유권
   문제(§6)로 AKG 유지가 기본.

추가 전제: **AKY는 플랫폼별 2회 내보내기 필수.** AKY 데이터에는 내보내기
시점의 PSG 클록으로 계산된 주기값이 구워지므로(§8 R-5), `.aks`의 PSG 주파수를
MSX=1,789,773Hz / Apple II Mockingboard=1,022,727Hz로 바꿔 각각 내보내야
음정이 맞는다. (AKG 경로에서는 A2 포팅 플레이어의 자체 period table이 처리해
이 문제가 없다.)

### Q2. 현재의 VGM을 Arkos Tracker에서 load한 다음 다시 `.aks`로 저장할 수 있나?

**불가능하다.** 두 가지 근거로 확인했다:

1. **실험**: AT3 3.6 CLI에 VGM을 직접 입력 → `Unknown format for the song!`
   거부. vgm2ym으로 만든 YM5도 동일하게 거부(YM은 SongToFap만 수용).
2. **바이너리 확인**: AT3 실행 파일의 임포터 심볼 목록 추출 결과 —
   `.aks`(AT1/2/3), Starkos, SoundTrakker 128, Chipnsfx, MOD, Vortex(VT2),
   WYZ, MIDI 뿐. **VGM/YM 곡 임포터는 존재하지 않는다.**

구조적 이유: `.aks`는 노트·패턴·인스트루먼트·이펙트라는 **작곡 구조**를 담고,
VGM은 매 프레임 AY 레지스터 쓰기의 **결과 로그**일 뿐이다. VGM→.aks는
"완성된 소리에서 악보·악기를 역추출"하는 손실 역변환으로, 주기→노트 추정,
볼륨 곡선→인스트루먼트 분리, 아르페지오/비브라토→이펙트 복원 휴리스틱이 전부
필요하며 결과물은 편집 불가능한 수준의 .aks가 되기 쉽다(AT3가 공식 지원하지
않는 이유). 방향은 반대로만 열려 있다: `.aks` → AKG/AKM/AKY/YM/VGM/WAV.

우회로(반자동): Ospaggi 폴더에는 곡마다 `.mid`(SC-55/MT-32)가 있고 AT3는
**MIDI 임포트**를 지원한다. 노트는 불러올 수 있으나 인스트루먼트/이펙트는
오지 않으므로 AT3 안에서 악기를 다시 입히는 수작업 편곡이 필요하다 —
"자동 변환"이 아니라 "재작업 출발점"이다. 따라서 실무 결론은 §9 Phase 0
그대로: **작곡가에게 `.aks` 원본을 요청하는 것이 유일하게 깨끗한 경로**다.
