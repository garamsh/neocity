# 생존 규칙

괴물 입속 탈출의 순수 C++ 진행 상태와 후속 기능의 소유 경계를 설명한다.

## 현재 결정

- `src/survival/`의 `survival` 정적 라이브러리는 C++ 표준 라이브러리에만 의존한다. 기존 `gameplay`, 실험, Godot와 연결하지 않는다. 공개 경계는 [`include/survival/simulation.hpp`](../../src/survival/include/survival/simulation.hpp)와 [`include/survival/world.hpp`](../../src/survival/include/survival/world.hpp)이고 내부 구현과 단위 테스트는 같은 라이브러리의 `src/`에 둔다.
- [`Simulation`](../../src/survival/include/survival/simulation.hpp)은 두 인물의 위치·탐색 체력·소지품, 일수, 단계, 공용 탐색 잔여 시간, 위산, 실행 시드를 소유한다. 외부에는 값 스냅샷과 명시적인 입장·귀환·피해·수집·시간 진행 명령을 제공한다. 거점은 시간제한 없는 작업 단계에 포함한다.
- 한 인물이라도 탐색 중일 때만 탐색 시간이 흐른다. 만료하면 둘 다 거점으로 돌아오고 작업 단계로 바뀐다. 인물 한 명이 체력을 소진하면 그 인물만 귀환하며, 일수·소지품·공용 잔여 시간·위산을 보존한다. 자발적 귀환·체력 소진·시간 만료 모두 귀환한 인물의 체력을 3칸으로 회복한다. 둘 다 거점에 있으면 작업 단계로 전환해 시간을 멈추는 것은 현재 기반의 임시 경계이며 최종 거점 시간 규칙을 확정하지 않는다.
- 남은 시간이 있으면 회복된 체력으로 재입장할 수 있다. 거점 귀환 시 3칸 회복은 [사용자 결정 기록](https://github.com/garamsh/neocity/issues/23#issuecomment-6544525723)에 따른다.
- 호출자가 양수 탐색 예산과 시드를 제공한다. 시간은 벽시계 대신 정수 마이크로초로 전달하므로 같은 명령 순서와 총 탐색 시간은 프레임 분할과 무관하게 같은 상태를 만든다. 음수 시간은 상태 변경 없이 거부한다.
- 현재 난수 소비는 없다. 시드는 스냅샷에 보존하는 실행 식별 입력이다. 일수는 첫날, 위산은 초기 상태로 유지된다. 탈출 단계는 공개 상태에 예약되어 있으나 전환 명령은 없다. 날짜 진행·난수 판정·요리·위산 상승식·월드와 진행의 실행부 연결·화면은 구현되지 않았다.
- 테스트는 [`src/simulation_test.cpp`](../../src/survival/src/simulation_test.cpp)와 [`src/world_test.cpp`](../../src/survival/src/world_test.cpp)에 있으며 같은 `survival_test`로 등록한다. 월드/진행을 직접 함께 구성한 귀환 검사는 라이브러리 통합 테스트이며 실제 화면 연결을 검증하지 않는다. Godot 검사는 기존 게임을 계속 검증한다.

### 월드 상태

- `World`는 NPC·지형·건물·사다리·입구·재료원을 식별자로 구분한다. 호출자가 각 대상의 정수 월드 영역, 손상·노출 임계, 재료원 소유 대상, 통로 수리 도구를 제공한다. 최종 맵이나 조정값을 내장하지 않는다. ID는 파괴와 노출 뒤에도 유지하며 값 스냅샷은 상태·통행·공급 가능 여부를 제공한다. 통행 가능 값은 지형과 통로에 적용되며 다른 종류의 충돌 판정은 뜻하지 않는다.
- 재료원은 자체가 온전하고, 소유 대상을 설정했다면 해당 NPC·건물·지형도 온전할 때 공급 가능하다. 아직 보이지 않는 소유 대상은 공급하지 않는다. 재료 획득·재고 소모·도구 소지 검증은 이 기능에 없으며 탐색이 해당 명령 경계를 소유한다. 수리는 파괴된 사다리·입구 중 설정된 도구를 요구하는 대상만 허용한다. 도구 소모·비용은 아직 정하지 않았다.
- 진행 입력은 실행 전체의 누적 탐색 시간과 현재 위산이다. 누적 시간 역행은 입력 전체를 거부하고, 같은 입력은 상태를 다시 전이시키지 않는다. 위산 하락은 현재 값만 바꾸며 역사상 최고 위산과 파괴·노출 이력을 되돌리지 않는다. 거점 귀환·체력 회복 명령은 월드를 소유하거나 초기화하지 않는다.
- 손상은 설정한 시간 또는 최고 위산 증가 임계 중 하나가 충족되면 발생한다. 처음에는 실행 시작을 기준으로 하며 미노출 상태에도 같은 누적 기준을 적용한다. 통로 수리 시 그 대상의 기준점만 현재 누적 시간·최고 위산으로 옮긴다. 같은 위험 입력은 수리를 취소하지 않고 추가 시간이나 새 최고 위산의 증가가 다시 임계에 도달하면 재파손한다. 이는 현재 최소 손상 모델이며 최종 부식·수리 밸런스가 아니다.
- 노출은 설정한 누적 시간 또는 최고 위산 임계를 한 번 넘으면 유지된다. 호출자 설정으로 건물이 파괴되고 다른 건물·통로·재료원이 나타나는 사례를 검증한다. 파편 물리·건물 간 연쇄 붕괴나 실제 새 화면은 구현하지 않았다.
- 실행부 연결 시 진행 전후의 탐색 잔여 시간 차이를 실행 전체 누적 시간에 더해 월드에 전달한다. 날짜가 바뀌거나 인물이 귀환해도 누적 시간이나 월드 인스턴스를 초기화하지 않는다. 현재 연결을 구현한 곳은 테스트이며 제품 연결은 #28의 범위다.

### 기능별 파일 소유 계획

다음 표의 월드 파일은 구현됐으며 탐색·거점·AI 전용 후속 파일은 아직 존재하지 않는다. 이 계획은 책임과 최종 대상 경로를 정하며 #24~#29의 Worker 실행 배정을 대신하지 않는다. 각 배정에서 한 파일의 쓰기 소유자는 한 명이다.

| 소유 작업 | 공개 경계 및 내부 구현 경로 | 읽기와 쓰기 방향 |
|---|---|---|
| 공통 진행 #23 및 후속 통합 담당 | `src/survival/include/survival/simulation.hpp`, `src/survival/src/simulation.cpp`, `src/survival/src/simulation_test.cpp`, `src/survival/CMakeLists.txt` | 공통 상태의 유일한 쓰기 소유자. 기능 결과를 명시적인 명령으로 반영한다. 공개 계약·빌드 등록 변경은 PM이 이 소유자에게 모아 배정한다. |
| 탐색 #24 | `src/survival/include/survival/exploration.hpp`, `src/survival/src/exploration.cpp`, `src/survival/src/exploration_test.cpp` | 인물 명령과 진행 스냅샷을 읽고 이동·충돌·스태미나·수집 가능 여부를 소유한다. 결과는 인물 위치 표현과 수집·피해 요청이다. 현재 공통 수집 명령은 재료 존재·거리·중복 획득을 검증하지 않으므로 이 기능이 검증한 결과만 전달한다. |
| 월드 변화 #25 | `src/survival/include/survival/world.hpp`, `src/survival/src/world.cpp`, `src/survival/src/world_test.cpp` | 누적 탐색 시간·현재 위산·수리 명령을 읽는다. NPC·지형·오브젝트와 통행·재료 공급 스냅샷을 소유하여 탐색에 제공한다. 일수별 초기화는 하지 않는다. 공통 상태를 직접 수정하지 않는다. |
| 거점 #26 | `src/survival/include/survival/workshop.hpp`, `src/survival/src/workshop.cpp`, `src/survival/src/workshop_test.cpp` | 소지품과 감정·조리·급식 명령을 읽고 도구·조리 결과·섭취 이력·점수를 소유한다. 재료 소비·위산·다음 날 전환 결과를 공통 진행 담당에게 전달한다. 이 결과를 반영할 공통 명령 추가는 #26 시작 전에 PM이 별도 소유자로 확정한다. |
| 동료 AI #27 | `src/survival/include/survival/companion.hpp`, `src/survival/src/companion.cpp`, `src/survival/src/companion_test.cpp`, `src/input/survival.hpp`, `src/input/survival.cpp` | 두 인물의 입력 변환도 이 소유자가 맡는다. 탐색·월드·거점의 읽기 전용 스냅샷과 조작 인물을 입력받아 사람과 같은 기능 명령을 만든다. 진행 상태를 별도로 소유하거나 강제로 갱신하지 않는다. |
| 화면 통합 #28 | `src/scene.cpp`, `src/scene.hpp`, `src/register_types.cpp`, `src/session/session.cpp`, `src/session/session.hpp`, `src/input/menu.cpp`, `src/input/menu.hpp`, `src/rendering/survival.cpp`, `src/rendering/survival.hpp`, `src/hud/survival.cpp`, `src/hud/survival.hpp`, `src/hud/menus.cpp`, `src/hud/menus.hpp`, `project/levels/monster-mouth-escape/survival.tscn`, `project/project.godot`, `project/tests/survival_timing.gd`, `project/e2e/survival.gd` 및 해당 `.uid`, 루트 `CMakeLists.txt`, `build_profile.json` | 기존 단일 조립 지점에서 각 구체 기능을 조립하고 입력·시간을 전달한다. 화면은 스냅샷을 읽으며 규칙을 재구현하지 않는다. #27의 두 인물 입력 변환을 사용하고 그 파일은 수정하지 않는다. `docs/architecture/session.md`·`presentation.md`·`survival.md`와 색인을 함께 갱신한다. 이 기존 경로는 #23에서 수정하지 않는다. |
| 시작·엔딩 아트 #29 | `project/levels/monster-mouth-escape/cinematics/intro/`, `project/levels/monster-mouth-escape/cinematics/ending/` | 아트 Worker가 자산과 자체 장면을 소유한다. #28은 로딩·전환을 소유한다. 장면 공개 연결은 두 배정에서 합의하고 같은 파일에 쓰지 않는다. |

- 기능은 같은 `survival` 경계 안의 구체 기능으로 추가하며 범용 이벤트 버스·공유 엔진·빈 기능 폴더를 만들지 않는다. 전체 기능을 `simulation.cpp`에 누적하지 않는다.
- 탐색은 월드의 읽기 전용 계약을 사용한다. 둘 사이에 필요한 계약 변경은 PM이 한 소유자에게 배정하며 상대 기능의 내부 상태를 포함하지 않는다.
- 탐색 시간 전달은 공통 진행에 한 번만 한다. 다른 기능이 읽을 탐색 경과량은 진행 전후 잔여 시간의 차이이므로 작업 시간과 만료 후 초과 시간을 포함하지 않는다.
- 공통 헤더·빌드 파일·아키텍처 문서는 각 기능 Worker가 동시에 수정하지 않는다. 후속 기능별 결과 타입과 새 공통 명령은 실제 기능 이슈에서 확정하고, 통합 담당에게 계약 변경을 모은 뒤 소비자를 연결한다.

## 근거

- [게임 기준](../design/monster-mouth-escape.md)은 사용자 결정과 임시 실행 동작·미정 사항을 구분한다. [부모 #21](https://github.com/garamsh/neocity/issues/21)과 [기반 #23](https://github.com/garamsh/neocity/issues/23)이 구현 범위를 정한다.
- 기존 [구조](structure.md)의 얕은 목적 폴더와 독립 규칙 경계를 적용한다. [ADR 0004](adr/0004-adopt-the-godot-cpp-template-layout.md)의 목적별 배치와 [ADR 0005](adr/0005-isolate-experiment-rules-behind-one-host.md)의 독립 규칙 경계를 바꾸지 않으므로 새 ADR은 추가하지 않는다.
- 서로 다른 탐색·거점 동작과 두 인물의 귀환 보존 규칙은 독립 생존 라이브러리의 책임이다. 기존 상승 게임이나 실험에 분기를 추가하지 않아 두 규칙을 각각 검증할 수 있다.

## 미정 사항

- 두 인물이 모두 잠시 귀환했을 때와 실제 조리 중의 구분 및 거점 체류 중 탐색 시계의 최종 동작. 현재는 둘 다 거점이면 시간제한 없는 작업 단계로 취급한다.
- 실제 탐색 예산, 위산 단위·상승식, 월드 손상·유입 임계와 수리 비용, 다음 날 전환, 인물의 상세 위치, 기능별 결과 타입. 테스트 입력은 밸런스 결정을 의미하지 않는다.
- 30일 이후 매일 탈출 기회를 판정할 정확한 시점과 확률. 실제 난수 기능이 생길 때 공통 시드에서 재현되는 난수 상태와 소비 순서의 소유자를 정하고 검증한다.
- 네트워크 모델과 저장·복원 형식. 현재 스냅샷은 읽기 전용 값 전달이며 저장 파일·복원·네트워크 프로토콜은 아니다.
