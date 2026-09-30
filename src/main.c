/* 정렬 비교 — 퀵 / 병합 / Timsort.
 *
 *   make run                   사람이 읽는 비교 표
 *   ./src/main.out --csv       같은 측정을 CSV로 (tools/plot.py가 쓴다)
 *   ./src/main.out --quick     퀵 정렬의 피벗·분할 방식을 바꿔 가며 잰 CSV
 *   ./src/main.out --disorder  정렬된 배열을 조금씩 흐트러뜨리며 잰 CSV
 *
 * 부르는 쪽은 정렬 이름을 하나도 적지 않는다. 구현 표(SORT_ALGORITHMS)를
 * 훑을 뿐이다. 무엇을 잴지도 아래 SPECS 한 곳에만 적는다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bench.h"
#include "sort.h"

/* --- 무엇을 잴 것인가 -------------------------------------------------- */

typedef struct Spec {
    const char *scope; /* kinds: 입력 모양별 · growth: n을 키우며 */
    InputKind kind;
    size_t n;
    int reps;
} Spec;

/* 세 정렬이 모두 O(n log n)이라 샘플(n = 4,000)보다 훨씬 크게 잰다.
 * 작으면 시간이 clock()의 눈금 아래로 떨어진다. */
static const Spec SPECS[] = {
    {"kinds", INPUT_RANDOM, 100000, 5},
    {"kinds", INPUT_SORTED, 100000, 5},
    {"kinds", INPUT_REVERSED, 100000, 5},
    {"kinds", INPUT_NEARLY_SORTED, 100000, 5},
    {"kinds", INPUT_FEW_UNIQUE, 100000, 5},
    {"kinds", INPUT_ALL_EQUAL, 100000, 5},
    {"growth", INPUT_RANDOM, 1000, 50},
    {"growth", INPUT_RANDOM, 4000, 20},
    {"growth", INPUT_RANDOM, 16000, 10},
    {"growth", INPUT_RANDOM, 64000, 5},
    {"growth", INPUT_RANDOM, 256000, 3},
    {"growth", INPUT_RANDOM, 1024000, 3},
};

static const size_t SPEC_COUNT = sizeof(SPECS) / sizeof(SPECS[0]);

/* 측정 결과 한 줄을 받아 가는 곳. 표로 찍을지 CSV로 찍을지만 다르다 —
 * 정렬을 함수 포인터로 갈아 끼웠듯, 출력도 같은 수를 쓴다. */
typedef void (*RowSink)(const Spec *spec, const BenchResult *r);

/* SPECS를 훑으며 측정하고, 한 줄이 나올 때마다 sink에 넘긴다.
 * onSpec은 줄을 찍기 전에 불린다 (표가 소제목을 낼 자리). */
static void measureAll(RowSink sink, void (*onSpec)(const Spec *spec)) {
    for (size_t s = 0; s < SPEC_COUNT; s++) {
        const Spec *spec = &SPECS[s];
        Record *input = (Record *)malloc(spec->n * sizeof(Record));
        if (input == NULL) {
            return;
        }
        makeInput(input, spec->n, spec->kind, 20260901u);
        if (onSpec != NULL) {
            onSpec(spec);
        }
        for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
            BenchResult r = benchRun(&SORT_ALGORITHMS[k], input, spec->n, spec->reps);
            sink(spec, &r);
        }
        free(input);
    }
}

/* --- 사람이 읽는 표 ---------------------------------------------------- */

#define ROW_FORMAT "%-11s %9.3f %12zu %12zu %10zu B %6zu %5s %6s\n"
#define ROW_HEADER "알고리즘     시간(ms)         비교         이동       메모리 재귀깊이 정렬 안정성\n"
#define ROW_RULE   "--------------------------------------------------------------------------------\n"

static void tableRow(const Spec *spec, const BenchResult *r) {
    (void)spec;
    printf(ROW_FORMAT, r->algo->name, r->millis, r->stats.compares, r->stats.moves,
           r->stats.extraBytes, r->stats.maxDepth, r->sorted ? "yes" : "NO!",
           r->stable ? "yes" : "no");
}

