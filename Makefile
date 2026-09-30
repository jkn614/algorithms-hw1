# 빌드와 테스트를 한 단어로 돌리기 위한 Makefile.
# 컨테이너 안에서 실행한다 (docker compose exec lab bash).
#
#   make run     예제 실행
#   make test    유닛 테스트
#   make charts  비교 그래프(SVG)를 report/ 아래에 다시 만든다
#   make debug   디버그 심볼을 넣어 빌드 (VS Code의 F5가 쓴다)
#   make clean   빌드 산출물 정리
#
# 실행 파일은 `*.out`으로 만든다. .gitignore가 그것만 걸러낸다.

CC ?= gcc
CFLAGS ?= -std=c17 -Wall -Wextra -O2
# 디버그 빌드: 최적화를 끄고 심볼을 남긴다 (VS Code의 F5가 이 결과물을 쓴다).
# `-I`는 아래 패턴 규칙이 대상 파일의 폴더로 붙인다. 여기서 고정하지 않는다.
DEBUGFLAGS ?= -std=c17 -Wall -Wextra -g -O0

.PHONY: all run run-c test test-c charts debug clean

all: test

run: run-c

run-c: src/main.out
	@./src/main.out

test: test-c

test-c: tests/test_sort.out
	@./tests/test_sort.out

# 그래프는 표준 모듈만 쓰는 tools/plot.py가 SVG로 직접 찍는다 (외부 라이브러리 없음).
charts: src/main.out
	@python3 tools/plot.py

debug: src/main.debug.out

# 파일 하나를 그 자리에서 빌드한다. 같은 폴더의 .c를 함께 링크하므로 헤더에
# 선언만 있고 구현이 옆 파일에 있어도 된다. 대신 **한 폴더에 main은 하나만** 둔다.
#   %.out        실행용 (Code Runner의 ▶ 버튼이 이 규칙을 부른다)
#   %.debug.out  디버그용 (VS Code의 "C 디버그 (현재 파일)"이 부른다)
# 명시 규칙(tests/test_sort.out 등)이 있으면 그쪽이 우선한다.
%.out: %.c
	$(CC) $(CFLAGS) -I$(@D) -o $@ $(wildcard $(@D)/*.c)

%.debug.out: %.c
	$(CC) $(DEBUGFLAGS) -I$(@D) -o $@ $(wildcard $(@D)/*.c)

# 정렬 구현이 파일마다 하나씩이라 여기에도 나열한다. 새 정렬을 넣으면 이 줄도 본다.
SORT_SRC = src/sort.c src/quickSort.c src/mergeSort.c src/timSort.c

tests/test_sort.out: tests/test_sort.c $(SORT_SRC) src/sort.h src/sortctx.h src/bench.c src/bench.h
	$(CC) $(CFLAGS) -Isrc -o $@ tests/test_sort.c $(SORT_SRC) src/bench.c

clean:
	rm -f src/*.out tests/*.out
