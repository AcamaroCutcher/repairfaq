import re, html

WIRE_PAT = re.compile(r"(\+-|-\+|-o|o-|\|\s*$|/\\/\\|_\|_|-\|\(|\)\|-|-\|<|>\|-|\|--|--\|)")

def score(text):
    lines = text.expandtabs(8).split("\n")
    lines = [l.rstrip() for l in lines]
    nonblank = [l for l in lines if l.strip()]
    if len(nonblank) < 3:
        return "text", {}
    # vertical bars aligned in consecutive lines
    vert = 0
    for i in range(1, len(lines)):
        a, b = lines[i-1], lines[i]
        for j, ch in enumerate(b):
            if ch in "|+" and j < len(a) and a[j] in "|+.":
                vert += 1
    corners = len(re.findall(r"\+-|-\+", text))
    zig = len(re.findall(r"/\\/|\\/\\", text))
    gnd = text.count("_|_")
    hwire = len(re.findall(r"-{3,}", text))
    sep_lines = sum(1 for l in nonblank if re.fullmatch(r"\s*[-=_ ]{10,}\s*", l))
    chars = sum(len(l.strip()) for l in nonblank)
    alpha = sum(c.isalpha() for c in text)
    words = len(re.findall(r"[A-Za-z]{4,}", text))
    feats = dict(vert=vert, corners=corners, zig=zig, gnd=gnd, hwire=hwire,
                 sep=sep_lines, alpha=alpha/max(chars,1), words=words, lines=len(nonblank))
    s = vert*1.0 + corners*1.5 + zig*3 + gnd*3
    if s >= 6 and (vert >= 2 or zig or gnd) and feats["alpha"] < 0.55:
        return "diagram", feats
    if sep_lines and len(nonblank) >= 3:
        return "table", feats
    return "text", feats

def pre_blocks(src):
    for m in re.finditer(r"<pre[^>]*>(.*?)</pre>", src, re.S | re.I):
        raw = m.group(1)
        txt = html.unescape(re.sub(r"<[^>]+>", "", raw))
        yield m, txt