static void tableSpecHeader(const Spec *spec) {
    static const char *lastScope = NULL;

    if (lastScope == NULL || strcmp(lastScope, spec->scope) != 0) {
        if (strcmp(spec->scope, "kinds") == 0) {
            printf("입력 모양별 비교 (n = %zu, %d회 평균)\n", spec->n, spec->reps);
        } else {
            printf("\nn을 키우며 (무작위 입력)\n");
        }
        lastScope = spec->scope;
    }
    if (strcmp(spec->scope, "kinds") == 0) {
        printf("\n[%s]\n", inputKindName(spec->kind));
    } else {
        printf("\n[n = %zu]\n", spec->n);
    }
    printf("%s%s", ROW_HEADER, ROW_RULE);
}

/* 구현 표가 뭐라고 주장하는지 먼저 보여 준다. 아래 측정과 견줘 보라고. */
static void printDeclarations(void) {
    printf("구현 표 (SortAlgorithm이 주장하는 값)\n");
    printf("알고리즘    시간복잡도     메모리     안정성\n");
    printf("%s", ROW_RULE);
    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        const SortAlgorithm *algo = &SORT_ALGORITHMS[k];
        printf("%-11s %-14s %-10s %s\n", algo->name, algo->timeComplexity,
               algo->spaceComplexity, algo->stable ? "stable" : "unstable");
    }
    printf("\n");
}

static void reportTable(void) {
    printf("=== 정렬 비교: 퀵 · 병합 · Timsort ===\n");
    printf("원소는 (key, tag) %zu바이트. key로 정렬하고 tag로 안정성을 본다.\n\n",
           sizeof(Record));
    printDeclarations();
    measureAll(tableRow, tableSpecHeader);

    printf("\n읽는 법\n");
    printf("  시간   : 같은 기계에서만 견준다. 비교·이동 횟수가 더 믿을 만하다.\n");
    printf("  메모리 : 퀵은 원소 한 칸(%zu B)뿐이고, 병합은 n칸을 늘 잡는다.\n",
           sizeof(Record));
    printf("           Timsort는 짧은 쪽 run만 복사하므로 많아야 n/2칸, 병합이 없으면 0칸이다.\n");
    printf("  재귀   : 퀵·병합의 스택 사용량. Timsort는 반복문과 run 스택이라 1이다.\n");
    printf("  안정성 : 표의 주장이 아니라 tag 순서로 실측한 값이다.\n");
}

/* --- 기계가 읽는 CSV --------------------------------------------------- */

/* CSV에는 ASCII 키를 쓴다. 표에 찍는 한글 이름(inputKindName)과 따로 둔다. */
static const char *inputKindKey(InputKind kind) {
    switch (kind) {
        case INPUT_RANDOM:        return "random";
        case INPUT_SORTED:        return "sorted";
        case INPUT_REVERSED:      return "reversed";
        case INPUT_NEARLY_SORTED: return "nearly-sorted";
        case INPUT_FEW_UNIQUE:    return "few-unique";
        case INPUT_ALL_EQUAL:     return "all-equal";
        default:                  return "unknown";
    }
}

static void csvRow(const Spec *spec, const BenchResult *r) {
    printf("%s,%s,%zu,%s,%.3f,%zu,%zu,%zu,%zu,%d,%d\n", spec->scope,
           inputKindKey(spec->kind), spec->n, r->algo->name, r->millis,
           r->stats.compares, r->stats.moves, r->stats.extraBytes,
           r->stats.maxDepth, r->sorted, r->stable);
}

static void reportCsv(void) {
    printf("scope,input,n,algo,millis,compares,moves,extraBytes,maxDepth,sorted,stable\n");
    measureAll(csvRow, NULL);
}

