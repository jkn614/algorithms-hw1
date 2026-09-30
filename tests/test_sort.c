/* 유닛 테스트 — 외부 프레임워크 없이 표준 C만 쓴다.
 * 실행: make test-c
 *
 * 테스트도 공통 인터페이스로 쓴다. 구현 표(SORT_ALGORITHMS)를 훑으며
 * 모든 정렬에 같은 검사를 돌리므로, 정렬을 하나 더 넣어도 테스트는 그대로다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"
#include "sort.h"

static int checks = 0;
static int failures = 0;

static void report(const char *algo, const char *name, int ok) {
    checks++;
    if (ok) {
        printf("ok    %-14s %s\n", algo, name);
        return;
    }
    failures++;
    printf("FAIL  %-14s %s\n", algo, name);
}

/* --- int 배열 --------------------------------------------------------- */

static void expectSorted(const SortAlgorithm *algo, const char *name,
                         const int input[], const int want[], size_t n) {
    int a[32];
    SortStats stats;

    memcpy(a, input, n * sizeof(int));
    algo->sort(a, n, sizeof(a[0]), sortCompareInt, &stats);

    int ok = (n == 0) || memcmp(a, want, n * sizeof(int)) == 0;
    report(algo->name, name, ok);
    if (!ok) {
        printf("      got :");
        for (size_t i = 0; i < n; i++) {
            printf(" %d", a[i]);
        }
        printf("\n      want:");
        for (size_t i = 0; i < n; i++) {
            printf(" %d", want[i]);
        }
        printf("\n");
    }
}

/* --- 안정성 ----------------------------------------------------------- */

/* 원소와 비교 함수는 bench.h의 Record·recordCompare를 그대로 쓴다.
 * key로 정렬하고 tag에는 입력 순서를 담아 둔다. 정렬 뒤에도 같은 key끼리
 * tag가 오름차순이면 안정 정렬이다. */

static void expectStable(const SortAlgorithm *algo) {
    enum { N = 60 };
    Record a[N];
    SortStats stats;

    /* key는 0~4만 쓴다. 중복이 많아야 안정성이 드러난다. */
    for (int i = 0; i < N; i++) {
        a[i].key = (i * 7) % 5;
        a[i].tag = i;
    }
    algo->sort(a, N, sizeof(a[0]), recordCompare, &stats);

    int ok = 1;
    for (int i = 1; i < N; i++) {
        if (a[i - 1].key > a[i].key) {
            ok = 0; /* 정렬조차 안 됐다 */
        }
        if (a[i - 1].key == a[i].key && a[i - 1].tag > a[i].tag) {
            ok = 0; /* 같은 key인데 입력 순서가 뒤집혔다 */
        }
    }
    /* 구현 표의 stable 값이 실측과 맞는지 함께 본다. */
    report(algo->name, "안정성 (표의 stable 값과 일치)", ok == algo->stable);
}

/* --- 난수 배열을 qsort 결과와 맞춰 본다 ------------------------------- */

static void expectMatchesQsort(const SortAlgorithm *algo) {
    enum { N = 500 };
    int *a = malloc(N * sizeof(int));
    int *want = malloc(N * sizeof(int));
    SortStats stats;

    srand(20260901); /* 씨앗을 고정해 매번 같은 입력을 쓴다 */
    for (int i = 0; i < N; i++) {
        a[i] = rand() % 100; /* 중복이 섞이도록 좁은 범위를 쓴다 */
        want[i] = a[i];
    }
    qsort(want, N, sizeof(want[0]), sortCompareInt);
    algo->sort(a, N, sizeof(a[0]), sortCompareInt, &stats);

    report(algo->name, "난수 500개가 qsort 결과와 같다",
           memcmp(a, want, N * sizeof(int)) == 0);
    free(a);
    free(want);
}

/* --- 크기를 바꿔 가며 --------------------------------------------------- */

/* 블록 정렬은 블록 경계·병합 경계에서 틀리기 쉽다. 2의 거듭제곱이 아닌
 * 크기까지 훑어 본다. 안정성도 같은 자리에서 함께 본다. */
static void expectManySizes(const SortAlgorithm *algo) {
    enum { MAX_N = 200 };
    Record a[MAX_N];
    Record want[MAX_N];
    SortStats stats;
    int ok = 1;

    srand(20260902);
    for (size_t n = 0; n <= MAX_N; n++) {
        for (size_t i = 0; i < n; i++) {
            a[i].key = rand() % 20; /* 중복이 많은 입력 */
            a[i].tag = (int)i;
            want[i] = a[i];
        }
        /* qsort는 안정 정렬이 아니므로 tag까지 견줄 수 없다. key 순서는
         * qsort로 확인하고, tag 순서는 따로 본다. */
        qsort(want, n, sizeof(want[0]), recordCompare);
        algo->sort(a, n, sizeof(a[0]), recordCompare, &stats);

        for (size_t i = 0; i < n; i++) {
            if (a[i].key != want[i].key) {
                ok = 0;
            }
            /* 안정성은 안정 정렬이라고 주장하는 구현에만 요구한다.
             * 불안정 정렬(퀵)은 expectStable이 "정말 불안정한지"를 따로 본다. */
            if (algo->stable && i > 0 && a[i - 1].key == a[i].key &&
                a[i - 1].tag > a[i].tag) {
                ok = 0; /* 같은 key인데 입력 순서가 뒤집혔다 */
            }
        }
        if (!ok) {
            printf("      n = %zu에서 어긋났다\n", n);
            break;
        }
    }
    report(algo->name, algo->stable ? "n = 0..200 전부 정렬되고 안정하다"
                                  : "n = 0..200 전부 정렬된다", ok);
}

