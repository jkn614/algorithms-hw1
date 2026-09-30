"""비교 결과를 그래프(SVG)로 그린다.

    make charts          # 또는
    python3 tools/plot.py

src/main.out 을 --csv · --quick · --disorder 로 돌려 측정값을 받아 report/ 아래에
SVG와 원본 CSV를 쓴다. 사람이 읽는 표를 파싱하지 않고 CSV를 쓰는 이유는, 표의
모양이 바뀌어도 그래프가 깨지지 않게 하려는 것이다.

표준 모듈만 쓴다. 그림은 tools/svgchart.py가 직접 찍어 낸다.
"""

import csv
import io
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import svgchart  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
BINARY = ROOT / "src" / "main.out"
OUT_DIR = ROOT / "report"
ALGOS = ["quickSort", "mergeSort", "timSort"]
SCHEMES = ["first-lomuto", "random-lomuto", "median3-hoare"]
KIND_KEYS = ["random", "sorted", "reversed", "nearly-sorted", "few-unique", "all-equal"]
KIND_LABEL = {
    "random": "무작위",
    "sorted": "정렬됨",
    "reversed": "역순",
    "nearly-sorted": "거의정렬",
    "few-unique": "중복많음",
    "all-equal": "모두같음",
}
INT_FIELDS = ("n", "compares", "moves", "extraBytes", "maxDepth", "swaps")


