"""SVG 그래프를 손으로 그리는 아주 작은 도구 — 표준 모듈만 쓴다.

이 저장소는 외부 라이브러리를 쓰지 않는다(matplotlib 없음). SVG는 그냥
텍스트라서 필요한 만큼만 직접 찍어 내면 된다. 덤으로 diff도 읽힌다.

좌표는 SVG 관례대로 왼쪽 위가 (0, 0)이고 y는 아래로 자란다.
"""

import math

# 배경을 밝게 깔아 GitHub의 라이트/다크 어느 쪽에서도 읽히게 한다.
BG = "#fbfbf9"
INK = "#24292f"      # 본문
MUTED = "#6e7781"    # 눈금 · 보조선
GRID = "#dfe1e4"

# Okabe-Ito 색약 안전 팔레트. 선 모양까지 달리해 색만으로 구분하지 않게 한다.
SERIES_STYLE = {
    "quickSort":     ("#0072B2", "none"),      # 파랑 · 실선
    "mergeSort":     ("#E69F00", "7 4"),       # 주황 · 파선
    "timSort":       ("#009E73", "2 3"),       # 초록 · 점선
    # 퀵 정렬 피벗 실험의 세 방식
    "first-lomuto":  ("#D55E00", "7 4"),       # 주홍 · 파선
    "random-lomuto": ("#CC79A7", "2 3"),       # 분홍 · 점선
    "median3-hoare": ("#0072B2", "none"),      # 파랑 · 실선 (= 기본값 quickSort)
}
EXTRA = [("#0072B2", "none"), ("#E69F00", "7 4"), ("#009E73", "2 3"),
         ("#8c4799", "4 2 1 2"), ("#CC79A7", "1 3")]
_assigned = {}

FONT = ("-apple-system, BlinkMacSystemFont, 'Segoe UI', 'Noto Sans KR', "
        "'Apple SD Gothic Neo', 'Malgun Gothic', sans-serif")


def style_for(name):
    """알고리즘 이름은 고정 색, 그 밖의 계열은 순서대로 돌려 쓴다."""
    if name in SERIES_STYLE:
        return SERIES_STYLE[name]
    if name not in _assigned:
        _assigned[name] = EXTRA[len(_assigned) % len(EXTRA)]
    return _assigned[name]


def esc(text):
    return (str(text).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def si(value):
    """3.2e7 대신 32M 으로 읽기 좋게."""
    for limit, suffix in ((1e9, "B"), (1e6, "M"), (1e3, "K")):
        if value >= limit:
            trimmed = value / limit
            text = f"{trimmed:.0f}" if trimmed >= 10 else f"{trimmed:.1f}"
            if "." in text:  # 소수점 뒤의 0만 떼어 낸다. "10"을 "1"로 만들면 안 된다
                text = text.rstrip("0").rstrip(".")
            return text + suffix
    if value >= 1:
        return f"{value:.0f}"
    return f"{value:g}"


def ms(value):
    """시간(ms) 막대에 붙일 라벨. 0.004와 27.9를 한 축에서 함께 읽혀야 한다."""
    if value >= 10:
        return f"{value:.0f}"
    if value >= 1:
        return f"{value:.1f}"
    return f"{value:.3f}".rstrip("0").rstrip(".")


def nbytes(value):
    """800008 대신 781 KB 처럼. 메모리 막대에 붙인다."""
    for limit, suffix in ((1 << 20, "MB"), (1 << 10, "KB")):
        if value >= limit:
            v = value / limit
            return f"{v:.0f} {suffix}" if v >= 10 else f"{v:.1f} {suffix}"
    return f"{value:.0f} B"


def nice_ticks(hi, count=5):
    """0부터 hi 위까지, 사람이 읽기 좋은 간격으로 끊은 눈금."""
    if hi <= 0:
        return [0], 1
    raw = hi / count
    mag = 10 ** math.floor(math.log10(raw))
    step = next(m * mag for m in (1, 2, 2.5, 5, 10) if raw <= m * mag)
    top = math.ceil(hi / step) * step
    return [step * i for i in range(int(round(top / step)) + 1)], top


class Canvas:
    """SVG 조각을 차곡차곡 쌓아 두었다가 한 번에 파일로 쓴다."""

    def __init__(self, width, height, title, subtitle=None):
        self.w, self.h = width, height
        self.parts = []
        self.parts.append(
            f'<rect width="{width}" height="{height}" fill="{BG}"/>')
        self.text(20, 30, title, size=16, weight="600")
        if subtitle:
            self.text(20, 50, subtitle, size=11.5, fill=MUTED)

    def text(self, x, y, s, size=12, fill=INK, anchor="start", weight="400"):
        self.parts.append(
            f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" fill="{fill}" '
            f'text-anchor="{anchor}" font-weight="{weight}">{esc(s)}</text>')

    def line(self, x1, y1, x2, y2, stroke=GRID, width=1, dash="none"):
        self.parts.append(
            f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
            f'stroke="{stroke}" stroke-width="{width}" stroke-dasharray="{dash}"/>')

    def rect(self, x, y, w, h, fill):
        self.parts.append(
            f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" '
            f'fill="{fill}"/>')

    def polyline(self, points, stroke, dash="none", width=2.2):
        pts = " ".join(f"{x:.1f},{y:.1f}" for x, y in points)
        self.parts.append(
            f'<polyline points="{pts}" fill="none" stroke="{stroke}" '
            f'stroke-width="{width}" stroke-dasharray="{dash}" '
            f'stroke-linejoin="round" stroke-linecap="round"/>')

    def dot(self, x, y, fill, r=3.4):
        self.parts.append(
            f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r}" fill="{fill}"/>')

    def save(self, path):
        body = "\n  ".join(self.parts)
        svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" '
               f'height="{self.h}" viewBox="0 0 {self.w} {self.h}" '
               f'font-family="{FONT}">\n  {body}\n</svg>\n')
        with open(path, "w", encoding="utf-8") as f:
            f.write(svg)
        return path


