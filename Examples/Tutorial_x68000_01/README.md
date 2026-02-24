# Tutorial_x68000_01

X68000 Human68k 기본 Hello World 튜토리얼입니다.

## 파일 구성

- `hello.c`: 예제 소스
- `compile.sh`: 빌드/검증 스크립트
- `build/hello.x`: 빌드 산출물

## 빠른 시작

```bash
cd Examples/Tutorial_x68000_01
./compile.sh all
```

성공 시 `run68` 출력에 아래 문구가 포함됩니다.

- `Hello, X68000 Tutorial!`

## GUI 에뮬레이터(px68k) 실행

```bash
./compile.sh run
```

스크립트가 `run_px68k_humanos.sh` 기반 수동 실행 절차를 안내합니다.