def run_csv(flag, save_as):
    """측정 프로그램을 돌려 CSV를 읽고, 같은 값을 report/에 남긴다."""
    if not BINARY.exists():
        subprocess.run(["make", "src/main.out"], cwd=ROOT, check=True)
    result = subprocess.run([str(BINARY), flag], cwd=ROOT, check=True,
                            capture_output=True, text=True)
    rows = list(csv.DictReader(io.StringIO(result.stdout)))
    # 그래프와 보고서의 표가 같은 실행에서 나오도록 측정값을 그대로 남긴다.
    with open(OUT_DIR / save_as, "w", encoding="utf-8", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    for row in rows:
        for key in INT_FIELDS:
            if key in row:
                row[key] = int(row[key])
        row["millis"] = float(row["millis"])
    return rows


def pick(rows, **conditions):
    return [r for r in rows if all(r[k] == v for k, v in conditions.items())]


def by_series(rows, series_field, names, field, keys, key_field):
    """{계열 이름: [키 순서대로의 값]} 으로 모은다."""
    table = {}
    for name in names:
        values = []
        for key in keys:
            match = [r for r in rows if r[series_field] == name and r[key_field] == key]
            values.append(match[0][field] if match else 0)
        table[name] = values
    return table


def main():
    OUT_DIR.mkdir(exist_ok=True)
    made = []

    # ------------------------------------------------------------------
    # A. 본 실험 (--csv)
    rows = run_csv("--csv", "results.csv")
    growth = pick(rows, scope="growth")
    kinds = pick(rows, scope="kinds")
    sizes = sorted({r["n"] for r in growth})
    n_kinds = kinds[0]["n"]

    # 1. n에 따라 자라는 모양. 선형은 격차의 크기를, 로그는 자라는 속도를 보여 준다.
    for field, unit, noun, stem in (
            ("compares", "비교 횟수", "비교 횟수", "growth-compares"),
            ("millis", "시간 (ms)", "걸린 시간", "growth-time")):
        data = by_series(growth, "algo", ALGOS, field, sizes, "n")
        made.append(svgchart.line_chart(
            OUT_DIR / f"{stem}.svg",
            f"n이 커질 때 {noun} — 선형 축",
            "무작위 입력 · 셋 다 O(n log n)이라 거의 곧게 자란다. 차이는 기울기가 아니라 상수배다",
            sizes, data, "n (원소 개수)", unit, log_axes=False))
        made.append(svgchart.line_chart(
            OUT_DIR / f"{stem}-log.svg",
            f"n이 커질 때 {noun} — 로그-로그 축",
            "기울기가 곧 복잡도 지수다 (2.0이면 n^2, 1.0에 가까우면 n log n)",
            sizes, data, "n (원소 개수)", unit))

    # 2. 입력 모양별 — 시간 · 비교 · 이동
    shape_labels = [KIND_LABEL[k] for k in KIND_KEYS]
    columns = (
        ("millis", "시간 (ms)", "input-shapes-time", "걸린 시간", svgchart.ms),
        ("compares", "비교 횟수", "input-shapes-compares", "비교 횟수", svgchart.si),
        ("moves", "이동 횟수", "input-shapes-moves", "이동 횟수", svgchart.si),
    )
    for field, unit, stem, label, fmt in columns:
        data = by_series(kinds, "algo", ALGOS, field, KIND_KEYS, "input")
        made.append(svgchart.grouped_bar_chart(
            OUT_DIR / f"{stem}.svg",
            f"입력 모양에 따른 {label} — 선형 축",
            f"n = {n_kinds:,} · 막대 높이가 곧 값의 비율이다. 대신 작은 값은 바닥에 붙는다",
            shape_labels, data, unit, value_label=fmt))
        made.append(svgchart.grouped_bar_chart(
            OUT_DIR / f"{stem}-log.svg",
            f"입력 모양에 따른 {label} — 로그 축",
            "같은 자료. 선형 축에서 사라졌던 작은 값이 여기서는 읽힌다",
            shape_labels, data, unit, log_scale=True, value_label=fmt))

    # 3. 추가 메모리 — 8 B와 800 KB를 한 축에 두려면 로그가 필요하다
    made.append(svgchart.grouped_bar_chart(
        OUT_DIR / "input-shapes-memory.svg",
        "입력 모양에 따른 추가 메모리 — 로그 축",
        f"n = {n_kinds:,} · 병합은 늘 n칸, Timsort는 병합할 run이 없으면 원소 한 칸뿐이다",
        shape_labels, by_series(kinds, "algo", ALGOS, "extraBytes", KIND_KEYS, "input"),
        "추가 메모리", log_scale=True, value_label=svgchart.nbytes))

    # 4. 재귀 깊이
    made.append(svgchart.grouped_bar_chart(
        OUT_DIR / "input-shapes-depth.svg",
        "입력 모양에 따른 재귀 깊이",
        f"n = {n_kinds:,} · Timsort는 반복문이라 늘 1이다. 퀵은 작은 쪽만 재귀해 log2(n) 이하",
        shape_labels, by_series(kinds, "algo", ALGOS, "maxDepth", KIND_KEYS, "input"),
        "재귀 깊이"))

    # ------------------------------------------------------------------
    # B. 퀵 정렬 피벗 실험 (--quick)
    quick = run_csv("--quick", "quick-schemes.csv")
    qsizes = sorted({r["n"] for r in quick})
    for kind, note in (
            ("sorted", "첫 원소 피벗만 무너진다 — 늘 최솟값을 골라 한쪽이 텅 빈다"),
            ("few-unique", "무작위 피벗으로도 못 막는다 — 문제는 피벗이 아니라 같은 값의 처리다"),
            ("all-equal", "Lomuto 둘 다 정확히 n(n-1)/2. Hoare는 같은 값에서 멈춰 반으로 가른다")):
        rows_k = pick(quick, input=kind)
        made.append(svgchart.line_chart(
            OUT_DIR / f"quick-{kind}.svg",
            f"퀵 정렬 피벗·분할 방식별 비교 횟수 — {KIND_LABEL[kind]} 입력",
            note, qsizes, by_series(rows_k, "scheme", SCHEMES, "compares", qsizes, "n"),
            "n (원소 개수)", "비교 횟수"))
    made.append(svgchart.line_chart(
        OUT_DIR / "quick-depth.svg",
        "퀵 정렬 피벗·분할 방식별 재귀 깊이 — 모두같음 입력",
        "로그-로그 · Lomuto는 깊이가 n 그대로다. n이 수만이면 스택이 넘친다",
        qsizes, by_series(pick(quick, input="all-equal"), "scheme", SCHEMES, "maxDepth",
                          qsizes, "n"),
        "n (원소 개수)", "재귀 깊이"))

    # ------------------------------------------------------------------
    # C. 흐트러짐 실험 (--disorder)
    disorder = run_csv("--disorder", "disorder.csv")
    swaps = sorted({r["swaps"] for r in disorder})
    swap_labels = [f"{s:,}번" for s in swaps]
    n_dis = disorder[0]["n"]
    for field, unit, stem, label, fmt in (
            ("compares", "비교 횟수", "disorder-compares", "비교 횟수", svgchart.si),
            ("millis", "시간 (ms)", "disorder-time", "걸린 시간", svgchart.ms)):
        made.append(svgchart.grouped_bar_chart(
            OUT_DIR / f"{stem}.svg",
            f"정렬된 배열을 흐트러뜨릴수록 — {label} (로그 축)",
            f"n = {n_dis:,} · 가로축은 두 자리를 맞바꾼 횟수. 왼쪽 끝이 정렬됨, 오른쪽 끝은 사실상 무작위",
            swap_labels, by_series(disorder, "algo", ALGOS, field, swaps, "swaps"),
            unit, log_scale=True, value_label=fmt))

    for path in made:
        print(f"wrote {Path(path).relative_to(ROOT)}")


if __name__ == "__main__":
    main()
