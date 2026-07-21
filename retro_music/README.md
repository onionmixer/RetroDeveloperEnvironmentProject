# retro_music — VGM → MSX/Apple IIe PSG 음악 파이프라인 작업 폴더

Ospaggi 제공 VGM(AY PSG) 음원을 DKFS_retro의 MSX/Apple IIe 프로젝트에서
AKG player보다 가볍게 재생하는 방법에 대한 검토·설계 작업 공간.

## 구성

- `docs/REVIEW_VGM_PSG_PIPELINE.md` — **검토 결과 문서 (여기부터 읽기)**.
  결론: 자작 VGM 변환기 대신 Arkos Tracker 3의 AKY 포맷(공식 6502/Z80
  플레이어) 전환을 권고. 실측 근거·리스크·단계별 로드맵 + 질의응답(§11:
  .aks 확보 시 즉시 사용 범위, VGM→.aks 역변환 불가 근거) 포함.
- `tools/vgm_ay_analyze.py` — AY VGM을 50Hz 프레임으로 재구성해 프레임당
  레지스터 변경 통계와 각 인코딩 후보의 데이터 크기를 실측.
- `tools/vgm2ym.py` — VGM→YM5 변환 브리지. AT3의 SongToFap 입력으로 검증
  완료. 분석·레퍼런스 오디오 용도(AT3는 YM을 곡으로 임포트하지 않음).

## 빠른 사용

```bash
# VGM 분석
python3 tools/vgm_ay_analyze.py ".../Let's roll!.vgm"

# VGM → YM5
python3 tools/vgm2ym.py input.vgm output.ym --rate 50
```

AT3 3.6 CLI(`SongToAky --sourceProfile 6502acme|z80` 등)는
https://www.julien-nevo.com/arkostracker/release/3.6/linux64/ 의 zip에 포함.
