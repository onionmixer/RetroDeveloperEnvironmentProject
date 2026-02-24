# Tutorial_msx_z88dk_rom_01

z88dk(`zcc`) 기반 MSX ROM 예제입니다.
소스는 `main.c` + `printf` 기준으로 구성됩니다.

## 개요

이 튜토리얼은 z88dk 네이티브 방식으로 ROM 카트리지 바이너리를 생성/검증합니다.

## 빌드 타깃

- `+msx -subtype=rom`
- MSX ROM(`AB` 헤더) 생성
- 출력 크기 16KB bank 단위 정렬

## 빠른 시작

```bash
cd Examples/Tutorial_msx_z88dk_rom_01
./compile.sh all
```

생성물:
- `build/HELLO_ROM_Z88DK.rom`
- `build/HELLO_ROM_Z88DK`
- `build/HELLO_ROM_Z88DK_BSS.bin`
- `build/HELLO_ROM_Z88DK_DATA.bin`

루트 복사본도 함께 생성됩니다.

## 검증 항목

- ROM 시그니처: `AB` (`0x41 0x42`)
- ROM 크기: 16KB 배수

## openMSX 실행

```bash
./run_openmsx_tutorial_msx_z88dk_rom_01.sh
```