/* 구현 표에서 이름으로 찾는다. 인덱스를 박아 두면 표 순서가 바뀔 때 깨진다. */
static const SortAlgorithm *findAlgorithm(const char *name) {
    for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
        if (strcmp(SORT_ALGORITHMS[k].name, name) == 0) {
            return &SORT_ALGORITHMS[k];
        }
    }
    return NULL;
}

/* --- 퀵 정렬 피벗 실험 --------------------------------------------------- */

/* 피벗·분할 방식 셋을 입력 모양 넷에서 잰다. 교과서형 퀵 정렬이 어디서 무너지는지,
 * 기본값이 왜 그 방식인지를 재는 실험이다. n은 작게 잡는다 — 무너진 쪽은 O(n^2)
 * 이고 재귀 깊이가 n이라, 크게 잡으면 스택이 넘친다(그것 자체가 결과다). */
static void reportQuickSweep(void) {
    static const size_t SIZES[] = {1000, 2000, 4000, 8000, 16000};
    static const InputKind KINDS[] = {INPUT_RANDOM, INPUT_SORTED, INPUT_FEW_UNIQUE,
                                      INPUT_ALL_EQUAL};
    const size_t sizeCount = sizeof(SIZES) / sizeof(SIZES[0]);
    const size_t kindCount = sizeof(KINDS) / sizeof(KINDS[0]);
    const SortAlgorithm *algo = findAlgorithm("quickSort");
    const QuickScheme saved = quickSortScheme;

    if (algo == NULL) {
        return;
    }
    printf("input,n,scheme,compares,moves,maxDepth,millis,sorted,stable\n");
    for (size_t k = 0; k < kindCount; k++) {
        for (size_t s = 0; s < sizeCount; s++) {
            size_t n = SIZES[s];
            Record *input = (Record *)malloc(n * sizeof(Record));
            if (input == NULL) {
                return;
            }
            makeInput(input, n, KINDS[k], 20260901u);
            for (int q = 0; q < QUICK_SCHEME_COUNT; q++) {
                quickSortScheme = (QuickScheme)q;
                BenchResult r = benchRun(algo, input, n, 3);
                printf("%s,%zu,%s,%zu,%zu,%zu,%.3f,%d,%d\n", inputKindKey(KINDS[k]), n,
                       quickSchemeName(quickSortScheme), r.stats.compares, r.stats.moves,
                       r.stats.maxDepth, r.millis, r.sorted, r.stable);
            }
            free(input);
        }
    }
    quickSortScheme = saved;
}

/* --- 흐트러짐 실험 ------------------------------------------------------- */

/* 정렬된 배열에서 두 자리를 맞바꾸는 횟수를 늘려 간다. 0이면 정렬됨이고,
 * n쯤 되면 사실상 무작위다. 어느 정렬이 "이미 정렬된 정도"를 활용하는지 본다. */
static void reportDisorderSweep(void) {
    enum { N = 100000 };
    static const size_t SWAPS[] = {0, 10, 100, 1000, 10000, 100000};
    const size_t swapCount = sizeof(SWAPS) / sizeof(SWAPS[0]);
    Record *input = (Record *)malloc(N * sizeof(Record));

    if (input == NULL) {
        return;
    }
    printf("n,swaps,algo,compares,moves,extraBytes,millis,sorted,stable\n");
    for (size_t s = 0; s < swapCount; s++) {
        makeNearlySorted(input, N, SWAPS[s], 20260901u);
        for (size_t k = 0; k < SORT_ALGORITHM_COUNT; k++) {
            BenchResult r = benchRun(&SORT_ALGORITHMS[k], input, N, 5);
            printf("%d,%zu,%s,%zu,%zu,%zu,%.3f,%d,%d\n", N, SWAPS[s], r.algo->name,
                   r.stats.compares, r.stats.moves, r.stats.extraBytes, r.millis,
                   r.sorted, r.stable);
        }
    }
    free(input);
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--csv") == 0) {
        reportCsv();
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--quick") == 0) {
        reportQuickSweep();
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--disorder") == 0) {
        reportDisorderSweep();
        return 0;
    }
    reportTable();
    return 0;
}
