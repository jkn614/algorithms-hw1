/* 정렬 셋을 같은 잣대로 재는 도구.
 *
 * 알고리즘(sort.c)과 측정(bench.c)을 나눠 둔다. 정렬은 자기가 측정당하는
 * 줄 모르고, 측정은 어떤 정렬인지 모른다. 둘을 잇는 것은 SortAlgorithm뿐이다.
 */
#ifndef BENCH_H
#define BENCH_H

#include <stddef.h>

#include "sort.h"

/* 측정에 쓰는 원소. key로 정렬하고 tag에는 입력 순서를 담아 둔다.
 * 정렬 뒤에도 같은 key끼리 tag가 오름차순이면 안정 정렬이다.
 * key만 있는 int 배열로는 안정성을 볼 수 없어서 원소를 이렇게 잡았다. */
typedef struct Record {
    int key;
    int tag;
} Record;

/* key만 본다. tag는 비교에 넣지 않는다 (넣으면 모든 정렬이 안정해 보인다). */
int recordCompare(const void *a, const void *b);

/* 입력 모양. 정렬은 입력에 따라 성능이 크게 달라진다. */
typedef enum InputKind {
    INPUT_RANDOM,     /* 무작위 */
    INPUT_SORTED,     /* 이미 정렬됨 */
    INPUT_REVERSED,   /* 역순 */
    INPUT_NEARLY_SORTED, /* 정렬된 배열에서 1%만 자리를 바꿈 */
    INPUT_FEW_UNIQUE, /* 중복 많음 (서로 다른 값 8개) */
    INPUT_ALL_EQUAL,  /* 모두 같은 값 */
    INPUT_KIND_COUNT
} InputKind;

const char *inputKindName(InputKind kind);

/* a[0..n-1]을 kind 모양으로 채운다. seed를 고정하면 매번 같은 입력이 나온다. */
void makeInput(Record *a, size_t n, InputKind kind, unsigned seed);

/* 정렬된 배열을 만든 뒤 무작위 두 자리를 swaps번 맞바꾼다. 흐트러짐의 정도를
 * 바꿔 가며 재는 실험(main.c --disorder)이 쓴다. */
void makeNearlySorted(Record *a, size_t n, size_t swaps, unsigned seed);

int recordsSorted(const Record *a, size_t n); /* key가 오름차순인가 */
int recordsStable(const Record *a, size_t n); /* 같은 key의 tag 순서가 남았는가 */

typedef struct BenchResult {
    const SortAlgorithm *algo;
    size_t n;
    double millis;   /* 한 번 도는 데 걸린 시간 */
    SortStats stats; /* 마지막 회차의 측정값 */
    int sorted;      /* 결과가 정렬됐는가 — 측정 전에 이것부터 본다 */
    int stable;      /* 실제로 안정했는가 (구현 표의 주장이 아니라 실측) */
} BenchResult;

/* input을 복사해 reps번 정렬하고 평균 시간을 남긴다. 복사 시간은 빼고 잰다. */
BenchResult benchRun(const SortAlgorithm *algo, const Record *input, size_t n, int reps);

#endif /* BENCH_H */
