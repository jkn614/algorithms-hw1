/* 병합 정렬 — 반으로 나눠 각각 정렬한 뒤, 정렬된 두 반쪽을 하나로 합친다.
 *
 * 원소 하나짜리 배열은 늘 정렬되어 있다. 거기서부터 합쳐 올라오면 끝난다.
 * 입력이 어떤 모양이든 나누는 방식이 같으므로 **언제나 O(n log n)** 이다.
 *
 * 대가는 메모리다. 두 반쪽을 합칠 때 원본을 덮어쓰며 읽을 수는 없으므로,
 * 크기 n짜리 보조 배열을 한 번 잡아 두고 병합마다 재사용한다 — O(n).
 *
 * 수업에서 배운 형태 그대로 둔다. "이미 정렬돼 있으면 병합을 건너뛴다" 같은
 * 최적화를 넣지 않았다. 그래야 Timsort가 무엇을 더했는지가 실험에서 드러난다.
 */
#include "sort.h"

#include <stdlib.h>

#include "sortctx.h"

/* a[lo..mid)와 a[mid..hi)는 각각 정렬되어 있다. 둘을 합쳐 a[lo..hi)에 쓴다. */
static void mergeHalves(SortCtx *c, char *aux, size_t lo, size_t mid, size_t hi) {
    const size_t sz = c->size;
    for (size_t k = lo; k < hi; k++) {
        sortMove(c, aux + k * sz, sortElemAt(c, k)); /* 두 반쪽을 통째로 옮겨 두고 */
    }
    size_t i = lo;
    size_t j = mid;
    size_t k = lo;
    while (i < mid && j < hi) {
        /* 오른쪽이 **엄격히 작을 때만** 오른쪽을 가져온다. 같으면 왼쪽이 먼저다.
         * 이 '<' 한 글자가 안정성을 만든다. '<='이면 같은 값의 순서가 뒤집힌다. */
        if (sortComparePtr(c, aux + j * sz, aux + i * sz) < 0) {
            sortMove(c, sortElemAt(c, k++), aux + j++ * sz);
        } else {
            sortMove(c, sortElemAt(c, k++), aux + i++ * sz);
        }
    }
    while (i < mid) {
        sortMove(c, sortElemAt(c, k++), aux + i++ * sz);
    }
    while (j < hi) {
        sortMove(c, sortElemAt(c, k++), aux + j++ * sz);
    }
}

static void mergeRange(SortCtx *c, char *aux, size_t lo, size_t hi, size_t depth) {
    sortNoteDepth(c, depth);
    if (hi - lo < 2) {
        return; /* 원소 하나는 이미 정렬되어 있다 */
    }
    size_t mid = lo + (hi - lo) / 2;
    mergeRange(c, aux, lo, mid, depth + 1);
    mergeRange(c, aux, mid, hi, depth + 1);
    mergeHalves(c, aux, lo, mid, hi);
}

void mergeSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    SortCtx c;
    if (!sortBegin(&c, base, n, size, cmp, stats)) {
        return;
    }
    char *aux = (char *)malloc(n * size); /* 보조 배열 n칸 — 병합 정렬의 대가 */
    if (aux != NULL) {
        sortNoteExtra(&c, n * size);
        mergeRange(&c, aux, 0, n, 1);
        free(aux);
    }
    sortEnd(&c);
}