/* --- 큰 입력 · 여러 모양 ------------------------------------------------ */

/* Timsort는 n >= 64에서야 run 스택과 병합이 돈다. 병합 규칙이 틀리면 run이
 * 여러 개 쌓이는 큰 입력에서만 드러나므로, 모양별로 n = 5,000을 돌려 본다.
 * "톱니" 입력은 길이가 제각각인 run을 일부러 만든다(스택 규칙을 흔드는 입력). */
static int checkAgainstQsort(const SortAlgorithm *algo, Record *a, size_t n) {
    Record *want = malloc(n * sizeof(Record));
    SortStats stats;
    int ok = 1;

    memcpy(want, a, n * sizeof(Record));
    qsort(want, n, sizeof(want[0]), recordCompare);
    algo->sort(a, n, sizeof(a[0]), recordCompare, &stats);
    for (size_t i = 0; i < n; i++) {
        if (a[i].key != want[i].key) {
            ok = 0;
        }
        if (algo->stable && i > 0 && a[i - 1].key == a[i].key && a[i - 1].tag > a[i].tag) {
            ok = 0;
        }
    }
    free(want);
    return ok;
}

static void expectLargeInputs(const SortAlgorithm *algo) {
    enum { N = 5000 };
    Record *a = malloc(N * sizeof(Record));
    int ok = 1;

    for (int kind = 0; kind < INPUT_KIND_COUNT; kind++) {
        makeInput(a, N, (InputKind)kind, 20260904u);
        if (!checkAgainstQsort(algo, a, N)) {
            printf("      %s 입력에서 어긋났다\n", inputKindName((InputKind)kind));
            ok = 0;
        }
    }
    /* 톱니: 길이 1, 2, 3, ...인 오름차순 run이 이어지고, 사이사이 내림차순도 섞인다 */
    srand(20260905u);
    size_t i = 0;
    for (size_t len = 1; i < N; len++) {
        int start = rand() % 1000;
        int down = (len % 3 == 0);
        for (size_t k = 0; k < len && i < N; k++, i++) {
            a[i].key = down ? start - (int)k : start + (int)k;
            a[i].tag = (int)i;
        }
    }
    if (!checkAgainstQsort(algo, a, N)) {
        printf("      톱니 입력에서 어긋났다\n");
        ok = 0;
    }
    report(algo->name, "n = 5,000 모든 입력 모양 + 톱니가 qsort와 같다", ok);
    free(a);
}

/* 퀵 정렬의 실험용 방식(교과서 Lomuto)도 결과는 맞아야 한다. 느릴 뿐이다. */
static void expectQuickSchemes(void) {
    const QuickScheme saved = quickSortScheme;
    const SortAlgorithm *quick = NULL;

    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        if (SORT_ALGORITHMS[k].sort == quickSort) {
            quick = &SORT_ALGORITHMS[k];
        }
    }
    for (int q = 0; q < QUICK_SCHEME_COUNT && quick != NULL; q++) {
        quickSortScheme = (QuickScheme)q;
        enum { N = 1000 };
        Record a[N];
        int ok = 1;
        for (int kind = 0; kind < INPUT_KIND_COUNT; kind++) {
            makeInput(a, N, (InputKind)kind, 20260906u);
            ok = ok && checkAgainstQsort(quick, a, N);
        }
        char name[128];
        snprintf(name, sizeof(name), "피벗 방식 %s: 모든 입력 모양이 정렬된다",
                 quickSchemeName(quickSortScheme));
        report("quickSort", name, ok);
    }
    quickSortScheme = saved;
}

/* --- 측정값이 채워지는지 --------------------------------------------- */

static void expectStats(const SortAlgorithm *algo) {
    int a[] = {5, 1, 4, 2, 3};
    SortStats stats;

    algo->sort(a, 5, sizeof(a[0]), sortCompareInt, &stats);
    report(algo->name, "측정값이 채워진다",
           stats.compares > 0 && stats.moves > 0 &&
           stats.extraBytes >= sizeof(a[0]) && stats.maxDepth >= 1);
}

/* --- 측정 도구 자체 (bench.c) ----------------------------------------- */