def _legend(canvas, names, x, y):
    for name in names:
        color, dash = style_for(name)
        canvas.line(x, y - 4, x + 22, y - 4, stroke=color, width=2.4, dash=dash)
        canvas.dot(x + 11, y - 4, color, r=3)
        canvas.text(x + 28, y, name, size=11.5)
        x += 28 + len(name) * 6.6 + 18


def line_chart(path, title, subtitle, xs, series, xlabel, ylabel,
               log_axes=True, annotate_slope=None, vline=None, vline_label=""):
    """꺾은선. log_axes면 양쪽 축이 로그다.

    같은 자료를 두 축으로 그리면 서로 다른 것이 보인다. 선형은 **격차의 크기**를,
    로그-로그는 **자라는 속도**를 보여 준다(기울기가 곧 복잡도 지수).

    xs      : x 값 목록 (예: [1000, 2000, 4000, 8000])
    series  : {이름: [y 값, ...]}
    """
    W, H = 720, 432
    L, R, T, B = 78, 24, 86, 58
    c = Canvas(W, H, title, subtitle)
    if annotate_slope is None:
        annotate_slope = log_axes  # 기울기는 로그-로그에서만 뜻이 있다

    ys_all = [v for values in series.values() for v in values if v > 0]
    lo, hi = min(ys_all), max(ys_all)

    if log_axes:
        y0, y1 = math.floor(math.log10(lo)), math.ceil(math.log10(hi))
        x0, x1 = math.log10(min(xs)), math.log10(max(xs))
        yticks = [10 ** e for e in range(y0, y1 + 1)]

        def px(v):
            return L + (math.log10(v) - x0) / (x1 - x0) * (W - L - R)

        def py(v):
            return H - B - (math.log10(v) - y0) / (y1 - y0) * (H - T - B)
    else:
        yticks, top = nice_ticks(hi)
        x0, x1 = min(xs), max(xs)

        def px(v):
            return L + (v - x0) / (x1 - x0) * (W - L - R)

        def py(v):
            return H - B - v / top * (H - T - B)

    for t in yticks:
        y = py(t)
        c.line(L, y, W - R, y)
        c.text(L - 10, y + 4, si(t), size=10.5, fill=MUTED, anchor="end")
    # x축: 실제로 잰 n 값에만 눈금. 선형 축에서 작은 n끼리 붙으면 라벨은 건너뛴다.
    last_label_x = None
    for v in xs:
        x = px(v)
        c.line(x, T, x, H - B, dash="2 4")
        if last_label_x is None or x - last_label_x >= 48:
            c.text(x, H - B + 18, f"{v:,}", size=10.5, fill=MUTED, anchor="middle")
            last_label_x = x

    if vline is not None:
        x = px(vline)
        c.line(x, T, x, H - B, stroke=INK, width=1.4, dash="5 4")
        c.text(x, T - 4, vline_label, size=11, fill=INK, anchor="middle", weight="600")

    c.line(L, H - B, W - R, H - B, stroke=MUTED)
    c.line(L, T, L, H - B, stroke=MUTED)
    c.text((L + W - R) / 2, H - 14, xlabel, size=11.5, fill=MUTED, anchor="middle")
    c.text(20, T - 12, ylabel, size=11.5, fill=MUTED)

    used_label_y = []  # 기울기 라벨끼리 겹치지 않게 이미 쓴 높이를 기억한다
    for name, values in series.items():
        color, dash = style_for(name)
        points = [(px(x), py(v)) for x, v in zip(xs, values) if v > 0]
        c.polyline(points, color, dash)
        for x, y in points:
            c.dot(x, y, color)
        if annotate_slope and len(values) > 1 and values[0] > 0:
            slope = ((math.log10(values[-1]) - math.log10(values[0]))
                     / (math.log10(xs[-1]) - math.log10(xs[0])))
            ex, ey = points[-1]
            ly = ey - 11
            while any(abs(ly - u) < 14 for u in used_label_y):
                ly -= 14  # 선이 겹치면 라벨만 위로 비켜 쌓는다
            used_label_y.append(ly)
            c.text(ex - 6, ly, f"{name} 기울기 {slope:.2f}", size=11,
                   fill=color, anchor="end", weight="600")

    _legend(c, list(series), L, H - 30)
    return c.save(path)


