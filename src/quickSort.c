/* 퀵 정렬 — 피벗을 하나 골라 "작은 것 | 피벗 | 큰 것"으로 나누고 양쪽을 다시 정렬한다.
 *
 * 평균 O(n log n)이고 추가 배열이 필요 없어 실제로 가장 빠른 축에 든다.
 * 대신 **피벗을 어떻게 고르고 어떻게 나누느냐**에 따라 O(n^2)으로 무너진다.
 * 그래서 세 방식을 모두 두고, 기본값은 무너지지 않는 쪽으로 잡았다.
 *
 *   QUICK_FIRST_LOMUTO   첫 원소 피벗 + Lomuto 분할  → 정렬된 입력에서 O(n^2)
 *   QUICK_RANDOM_LOMUTO  무작위 피벗 + Lomuto 분할   → 같은 값이 많으면 O(n^2)
 *   QUICK_MEDIAN3_HOARE  세 값의 중앙값 + Hoare 분할 → 기본값
 *
 * 퀵 정렬은 **안정 정렬이 아니다.** 분할이 멀리 떨어진 원소끼리 교환하므로
 * 같은 key의 앞뒤가 뒤집힌다. 테스트가 이것을 실측한다.
 */
#include "sort.h"

#include "sortctx.h"

QuickScheme quickSortScheme = QUICK_MEDIAN3_HOARE;

const char *quickSchemeName(QuickScheme scheme) {
    switch (scheme) {
        case QUICK_FIRST_LOMUTO:  return "first-lomuto";
        case QUICK_RANDOM_LOMUTO: return "random-lomuto";
        case QUICK_MEDIAN3_HOARE: return "median3-hoare";
        default:                  return "?";
    }
}

/* --- 기본값: 세 값의 중앙값 + Hoare 분할 --------------------------------- */

/* a[lo], a[mid], a[hi-1] 셋 중 가운데 값을 피벗으로 고르고 a[lo]에 둔다.
 * 정렬된 입력이면 한가운데 값이 뽑혀 배열이 반으로 갈린다. */
static void medianOfThreeToFront(SortCtx *c, size_t lo, size_t hi) {
    size_t mid = lo + (hi - lo) / 2;
    if (sortCompareAt(c, mid, lo) < 0) {
        sortSwap(c, mid, lo);
    }
    if (sortCompareAt(c, hi - 1, mid) < 0) {
        sortSwap(c, hi - 1, mid);
        if (sortCompareAt(c, mid, lo) < 0) {
            sortSwap(c, mid, lo);
        }
    }
    /* 이제 a[lo] <= a[mid] <= a[hi-1]. 중앙값을 맨 앞으로 옮긴다.
     * a[hi-1]은 피벗 이상이므로 아래 i 스캔의 벽(sentinel)이 된다. */
    sortSwap(c, lo, mid);
}

/* Hoare 분할. 양 끝에서 안쪽으로 걸어 들어오며 잘못 놓인 쌍을 바꾼다.
 * 핵심은 **피벗과 같은 값에서도 멈춘다**는 것이다(< 와 > 로 비교한다).
 * 그래서 모든 값이 같아도 i와 j가 가운데서 만나 배열이 반으로 갈린다. */
static size_t partitionHoare(SortCtx *c, size_t lo, size_t hi) {
    medianOfThreeToFront(c, lo, hi);
    size_t i = lo;
    size_t j = hi;
    for (;;) {
        do {
            i++;
        } while (i < hi && sortCompareAt(c, i, lo) < 0);
        do {
            j--;
        } while (sortCompareAt(c, j, lo) > 0); /* a[lo]에서 반드시 멈춘다 */
        if (i >= j) {
            break;
        }
        sortSwap(c, i, j);
    }
    if (j != lo) {
        sortSwap(c, lo, j); /* 피벗을 제자리로 */
    }
    return j;
}

/* 작은 쪽만 재귀하고 큰 쪽은 반복문으로 돈다. 그러면 재귀 깊이가 어떤 입력에서도
 * log2(n)을 넘지 않는다 — 작은 쪽은 늘 절반 이하이기 때문이다. */
static void quickHoare(SortCtx *c, size_t lo, size_t hi, size_t depth) {
    sortNoteDepth(c, depth);
    while (hi - lo > 1) {
        if (hi - lo == 2) {
            if (sortCompareAt(c, lo + 1, lo) < 0) {
                sortSwap(c, lo, lo + 1);
            }
            return;
        }
        size_t p = partitionHoare(c, lo, hi);
        if (p - lo < hi - (p + 1)) {
            quickHoare(c, lo, p, depth + 1);
            lo = p + 1;
        } else {
            quickHoare(c, p + 1, hi, depth + 1);
            hi = p;
        }
    }
}

/* --- 실험용: Lomuto 분할 (교과서 기본형) ---------------------------------- */

/* 난수는 rand()를 쓰지 않는다. 입력을 만드는 쪽이 rand()를 쓰므로, 여기서
 * 섞으면 측정이 재현되지 않는다. 정렬 한 번마다 같은 씨앗에서 시작한다. */
static unsigned nextRandom(unsigned *state) {
    unsigned x = *state; /* xorshift32 */
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/* Lomuto 분할. 피벗을 맨 뒤로 보내 두고, 왼쪽부터 "피벗 이하"를 앞으로 모은다.
 * '<='로 비교하므로 **피벗과 같은 값은 전부 왼쪽으로 간다.** 모든 값이 같으면
 * 오른쪽이 텅 비어 한 번에 한 원소씩만 줄어든다 — O(n^2), 재귀 깊이 n. */
static size_t partitionLomuto(SortCtx *c, size_t lo, size_t hi, size_t pivot) {
    size_t last = hi - 1;
    if (pivot != last) {
        sortSwap(c, pivot, last);
    }
    size_t store = lo;
    for (size_t i = lo; i < last; i++) {
        if (sortCompareAt(c, i, last) <= 0) {
            if (i != store) {
                sortSwap(c, i, store);
            }
            store++;
        }
    }
    if (store != last) {
        sortSwap(c, store, last);
    }
    return store;
}

/* 양쪽을 모두 재귀한다(교과서 그대로). 분할이 한쪽으로 쏠리면 깊이가 n까지 간다. */
static void quickLomuto(SortCtx *c, size_t lo, size_t hi, size_t depth, unsigned *rng) {
    sortNoteDepth(c, depth);
    if (hi - lo < 2) {
        return;
    }
    size_t pivot = lo;
    if (quickSortScheme == QUICK_RANDOM_LOMUTO) {
        pivot = lo + nextRandom(rng) % (hi - lo);
    }
    size_t p = partitionLomuto(c, lo, hi, pivot);
    quickLomuto(c, lo, p, depth + 1, rng);
    quickLomuto(c, p + 1, hi, depth + 1, rng);
}

void quickSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    SortCtx c;
    if (!sortBegin(&c, base, n, size, cmp, stats)) {
        return;
    }
    if (quickSortScheme == QUICK_MEDIAN3_HOARE) {
        quickHoare(&c, 0, n, 1);
    } else {
        unsigned rng = 20260930u;
        quickLomuto(&c, 0, n, 1, &rng);
    }
    sortEnd(&c);
}