static void expectInputShapes(void) {
    enum { N = 40 };
    Record a[N];
    int ok = 1;

    makeInput(a, N, INPUT_SORTED, 1u);
    if (!recordsSorted(a, N)) {
        ok = 0;
    }
    for (int i = 0; i < N; i++) {
        if (a[i].tag != i) {
            ok = 0; /* tag에는 입력 순서가 들어 있어야 한다 */
        }
    }
    report("bench", "makeInput(정렬됨)이 정렬된 입력을 만든다", ok);

    makeInput(a, N, INPUT_REVERSED, 1u);
    report("bench", "makeInput(역순)이 역순 입력을 만든다",
           N > 1 && !recordsSorted(a, N) && a[0].key > a[N - 1].key);

    makeInput(a, N, INPUT_FEW_UNIQUE, 1u);
    int distinct = 0;
    for (int i = 0; i < N; i++) {
        int seen = 0;
        for (int j = 0; j < i; j++) {
            if (a[j].key == a[i].key) {
                seen = 1;
            }
        }
        distinct += !seen;
    }
    report("bench", "makeInput(중복많음)의 서로 다른 key가 적다", distinct <= 8);

    makeInput(a, N, INPUT_ALL_EQUAL, 1u);
    int same = 1;
    for (int i = 1; i < N; i++) {
        same = same && a[i].key == a[0].key;
    }
    report("bench", "makeInput(모두같음)의 key가 전부 같다", same);

    enum { M = 1000 };
    Record b[M];
    makeInput(b, M, INPUT_NEARLY_SORTED, 1u);
    int outOfPlace = 0;
    for (int i = 0; i < M; i++) {
        outOfPlace += (b[i].key != i);
    }
    report("bench", "makeInput(거의정렬)은 정렬됐지만 몇 자리만 바뀌어 있다",
           outOfPlace > 0 && outOfPlace <= 2 * (M / 100 + 1));
}

static void expectDetectorsCatchViolations(void) {
    Record a[4] = {{1, 0}, {1, 1}, {2, 2}, {2, 3}};

    report("bench", "정렬·안정 판정이 멀쩡한 배열을 통과시킨다",
           recordsSorted(a, 4) && recordsStable(a, 4));

    Record swapped[4] = {{1, 1}, {1, 0}, {2, 2}, {2, 3}};
    report("bench", "같은 key의 순서가 뒤집히면 안정하지 않다고 본다",
           recordsSorted(swapped, 4) && !recordsStable(swapped, 4));

    Record unsorted[4] = {{2, 0}, {1, 1}, {3, 2}, {4, 3}};
    report("bench", "정렬되지 않은 배열을 잡아낸다", !recordsSorted(unsorted, 4));
}

static void expectBenchRun(const SortAlgorithm *algo) {
    enum { N = 300 };
    Record input[N];

    makeInput(input, N, INPUT_FEW_UNIQUE, 20260903u);
    BenchResult r = benchRun(algo, input, N, 2);

    report(algo->name, "benchRun이 정렬·안정·측정값을 채운다",
           r.sorted && r.stable == algo->stable && r.millis >= 0.0 &&
           r.stats.compares > 0 && r.n == N && r.algo == algo);
}

/* --- 전부 돌린다 ------------------------------------------------------ */

int main(void) {
    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        const SortAlgorithm *algo = &SORT_ALGORITHMS[k];
        {
            const int a[] = {6, 8, 5, 9, 10, 1, 7, 2, 4, 3};
            const int want[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
            expectSorted(algo, "섞인 배열", a, want, 10);
        }
        {
            const int a[] = {1, 2, 3, 4, 5};
            const int want[] = {1, 2, 3, 4, 5};
            expectSorted(algo, "이미 정렬된 배열", a, want, 5);
        }
        {
            const int a[] = {5, 4, 3, 2, 1};
            const int want[] = {1, 2, 3, 4, 5};
            expectSorted(algo, "역순 배열", a, want, 5);
        }
        {
            const int a[] = {3, 1, 3, 1, 2};
            const int want[] = {1, 1, 2, 3, 3};
            expectSorted(algo, "중복이 있는 배열", a, want, 5);
        }
        {
            const int a[] = {2, 2, 2, 2};
            const int want[] = {2, 2, 2, 2};
            expectSorted(algo, "모두 같은 값", a, want, 4);
        }
        {
            const int a[] = {42};
            const int want[] = {42};
            expectSorted(algo, "원소 하나", a, want, 1);
        }
        {
            /* n = 0이면 배열을 건드리지 않는다. */
            const int a[1] = {0};
            const int want[1] = {0};
            expectSorted(algo, "빈 배열", a, want, 0);
        }
        expectStable(algo);
        expectManySizes(algo);
        expectMatchesQsort(algo);
        expectLargeInputs(algo);
        expectStats(algo);
        expectBenchRun(algo);
        printf("\n");
    }

    expectQuickSchemes();
    expectInputShapes();
    expectDetectorsCatchViolations();

    printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