def grouped_bar_chart(path, title, subtitle, groups, series, ylabel,
                      log_scale=False, value_label=si):
    """묶음 막대. groups는 x축 묶음 이름, series는 {이름: [묶음별 값]}."""
    # 묶음 수 × 계열 수가 많으면 막대 라벨끼리 부딪힌다. 막대 폭 30px을 지키도록 넓힌다.
    L, R, T, B = 78, 24, 86, 62
    W, H = max(720, int(L + R + len(groups) * (len(series) + 1.4) * 30)), 432
    c = Canvas(W, H, title, subtitle)

    values = [v for vs in series.values() for v in vs]
    hi = max(values)
    positive = [v for v in values if v > 0]
    lo = min(positive) if positive else 1

    if log_scale:
        y0 = math.floor(math.log10(lo))
        y1 = math.ceil(math.log10(hi))

        def py(v):
            v = max(v, 10 ** y0)
            return H - B - (math.log10(v) - y0) / (y1 - y0) * (H - T - B)

        ticks = [10 ** e for e in range(y0, y1 + 1)]
    else:
        ticks, y1 = nice_ticks(hi)

        def py(v):
            return H - B - v / y1 * (H - T - B)

    for t in ticks:
        y = py(t)
        c.line(L, y, W - R, y)
        c.text(L - 10, y + 4, si(t), size=10.5, fill=MUTED, anchor="end")

    c.line(L, H - B, W - R, H - B, stroke=MUTED)
    c.text(20, T - 12, ylabel, size=11.5, fill=MUTED)

    span = (W - L - R) / len(groups)
    names = list(series)
    bar_w = min(38, span / (len(names) + 1.4))
    for gi, group in enumerate(groups):
        center = L + span * (gi + 0.5)
        start = center - bar_w * len(names) / 2
        for si_, name in enumerate(names):
            color, _ = style_for(name)
            v = series[name][gi]
            top = py(v) if v > 0 else H - B
            x = start + bar_w * si_
            c.rect(x, top, bar_w - 3, max(H - B - top, 0.8), color)
            c.text(x + (bar_w - 3) / 2, top - 5, value_label(v), size=9.5,
                   fill=MUTED, anchor="middle")
        c.text(center, H - B + 18, group, size=11.5, anchor="middle")

    _legend(c, names, L, H - 26)
    return c.save(path)
