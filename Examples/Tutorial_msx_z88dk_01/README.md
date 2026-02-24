# Tutorial_msx_z88dk_01

MSX-DOS2용 z88dk(`zcc`) `main.c` + `printf` Hello World 튜토리얼입니다.

## 빠른 실행

```bash
cd Examples/Tutorial_msx_z88dk_01
./compile.sh all
```

생성물:
- `build/HELLO.COM`
- `build/Tutorial_msx_z88dk_01.dsk`
- `HELLO.COM`
- `Tutorial_msx_z88dk_01.dsk`

소스:
- `main.c` (`printf` 사용)

## 에뮬레이터 실행

```bash
./compile.sh run
```

또는 튜토리얼 전용 스크립트:

```bash
./run_openmsx_tutorial_msx_z88dk_01.sh
```

부팅 후:

```text
B:
DIR
HELLO
```
