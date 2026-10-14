# 로컬 월드 모니터

1. Python 3를 설치한 상태에서 변경된 게임을 빌드하고 실행한다. 모니터 서버는 자동으로 시작한다. 기본 실행과 튜토리얼 모두 같은 모니터를 사용한다. 별도의 모니터 모드 인자는 없다.
2. 브라우저에서 http://127.0.0.1:8765 를 연다. 수동 실행이 필요한 경우에만 아래 명령을 사용한다. `--data`에는 실제 실행 파일 옆 GameData의 경로를 넣는다.

```powershell
python server.py --data "C:\경로\GameData\monitor.json"
```

3. 별도 브라우저 창에서 http://127.0.0.1:8765 를 연다. 종료는 서버 터미널에서 Ctrl+C.

HTML 더블 클릭 대신 위 로컬 서버를 사용한다. 외부 패키지와 인터넷 연결은 필요 없다. 포트 충돌 시 `--port 8766`으로 변경한다.
게임은 1초 간격으로 스냅샷 파일을 원자적으로 교체하고 브라우저는 HTTP GET `/api/state`에서 JSON을 수신한다. 서버는 127.0.0.1에만 바인딩하며 게임 데이터를 변경하지 않는다.

## 데이터 규약 (schema_version 1)
- timestamp_ms: 게임에서 발행한 Unix 밀리초. 5초 이상 오래되면 갱신 중단 표시.
- focused: 게임 창 활성 여부. 브라우저로 전환하면 기존 게임 로직에 따라 시뮬레이션이 정지할 수 있다.
- scene: tutorial 또는 level_one. 현재 로드된 장면만 보고한다.
- counts: npc, mobs_alive, mobs_total(사망 포함), wildlife(튜토리얼 비전투 동물).
- villages: id, name, population, mood, rumors, alignment. 인구는 주민 NPC 수.
- npcs: id, name, village_id, state(idle/walking/talking), x, y, dialogue(기본 대사).

현재 게임에는 마을 분위기·소문·성향 시뮬레이션이 없어 null(미구현)이다. 기본 대사를 소문으로 간주하지 않는다. 첫 사냥터에는 주민 NPC와 마을이 없으므로 빈 목록이 정상이다. 향후 마을을 추가하면 villages 배열과 NPC village_id로 구분한다. NPC ID는 현재 장면의 배열 인덱스이며 영구 저장용 ID가 아니다.

빌드·실행은 수행하지 않았다. 수동 확인: 튜토리얼 주민 수 및 위치 갱신, 첫 사냥터 처치 후 생존 몹 수, 게임 종료 후 갱신 중단, 서버 중단 후 수신 실패 표시.

모든 장면은 동일한 JSON 규약을 사용한다. `scene_name`은 게임이 전달하는 표시 이름이며 브라우저는 장면별 분기 없이 렌더링한다. `creatures` 배열은 개체별 id/name/category/state/health/x/y 및 몹의 respawn_seconds를 제공한다. 미로드 장면의 전역 상태 저장소는 현재 게임에 없으므로 집계에 포함하지 않는다.

자동 시작: 빌드 시 server.py/index.html을 실행 파일 옆 Monitor에 복사한다. 게임이 시작한 서버는 게임 종료 시 함께 종료한다. 같은 데이터 경로의 기존 서버는 재사용하며 직접 종료하지 않는다. Python 3가 필요하다. 포트 충돌 상세는 GameData/monitor-server-error.txt를 확인한다. 브라우저는 주소를 직접 열어 사용한다.
