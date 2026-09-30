/* 공통 토대 — 구현 셋이 함께 쓰는 도구와, 그 구현들을 모아 둔 표.
 *
 * 정렬 알고리즘 자체는 각각 제 파일에 있다:
 *   quickSort.c · mergeSort.c · timSort.c
 */
#include "sort.h"

#include <stdlib.h>
#include <string.h>

#include "sortctx.h"

void sortStatsReset(SortStats *stats) {
    if (stats == NULL) {
        return;
    }
    stats->compares = 0;
    stats->moves = 0;
    stats->extraBytes = 0;
    stats->maxDepth = 1; /* 재귀를 쓰지 않아도 깊이는 1로 센다 */
}

int sortCompareInt(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y); /* 뺄셈은 overflow가 날 수 있어 쓰지 않는다 */
}

/* --- 작업 문맥 --------------------------------------------------------- */

int sortBegin(SortCtx *c, void *base, size_t n, size_t size,
              SortCompare cmp, SortStats *stats) {
    sortStatsReset(stats);
    if (base == NULL || cmp == NULL || size == 0 || n < 2) {
        return 0;
    }
    c->base = (char *)base;
    c->size = size;
    c->cmp = cmp;
    c->stats = stats;
    c->tmp = (char *)malloc(size); /* 원소 한 칸. 교환이 거쳐 가는 자리 */
    if (c->tmp == NULL) {
        return 0;
    }
    if (stats != NULL) {
        stats->extraBytes = size;
    }
    return 1;
}

void sortEnd(SortCtx *c) {
    free(c->tmp);
    c->tmp = NULL;
}

char *sortElemAt(const SortCtx *c, size_t i) {
    return c->base + i * c->size;
}

int sortCompareAt(SortCtx *c, size_t i, size_t j) {
    if (c->stats != NULL) {
        c->stats->compares++;
    }
    return c->cmp(sortElemAt(c, i), sortElemAt(c, j));
}

int sortCompareTmp(SortCtx *c, size_t i) {
    if (c->stats != NULL) {
        c->stats->compares++;
    }
    return c->cmp(sortElemAt(c, i), c->tmp);
}

void sortMove(SortCtx *c, void *dst, const void *src) {
    memcpy(dst, src, c->size);
    if (c->stats != NULL) {
        c->stats->moves++;
    }
}

int sortComparePtr(SortCtx *c, const void *x, const void *y) {
    if (c->stats != NULL) {
        c->stats->compares++;
    }
    return c->cmp(x, y);
}

void sortSwap(SortCtx *c, size_t i, size_t j) {
    sortMove(c, c->tmp, sortElemAt(c, i));
    sortMove(c, sortElemAt(c, i), sortElemAt(c, j));
    sortMove(c, sortElemAt(c, j), c->tmp);
}

void sortNoteDepth(SortCtx *c, size_t depth) {
    if (c->stats != NULL && depth > c->stats->maxDepth) {
        c->stats->maxDepth = depth;
    }
}

void sortNoteExtra(SortCtx *c, size_t bytes) {
    /* tmp 한 칸(size)은 늘 잡혀 있으므로 그 위에 얹어 센다. */
    if (c->stats != NULL && c->size + bytes > c->stats->extraBytes) {
        c->stats->extraBytes = c->size + bytes;
    }
}

/* --- 구현 표 ----------------------------------------------------------- */

/* 정렬을 하나 더 만들면 파일을 하나 더 두고 여기에 한 줄 넣는다.
 * main.c도 테스트도 이 표만 훑으므로 그것으로 끝이다. */
const SortAlgorithm SORT_ALGORITHMS[] = {
    {"quickSort", "O(n log n)", "O(log n)", 0, quickSort},
    {"mergeSort", "O(n log n)", "O(n)",     1, mergeSort},
    {"timSort",   "O(n log n)", "O(n)",     1, timSort},
};

const size_t SORT_ALGORITHM_COUNT = sizeof(SORT_ALGORITHMS) / sizeof(SORT_ALGORITHMS[0]);
