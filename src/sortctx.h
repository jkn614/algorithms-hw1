/* 정렬 구현들이 함께 쓰는 작업 문맥 — 구현 전용 헤더.
 *
 * sort.h가 바깥에 보이는 인터페이스라면, 이쪽은 구현끼리만 쓰는 도구다.
 * 정렬을 부르는 코드(main.c, bench.c, 테스트)는 이 파일을 include하지 않는다.
 *
 * 헤더에 드러나는 이름은 파일 밖에서도 보이므로 sort- 를 붙여 둔다.
 * 한 파일 안에서만 쓰는 함수는 static으로 감춘다.
 */
#ifndef SORTCTX_H
#define SORTCTX_H

#include <stddef.h>

#include "sort.h"

/* 정렬 함수의 인자 다섯 개를 한 덩어리로 들고 다닌다. tmp는 원소 하나를
 * 잠시 담아 두는 자리다. 병합이 쓰는 보조 배열은 각 구현이 따로 잡고,
 * 그 크기를 sortNoteExtra로 알린다. */
typedef struct SortCtx {
    char *base;
    size_t size;
    SortCompare cmp;
    SortStats *stats;
    char *tmp;
} SortCtx;

/* 정렬 셋이 똑같이 하는 앞처리·뒤처리. 정렬할 것이 없으면 sortBegin이 0을
 * 돌려주고, 그때는 sortEnd를 부르지 않는다. */
int sortBegin(SortCtx *c, void *base, size_t n, size_t size,
              SortCompare cmp, SortStats *stats);
void sortEnd(SortCtx *c);

char *sortElemAt(const SortCtx *c, size_t i);
int sortCompareAt(SortCtx *c, size_t i, size_t j);           /* a[i]와 a[j] */
int sortCompareTmp(SortCtx *c, size_t i);                    /* a[i]와 tmp */
int sortComparePtr(SortCtx *c, const void *x, const void *y); /* 보조 배열의 원소까지 */
void sortMove(SortCtx *c, void *dst, const void *src);
void sortSwap(SortCtx *c, size_t i, size_t j);

/* 측정값 갱신. 알고리즘이 "지금 깊이 d에 있다", "보조 공간을 b바이트 잡았다"를
 * 알려 주면 최댓값만 남긴다. tmp 한 칸은 sortBegin이 이미 세어 두었다. */
void sortNoteDepth(SortCtx *c, size_t depth);
void sortNoteExtra(SortCtx *c, size_t bytes);

#endif /* SORTCTX_H */
