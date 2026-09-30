/* Timsort — 이 과제의 "배우지 않은 정렬".
 *
 * 2002년 Tim Peters가 Python의 list.sort()를 위해 만든 하이브리드 정렬이다.
 * 병합 정렬과 삽입 정렬을 섞었는데, 핵심 아이디어는 하나다:
 *
 *   **실제 데이터는 이미 부분부분 정렬되어 있는 경우가 많다. 그것을 버리지 말자.**
 *
 * 병합 정렬은 입력을 무조건 반으로 쪼갠다. 이미 정렬된 배열이 와도 똑같이
 * 쪼개고 똑같이 합친다. Timsort는 먼저 배열을 훑어 **이미 정렬된 구간(run)**을
 * 찾고, 그 run들을 병합한다. 그래서 정렬된 입력은 비교 n-1번으로 끝난다.
 *
 * 이 파일이 하는 일, 순서대로:
 *   1. run 찾기     오름차순 구간은 그대로, 엄격한 내림차순 구간은 뒤집어서 쓴다
 *   2. minrun       run이 너무 짧으면 이진 삽입 정렬로 minrun(32~64)까지 늘린다
 *   3. run 스택     run을 스택에 쌓으며, 길이가 균형을 이루도록 규칙대로 병합한다
 *   4. 병합         양 끝의 "이미 제자리인" 원소를 이분 탐색으로 잘라 낸 뒤,
 *                   **더 짧은 쪽만** 보조 배열로 복사해 병합한다
 *
 * 수업용으로 줄인 판이다. CPython · Java의 Timsort에 있는 **galloping 모드**
 * (한쪽 run에서 연달아 원소가 나오면 지수 탐색으로 건너뛰는 것)는 넣지 않았다.
 * 4번의 "양 끝 잘라 내기"는 넣었으나, 원본처럼 지수 탐색이 아니라 이분 탐색이다.
 */
#include "sort.h"

#include <stdlib.h>

#include "sortctx.h"

/* 이보다 짧은 배열은 run 병합 없이 이진 삽입 정렬 하나로 끝낸다. */
#define TIM_MIN_MERGE 64
/* run 스택의 칸 수. 병합 규칙이 run 길이를 피보나치 수처럼 자라게 만들므로
 * 2^64개 원소라도 85칸이면 넘치지 않는다 (Java TimSort와 같은 값). */
#define TIM_MAX_RUNS 85

typedef struct TimState {
    SortCtx *c;
    char *buf;     /* 병합용 보조 배열. 짧은 쪽 run만 담으므로 최대 n/2칸 */
    size_t bufCap; /* buf의 칸 수 */
    size_t runBase[TIM_MAX_RUNS];
    size_t runLen[TIM_MAX_RUNS];
    size_t stackSize;
} TimState;

/* --- 1. run 찾기 -------------------------------------------------------- */

static void reverseRange(SortCtx *c, size_t lo, size_t hi) {
    while (lo + 1 < hi) {
        sortSwap(c, lo, hi - 1);
        lo++;
        hi--;
    }
}

/* a[lo]부터 시작하는 run의 길이를 돌려준다. 내림차순이면 뒤집어서 오름차순으로 만든다.
 *
 * 내림차순은 **엄격히 작아질 때만** 이어 간다('<'). 같은 값이 섞인 구간을
 * 뒤집으면 같은 값의 앞뒤가 바뀌어 안정성이 깨지기 때문이다.
 * 오름차순은 같은 값을 허용한다('>='). 뒤집지 않으니 순서가 그대로다. */
static size_t countRunAndMakeAscending(SortCtx *c, size_t lo, size_t hi) {
    size_t runHi = lo + 1;
    if (runHi == hi) {
        return 1;
    }
    if (sortCompareAt(c, runHi, lo) < 0) {
        runHi++;
        while (runHi < hi && sortCompareAt(c, runHi, runHi - 1) < 0) {
            runHi++;
        }
        reverseRange(c, lo, runHi);
    } else {
        runHi++;
        while (runHi < hi && sortCompareAt(c, runHi, runHi - 1) >= 0) {
            runHi++;
        }
    }
    return runHi - lo;
}

/* --- 2. 짧은 run 늘리기 — 이진 삽입 정렬 ------------------------------------ */

/* a[lo..start)는 이미 정렬되어 있다. a[start..hi)를 하나씩 끼워 넣는다.
 * 넣을 자리는 이분 탐색으로 찾는다(비교 O(log n)). 이동은 여전히 O(n)이지만
 * minrun이 64 이하라 짧다. 같은 값 **뒤에** 넣으므로(upper bound) 안정하다. */
