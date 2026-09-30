# 과제 1. 정렬 비교 — 퀵 · 병합 · Timsort

2026-2 **고급알고리즘**(SIT2001-01) 과제 1. 배운 정렬 둘(퀵 · 병합)과 배우지
않은 정렬 하나(Timsort)를 하나의 인터페이스로 구현하고 같은 잣대로 비교했습니다.
실습 환경 template(`lec-algorithm/hw1-sample-2026`)에서 시작했습니다.

- **보고서: [report/REPORT.md](report/REPORT.md)**
- 강의 자료: [lec-algorithm.github.io/lecture](https://lec-algorithm.github.io/lecture/)
- 강의 예제 코드: [lec-algorithm/algorithm-code](https://github.com/lec-algorithm/algorithm-code)
- 시각화 자료: [lec-algorithm/algorithm-viz](https://github.com/lec-algorithm/algorithm-viz)

## 언제 쓰나

이 저장소는 **새 저장소의 출발점**입니다. 상단의 **Use this template**을 눌러
자기 계정에 사본을 만들고 거기서 작업하세요.

- **과제**를 낼 때
- **개인프로젝트**를 시작할 때 (수업계획서상 GitHub 저장소 제출이 필수입니다)
- 알고리즘 코드를 돌려 볼 환경이 필요할 때

수업에서 다루는 예제 코드는 여기가 아니라 `algorithm-code`에 있습니다.
그쪽은 매주 새 주제가 추가되므로, 복사하지 말고 저장소에서 바로 Codespace를
만들거나 클론해서 `git pull`로 받으세요.

## 준비물

**GitHub 계정 하나면 됩니다.** 로컬에서 돌리려면 Git과 Docker가 필요합니다.
컴파일러와 Python은 컨테이너 이미지 안에 들어 있어 따로 설치하지 않습니다.

## 시작하기 (권장): Codespaces

1. 이 저장소 상단의 **Use this template** → **Create a new repository**
2. 저장소 이름을 정합니다 (예: `algorithms-hw1`, `my-algorithm-project`)
3. 만들어진 **내 저장소**에서 **Code** → **Codespaces** 탭
4. **Create codespace on main**

잠시 기다리면 브라우저에 VS Code가 뜹니다. **그 터미널이 곧 컨테이너 안**이므로
바로 아래 [돌려보기](#돌려보기)로 넘어가면 됩니다.

## 로컬에서 하기

위와 같이 **내 저장소를 먼저 만든 뒤** 그것을 클론합니다.

```sh
git clone https://github.com/<본인 계정>/<내 저장소>.git
cd <내 저장소>
docker compose up -d
docker compose exec lab bash
```

처음 한 번은 이미지를 받느라 몇 분 걸립니다. 이후에는 몇 초면 뜹니다.
**이후 모든 `docker compose` 명령은 이 폴더에서 칩니다.**

VS Code를 쓴다면 Dev Containers 확장의 **Reopen in Container**를 골라도
됩니다. Codespaces와 같은 설정을 씁니다.

## 돌려보기

컨테이너 안에서 `make` 한 단어면 됩니다.

- 실행

```sh
make run
```

- 결과

```console
=== 정렬 비교: 퀵 · 병합 · Timsort ===
...
[정렬됨]
알고리즘     시간(ms)         비교         이동       메모리 재귀깊이 정렬 안정성
--------------------------------------------------------------------------------
quickSort       5.221      1606802       206784          8 B     16   yes    yes
mergeSort      16.882       815024      3337856     800008 B     18   yes    yes
timSort         0.216        99999            0          8 B      1   yes    yes
```

## 테스트

- 실행

```sh
make test
```

- 결과

```console
ok    quickSort      섞인 배열
ok    quickSort      이미 정렬된 배열
...
50 checks, 0 failures
```

테스트가 하나라도 실패하면 `make`가 0이 아닌 코드로 끝납니다. 과제를 내기
전에 이 명령이 통과하는지 확인하세요.

| 명령 | 하는 일 |
| --- | --- |
| `make run` | 예제 실행 |
| `make test` | 유닛 테스트 |
| `make charts` | 비교 그래프(SVG)를 `report/` 아래에 다시 만든다 |
| `make debug` | 디버그 심볼을 넣어 빌드 |
| `make clean` | 빌드 산출물 정리 |

## VS Code에서 실행·디버그

Codespaces나 Dev Containers로 열었다면 편집기에서 바로 됩니다.

| 하고 싶은 것 | 방법 |
| --- | --- |
| 파일 하나 실행 | 편집기 오른쪽 위 **▶ 버튼** (Code Runner) |
| 전체 실행 | `Cmd/Ctrl + Shift + B` (기본 빌드 작업이 `make run`) |
| 테스트 | 명령 팔레트 → **Tasks: Run Test Task** |
| C 디버그 | `F5` → **C 디버그 (현재 파일)** |

`F5`를 누르면 빌드가 먼저 돌아 심볼이 있는 바이너리를 만들고 디버거가
붙습니다. 중단점을 걸고 변수를 들여다볼 수 있습니다.

### 파일 하나만 실행·디버그하기

**C 디버그 (현재 파일)**은 열려 있는 `.c` 파일을 그대로 디버깅합니다. 폴더가
늘어나도 구성을 새로 만들 필요가 없습니다.

같은 폴더의 `.c`를 함께 링크하므로, 구현이 옆 파일에 있어도 됩니다. 대신
**한 폴더에 `main`은 하나만** 두세요.

터미널에서 직접 부를 수도 있습니다.

```sh
make src/main.debug.out && ./src/main.debug.out
```

### ▶ 버튼에 대해

편집기 오른쪽 위의 ▶ 버튼은 **Code Runner** 확장이 제공합니다. C든 Python이든
열려 있는 파일을 그대로 실행합니다.

두 확장이 각각 ▶ 버튼을 내놓으면 헷갈리므로, C/C++ 확장 쪽은 꺼 두었습니다
(`C_Cpp.debugShortcut`). 그쪽 버튼은 **파일 하나만** 컴파일해서 이런 오류를
냅니다.

```console
undefined reference to `bubbleSort'
collect2: error: ld returned 1 exit status
```

Code Runner도 기본 설정 그대로면 같은 문제가 나고, Python은 이미지에 없는
`python`을 찾습니다. 그래서 `.vscode/settings.json`에서 두 가지를 고쳐
두었습니다.

- C는 `Makefile`의 `%.out` 규칙을 거쳐 **같은 폴더의 `.c`를 함께** 빌드합니다
- Python은 `python3`로 실행합니다
- 출력 패널이 아니라 **터미널**에서 돌립니다. 그래야 `scanf`나 `input()`이 멈추지 않습니다

## 저장소 구조

```plaintext
algorithm-env/
├── .devcontainer/devcontainer.json  # Codespaces · Dev Containers 설정
├── compose.yml                      # 실습 컨테이너 (서비스 이름: lab)
├── Dockerfile                       # gcc · gdb · make · python3 · git
├── .vscode/                         # 빌드·디버그 설정 (F5, Cmd+Shift+B)
├── Makefile                         # run · test · debug · clean
├── src/
│   ├── sort.h                       # 공통 인터페이스 (SortAlgorithm)
│   ├── sortctx.h · sort.c           # 구현들이 함께 쓰는 도구 · 구현 표
│   ├── quickSort.c                  # 퀵 정렬 (배운 정렬, 피벗 방식 셋)
│   ├── mergeSort.c                  # 병합 정렬 (배운 정렬)
│   ├── timSort.c                    # Timsort (배우지 않은 정렬)
│   ├── bench.h · bench.c            # 시간 · 메모리 · 안정성 측정
│   └── main.c                       # 비교 결과 출력 (--csv · --quick · --disorder)
├── report/
│   ├── REPORT.md                    # 정렬 비교 보고서
│   ├── *.svg                        # 비교 그래프 (make charts가 만든다)
│   └── *.csv                        # 그래프·표가 나온 측정값 원본
├── tools/                           # 그래프를 그리는 스크립트 (표준 모듈만)
└── tests/
    └── test_sort.c                  # 유닛 테스트 (표준 C만 사용)
```

## 규약

- **실행 파일은 `*.out`으로 만듭니다.** `.gitignore`가 `*.out`만 걸러내므로,
  컨테이너에서 컴파일한 Linux 바이너리가 커밋에 섞이지 않습니다.
- **외부 라이브러리를 쓰지 않습니다.** 표준 라이브러리만 씁니다. 테스트도
  프레임워크 없이 `assert` 수준으로 직접 씁니다.
- 함수 이름은 camelCase(`bubbleSort`)를 씁니다.

## 자기 코드로 바꾸기

`src`의 버블 정렬은 환경이 도는지 보여 주는 예제일 뿐입니다. 지우고 자기
코드를 넣으세요. `tests`도 마찬가지입니다. 뼈대(`Makefile`, `src`, `tests`,
컨테이너 설정)만 남기면 됩니다.

## 변경 기록

버전과 변경 내역은 [CHANGELOG.md](CHANGELOG.md)에 있습니다.

## 정리

```sh
docker compose down
```

컨테이너를 지워도 코드는 그대로 남습니다.