static void binaryInsertionSort(SortCtx *c, size_t lo, size_t start, size_t hi) {
    if (start == lo) {
        start++;
    }
    for (; start < hi; start++) {
        size_t left = lo;
        size_t right = start;
        while (left < right) { /* a[left..start) 중 a[start]보다 큰 첫 자리 */
            size_t mid = left + (right - left) / 2;
            if (sortCompareAt(c, mid, start) > 0) {
                right = mid;
            } else {
                left = mid + 1;
            }
        }
        if (left == start) {
            continue; /* 이미 제자리다 */
        }
        sortMove(c, c->tmp, sortElemAt(c, start));
        for (size_t j = start; j > left; j--) {
            sortMove(c, sortElemAt(c, j), sortElemAt(c, j - 1));
        }
        sortMove(c, sortElemAt(c, left), c->tmp);
    }
}

/* n이 64 이상이면 32~64 사이의 값을 돌려준다. n의 상위 6비트를 쓰고, 버린
 * 비트 중 하나라도 1이면 1을 더한다. 이렇게 고르면 n / minrun이 2의 거듭제곱과
 * 같거나 조금 작아져, 마지막 병합들이 비슷한 길이끼리 이루어진다. */
static size_t minRunLength(size_t n) {
    size_t r = 0;
    while (n >= TIM_MIN_MERGE) {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}

/* --- 4. 병합 ------------------------------------------------------------ */

/* a[lo..hi) 중 key보다 **큰** 원소가 처음 나오는 자리 */
static size_t upperBound(SortCtx *c, size_t lo, size_t hi, const char *key) {
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (sortComparePtr(c, sortElemAt(c, mid), key) > 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return lo;
}

/* a[lo..hi) 중 key **이상**인 원소가 처음 나오는 자리 */
static size_t lowerBound(SortCtx *c, size_t lo, size_t hi, const char *key) {
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (sortComparePtr(c, sortElemAt(c, mid), key) >= 0) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    return lo;
}

static int ensureBuffer(TimState *s, size_t need) {
    if (s->bufCap >= need) {
        return 1;
    }
    free(s->buf);
    s->buf = (char *)malloc(need * s->c->size);
    s->bufCap = (s->buf != NULL) ? need : 0;
    sortNoteExtra(s->c, s->bufCap * s->c->size); /* 실제로 잡은 만큼만 센다 */
    return s->buf != NULL;
}

/* 왼쪽 run A가 짧을 때. A를 보조 배열에 옮겨 두고 **앞에서부터** 채운다. */
static void mergeLo(TimState *s, size_t base1, size_t len1, size_t base2, size_t len2) {
    SortCtx *c = s->c;
    const size_t sz = c->size;
    for (size_t k = 0; k < len1; k++) {
        sortMove(c, s->buf + k * sz, sortElemAt(c, base1 + k));
    }
    size_t i = 0;            /* buf 안의 A */
    size_t j = base2;        /* 배열 안의 B */
    size_t dest = base1;
    const size_t end2 = base2 + len2;
    while (i < len1 && j < end2) {
        /* B가 엄격히 작을 때만 B를 먼저 — 같으면 A가 먼저라 안정하다 */
        if (sortComparePtr(c, sortElemAt(c, j), s->buf + i * sz) < 0) {
            sortMove(c, sortElemAt(c, dest++), sortElemAt(c, j++));
        } else {
            sortMove(c, sortElemAt(c, dest++), s->buf + i++ * sz);
        }
    }
    while (i < len1) {
        sortMove(c, sortElemAt(c, dest++), s->buf + i++ * sz);
    }
    /* B가 남았다면 이미 제자리에 있다 */
}

/* 오른쪽 run B가 짧을 때. B를 보조 배열에 옮겨 두고 **뒤에서부터** 채운다. */
static void mergeHi(TimState *s, size_t base1, size_t len1, size_t base2, size_t len2) {
    SortCtx *c = s->c;
    const size_t sz = c->size;
    for (size_t k = 0; k < len2; k++) {
        sortMove(c, s->buf + k * sz, sortElemAt(c, base2 + k));
    }
    size_t i = base1 + len1; /* 배열 안의 A, 끝(미포함)에서부터 */
    size_t j = len2;         /* buf 안의 B, 끝(미포함)에서부터 */
    size_t dest = base2 + len2;
    while (i > base1 && j > 0) {
        /* 뒤에서 채우므로 반대로: A가 엄격히 클 때만 A를 뒤에 둔다 */
        if (sortComparePtr(c, s->buf + (j - 1) * sz, sortElemAt(c, i - 1)) < 0) {
            sortMove(c, sortElemAt(c, --dest), sortElemAt(c, --i));
        } else {
            sortMove(c, sortElemAt(c, --dest), s->buf + --j * sz);
        }
    }
    while (j > 0) {
        sortMove(c, sortElemAt(c, --dest), s->buf + --j * sz);
    }
}

/* 스택의 i번째와 i+1번째 run을 병합한다. */
static void mergeAt(TimState *s, size_t i) {
    SortCtx *c = s->c;
    size_t base1 = s->runBase[i];
    size_t len1 = s->runLen[i];
    size_t base2 = s->runBase[i + 1];
    size_t len2 = s->runLen[i + 1];

    s->runLen[i] = len1 + len2;
    if (i + 3 == s->stackSize) { /* 맨 위에서 셋째를 병합했으면 맨 위를 한 칸 내린다 */
        s->runBase[i + 1] = s->runBase[i + 2];
        s->runLen[i + 1] = s->runLen[i + 2];
    }
    s->stackSize--;

    /* A의 앞부분 중 B[0] 이하인 것은 이미 제자리다 → 잘라 낸다 */
    size_t k = upperBound(c, base1, base1 + len1, sortElemAt(c, base2));
    len1 -= k - base1;
    base1 = k;
    if (len1 == 0) {
        return; /* A 전체가 B[0] 이하 — 두 run이 이미 이어져 있다 */
    }
    /* B의 뒷부분 중 A의 마지막 이상인 것도 이미 제자리다 → 잘라 낸다 */
    len2 = lowerBound(c, base2, base2 + len2, sortElemAt(c, base1 + len1 - 1)) - base2;
    if (len2 == 0) {
        return;
    }

    size_t shorter = len1 <= len2 ? len1 : len2;
    if (!ensureBuffer(s, shorter)) {
        /* 메모리를 못 잡으면 느리지만 맞는 길로 간다 */
        binaryInsertionSort(c, base1, base2, base2 + len2);
        return;
    }
    if (len1 <= len2) {
        mergeLo(s, base1, len1, base2, len2);
    } else {
        mergeHi(s, base1, len1, base2, len2);
    }
}

/* --- 3. run 스택의 규칙 ---------------------------------------------------- */

/* 스택 위쪽 run 길이 X, Y, Z (Z가 맨 위)에 대해 다음이 늘 성립하도록 병합한다.
 *     X > Y + Z   그리고   Y > Z
 * 그러면 run 길이가 아래로 갈수록 피보나치 수열 이상으로 커져서, 스택 깊이가
 * O(log n)으로 묶이고 병합이 비슷한 길이끼리 일어난다(병합 정렬의 "반으로"와 같은 효과).
 *
 * 원래 Tim Peters의 규칙은 맨 위 셋만 봤는데, 2015년에 그것만으로는 불변식이
 * 깨질 수 있다는 것이 밝혀졌다. 그래서 넷째(runLen[n-2])까지 보는 수정판을 쓴다. */
static void mergeCollapse(TimState *s) {
    size_t *len = s->runLen;
    while (s->stackSize > 1) {
        size_t n = s->stackSize - 2;
        if ((n > 0 && len[n - 1] <= len[n] + len[n + 1]) ||
            (n > 1 && len[n - 2] <= len[n - 1] + len[n])) {
            if (len[n - 1] < len[n + 1]) {
                n--; /* 더 짧은 이웃과 병합한다 */
            }
        } else if (len[n] > len[n + 1]) {
            break; /* 불변식이 성립한다 */
        }
        mergeAt(s, n);
    }
}

/* 끝났으면 남은 run을 전부 병합한다. */
static void mergeForceCollapse(TimState *s) {
    size_t *len = s->runLen;
    while (s->stackSize > 1) {
        size_t n = s->stackSize - 2;
        if (n > 0 && len[n - 1] < len[n + 1]) {
            n--;
        }
        mergeAt(s, n);
    }
}

void timSort(void *base, size_t n, size_t size, SortCompare cmp, SortStats *stats) {
    SortCtx c;
    if (!sortBegin(&c, base, n, size, cmp, stats)) {
        return;
    }
    if (n < TIM_MIN_MERGE) {
        size_t run = countRunAndMakeAscending(&c, 0, n);
        binaryInsertionSort(&c, 0, run, n);
        sortEnd(&c);
        return;
    }

    TimState s;
    s.c = &c;
    s.buf = NULL;
    s.bufCap = 0;
    s.stackSize = 0;

    const size_t minRun = minRunLength(n);
    size_t lo = 0;
    while (lo < n) {
        size_t run = countRunAndMakeAscending(&c, lo, n);
        if (run < minRun) {
            size_t force = (n - lo < minRun) ? n - lo : minRun;
            binaryInsertionSort(&c, lo, lo + run, lo + force);
            run = force;
        }
        s.runBase[s.stackSize] = lo;
        s.runLen[s.stackSize] = run;
        s.stackSize++;
        mergeCollapse(&s);
        lo += run;
    }
    mergeForceCollapse(&s);

    free(s.buf);
    sortEnd(&c);
}
