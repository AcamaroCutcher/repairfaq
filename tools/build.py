#!/usr/bin/env python3
"""
Build the repairfaq mirror + companion data.

    python3 -I build.py ARCHIVE_DIR SITE_DIR [--max-file-mb 25] [--skip-ext pdf,zip]

ARCHIVE_DIR  the local crawl (left untouched; only read)
SITE_DIR     the GitHub Pages repo. Writes:
               SITE_DIR/archive/...            pages + media, faithful copies
               SITE_DIR/companion/data/*.json  diagrams, search index, contributors, stats
               SITE_DIR/AI_DIAGRAM_GUIDE.md    is written separately (hand-maintained)

What changes in a mirrored HTML page (everything else is byte-for-byte the original):
  1. An HTML comment at the top naming the source URL      (required by the fair-use terms)
  2. A <link> to companion/assets/mirror-theme.css          (fonts and colors only: permitted)
  3. One line after <body> labeled "Source:" linking the original page (required)
  4. Links to files not hosted here (excluded media) point to the original repairfaq.org URL
"""
import argparse, hashlib, html, json, os, posixpath, re, shutil, sys
from urllib.parse import quote, unquote, urljoin, urlsplit

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from classify import score  # noqa: E402

ORIGIN = "https://www.repairfaq.org/"
HTML_EXT = (".htm", ".html")
SKIP_DIRS = {"cgi-bin"}
SKIP_FILES = {"_state.json", "_state.json.tmp"}

LINK_RE = re.compile(
    rb"""(?P<pre>\b(?:href|src|background|lowsrc)\s*=\s*)"""
    rb"""(?:"(?P<dq>[^"]*)"|'(?P<sq>[^']*)'|(?P<uq>[^\s>"']+))""", re.I)


def original_url(rel):
    if rel == "index.html":
        return ORIGIN
    if rel.endswith("/index.html"):
        return ORIGIN + quote(rel[: -len("index.html")])
    return ORIGIN + quote(rel)


# ---------------------------------------------------------------- mirror

def list_archive(arch):
    out = []
    for dp, dns, fns in os.walk(arch):
        rel_dir = os.path.relpath(dp, arch)
        dns[:] = [d for d in dns if not (rel_dir == "." and d in SKIP_DIRS)]
        for fn in fns:
            if fn in SKIP_FILES or "@" in fn:
                continue
            rel = os.path.normpath(os.path.join(rel_dir, fn)).replace(os.sep, "/")
            out.append(rel)
    return sorted(out)


def decide_media(arch, files, max_file, skip_ext, photo_dirs):
    """hosted: main site; photos: the photos site; excluded: linked to repairfaq.org."""
    hosted, photos, excluded = set(), set(), set()
    for rel in files:
        if rel.lower().endswith(HTML_EXT):
            hosted.add(rel)
            continue
        ext = posixpath.splitext(rel)[1].lower().lstrip(".")
        size = os.path.getsize(os.path.join(arch, rel))
        if ext in skip_ext or size > max_file:
            excluded.add(rel)
        elif ext in ("jpg", "jpeg") and any(rel.startswith(d + "/") for d in photo_dirs):
            photos.add(rel)
        else:
            hosted.add(rel)
    return hosted, photos, excluded


def mirror_page(body, rel, hosted, excluded, photos=(), photos_base=""):
    depth = rel.count("/")
    up = "../" * (depth + 1)            # from archive/<rel> back to SITE_DIR
    src = original_url(rel)
    here = posixpath.dirname(rel)

    def fix_link(m):
        raw = m.group("dq") or m.group("sq") or m.group("uq") or b""
        val = raw.decode("latin-1").strip()
        if not val or re.match(r"^(#|[a-z][a-z0-9+.-]*:)", val, re.I):
            return m.group(0)
        path = val.split("#", 1)[0].split("?", 1)[0]
        frag = val[len(path):]
        if path.startswith("/"):
            target = posixpath.normpath(unquote(path).lstrip("/")) or "index.html"
        else:
            target = posixpath.normpath(posixpath.join(here, unquote(path))) if path else rel
        if target.startswith("../") or target == "..":
            return m.group(0)
        if target.endswith("/") or (path.endswith("/") and not target.endswith(".html")):
            target = target.rstrip("/") + "/index.html"
        if target in photos and photos_base:
            new = photos_base + quote(target) + frag
            return m.group("pre") + b'"' + new.encode("latin-1", "replace") + b'"'
        if target in excluded or (target not in hosted and target not in photos):
            # not hosted here (excluded, or missing from the crawl): point at the original on repairfaq.org
            new = original_url(target) + frag
            return m.group("pre") + b'"' + new.encode("latin-1", "replace") + b'"'
        if path.startswith("/"):
            # a root-relative link would point outside this project site: make it relative
            new = posixpath.relpath(target, here or ".") + frag
            return m.group("pre") + b'"' + quote(new, safe="/#%._-~").encode("latin-1") + b'"'
        return m.group(0)

    body = LINK_RE.sub(fix_link, body)

    comment = (
        f"<!--\n  Mirrored from {src}\n"
        "  Sci.Electronics.Repair FAQ / Sam's Laser FAQ / Silicon Sam's Technology Resource.\n"
        "  Copyright (c) Samuel M. Goldwasser and Filip M. Gieszczykiewicz, and the contributing authors\n"
        "  where marked (From: ...). All rights reserved. Reproduced unaltered and free of charge under the\n"
        f"  fair-use terms at {ORIGIN}sam/copyright.htm. The original site: {ORIGIN}\n"
        "  Only fonts and colors are restyled, as those terms permit.\n-->\n"
    ).encode("latin-1")
    css = f'<link rel="stylesheet" href="{up}companion/assets/mirror-theme.css">\n'.encode()
    line = (
        f'<div class="rfm-source"><small>Source: <a href="{html.escape(src)}">{html.escape(src)}</a>'
        f' &middot; Original site: <a href="{ORIGIN}">repairfaq.org</a>'
        f' &middot; Unaltered mirror with a <a href="{up}index.html">companion guide</a></small></div>\n'
    ).encode()

    m = re.match(rb"\s*<!doctype[^>]*>\s*", body, re.I)
    if m:
        body = body[: m.end()] + comment + body[m.end():]
    else:
        body = comment + body

    hm = re.search(rb"</head\s*>", body, re.I)
    if hm:
        body = body[: hm.start()] + css + body[hm.start():]
    bm = re.search(rb"<body[^>]*>", body, re.I)
    if bm:
        body = body[: bm.end()] + b"\n" + line + body[bm.end():]
    else:
        # no <head>/<body>: put both right after the comment
        i = body.find(comment) + len(comment)
        body = body[:i] + css + line + body[i:]
    return body


# ---------------------------------------------------------------- extraction

def decode(raw):
    m = re.search(rb"charset=[\"']?([A-Za-z0-9_-]+)", raw[:4000], re.I)
    enc = (m.group(1).decode().lower() if m else "") or "cp1252"
    if enc in ("iso-8859-1", "latin-1", "us-ascii"):
        enc = "cp1252"
    try:
        return raw.decode(enc)
    except (LookupError, UnicodeDecodeError):
        try:
            return raw.decode("utf-8")
        except UnicodeDecodeError:
            return raw.decode("cp1252", "replace")


def text_of(fragment):
    t = re.sub(r"<[^>]+>", " ", fragment)
    return re.sub(r"\s+", " ", html.unescape(t)).strip()


def collection_of(rel, title):
    t = title.lower()
    if rel.startswith("sam/laser") or "laser faq" in t:
        return "laser"
    if rel.startswith(("sam/reppic/", "sam/laserpic/", "sam/3-D/", "sam/brochures/")):
        return "gallery"
    if rel.startswith(("REPAIR/", "samnew/")) or "sci.electronics.repair" in t or "repair faq" in t:
        return "repair"
    if rel.startswith("sam/"):
        return "sam"
    return "other"


ANCHOR_RE = re.compile(r"<a\s[^>]*name\s*=\s*[\"']?([^\"'\s>]+)", re.I)
HEAD_RE = re.compile(r"<h([1-4])[^>]*>(.*?)</h\1\s*>", re.I | re.S)
PRE_RE = re.compile(r"<pre[^>]*>(.*?)</pre\s*>", re.I | re.S)
FROM_RE = re.compile(r"\(From:\s*([^()<>\n]{3,60}?)\s*(?:\(|\)|<|$)", re.M)


def chunks_of_pre(text):
    """Split a <pre> block into blank-line separated chunks, merging diagram pieces."""
    text = text.expandtabs(8).replace("\r", "")
    parts, cur, blanks = [], [], 0
    for line in text.split("\n"):
        if line.strip():
            if blanks >= 1 and cur:
                parts.append(("\n".join(cur), blanks))
                cur = []
            cur.append(line.rstrip())
            blanks = 0
        else:
            blanks += 1
    if cur:
        parts.append(("\n".join(cur), 0))
    out = []
    for chunk, _ in parts:
        kind, _f = score(chunk)
        out.append([kind, chunk])
    # merge: diagram + short label chunk(s) right next to it
    merged = []
    for kind, chunk in out:
        if merged and merged[-1][0] == "diagram" and (
                kind == "diagram" or (len(chunk.split("\n")) <= 2 and len(chunk) < 160)):
            merged[-1][1] += "\n\n" + chunk
            continue
        merged.append([kind, chunk])
    return [c for k, c in merged if k == "diagram"]


NOTICE_END = re.compile(r"costs?\s+of\s+copying[^.]*\.", re.I)


def notice_of(s):
    """The document's own 'Author and Copyright' section, verbatim as text (or '')."""
    hits = [m.start() for m in re.finditer(r"Author\s+and\s+Copyright", s, re.I)]
    for start in reversed(hits):
        end = NOTICE_END.search(s, start, start + 6000)
        if end:
            frag = s[start:end.end()]
            frag = re.sub(r"<(br|p|li|h\d)[^>]*>", "\n", frag, flags=re.I)
            t = html.unescape(re.sub(r"<[^>]+>", "", frag))
            t = re.sub(r"[ \t]+", " ", t)
            t = re.sub(r"\s*\n\s*", "\n", t).strip()
            if "Copyright" in t:
                return t
    return ""


def header_line(s):
    """The short copyright line most chapters carry at the top."""
    b = s.lower().find("<body")
    head = text_of(s[max(b, 0):][:6000])
    m = re.search(r"[^.]{0,80}Copyright\s*(?:&copy;?|©|\(c\))?\s*\d{4}.{0,160}?Rights Reserved\.?", head, re.I)
    return m.group(0).strip() if m else ""


def extract(rel, raw):
    s = decode(raw)
    notice, hline = notice_of(s), header_line(s)
    tm = re.search(r"<title[^>]*>(.*?)</title", s, re.I | re.S)
    title = text_of(tm.group(1)) if tm else posixpath.basename(rel)
    coll = collection_of(rel, title)

    # ordered events: anchors, headings, pre blocks
    events = []
    for m in ANCHOR_RE.finditer(s):
        events.append((m.start(), "a", m.group(1)))
    for m in HEAD_RE.finditer(s):
        events.append((m.start(), "h", text_of(m.group(2))))
    for m in PRE_RE.finditer(s):
        events.append((m.start(), "pre", m))
    events.sort(key=lambda e: e[0])

    anchor, heading, headings, diagrams = "", title, [], []
    last_end = 0
    for pos, kind, val in events:
        if kind == "a":
            anchor = val
        elif kind == "h":
            if val:
                heading = val
                if coll != "gallery":
                    headings.append([val[:140], anchor])
        else:
            m = val
            if m.start() < last_end:
                continue
            body = html.unescape(re.sub(r"<[^>]+>", "", m.group(1)))
            before = text_of(s[max(last_end, m.start() - 1500): m.start()])[-320:]
            last_end = m.end()
            for d in chunks_of_pre(body):
                lines = d.split("\n")
                indent = min(len(l) - len(l.lstrip()) for l in lines if l.strip())
                d = "\n".join(l[indent:] for l in lines)
                diagrams.append({"ascii": d, "heading": heading, "anchor": anchor,
                                 "context": before})
    contributors = [c.strip(" .,;") for c in FROM_RE.findall(text_of(s.replace("<", " <")))]
    return title, coll, headings, diagrams, contributors, notice, hline


# Used only when a page has no "Author and Copyright" section of its own: these are the site-wide
# terms, quoted from archive/sam/copyright.htm.
GENERIC_NOTICE = (
    "Unless otherwise specified and not marked with (From xxxxxx), all the text in the FAQs and "
    "contained on this site is Copyright (c) 1992-2021 by Filip M. Gieszczykiewicz "
    "https://www.repairfaq.org/filipg/ and/or by Samuel M. Goldwasser https://www.repairfaq.org/sam/. "
    "All rights reserved.\n"
    "Reproduction of the document in whole or in part is permitted if the following conditions are "
    "satisfied: This [HTML Comment] notice is included in its entirety. The [Hardcopy] Copyright section "
    "is included in its entirety. There is no charge except to cover the costs of copying and distribution."
)

LABEL_RE = re.compile(r"\b([A-Z]{1,3}\d{1,4}[A-Z]?|\d+(?:\.\d+)?\s?(?:[kKmMuµnp]?(?:V|A|W|F|H|Hz|ohm|Ohm))|[A-Za-z]{3,})\b")
PARTS = [
    ("resistor", r"/\\/\\|\\/\\/|\bR\d+"), ("capacitor", r"_\|_\s*\n\s*-{3}|\|\|\|?|\bC\d+"),
    ("diode", r"\|<|>\||-\|>|<\|-|\bD\d+|\bCR\d+"), ("transistor", r"\|/|\|\\|\bQ\d+"),
    ("transformer", r"\)\|\(|\)::\(|::|\bT\d+"), ("inductor", r"\bL\d+|\)\)\)|ooo"),
    ("ground", r"_\|_|GND|[Gg]round"), ("lamp/tube", r"[Tt]ube|[Ll]amp|HeNe|CRT"),
    ("switch", r"o/ o|\bSW\d+|[Ss]witch"), ("fuse", r"\bF\d+\b|[Ff]use"), ("ic", r"\bU\d+|\bIC\d+"),
    ("battery", r"[Bb]attery|\bB\d+\b"), ("motor", r"[Mm]otor"), ("led", r"\bLED"),
]


def describe(ascii_):
    parts = [name for name, pat in PARTS if re.search(pat, ascii_)]
    refs = sorted(set(re.findall(r"\b(?:[RCDQLTUFKJ]|SW|IC|CR|VR|ZD)\d{1,3}\b", ascii_)))
    return parts, refs


# ---------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("archive")
    ap.add_argument("site")
    ap.add_argument("--max-file-mb", type=float, default=25)
    ap.add_argument("--skip-ext", default="")
    ap.add_argument("--data-only", action="store_true")
    ap.add_argument("--photo-dirs", default="", help="comma list of archive folders whose JPGs go to the photos site")
    ap.add_argument("--photos-site", default="", help="local folder of the photos repo")
    ap.add_argument("--photos-base", default="", help="public URL of the photos repo's archive/ folder, ending in /")
    a = ap.parse_args()
    arch, site = a.archive, a.site
    skip_ext = {e.strip().lower() for e in a.skip_ext.split(",") if e.strip()}

    files = list_archive(arch)
    photo_dirs = [d.strip().strip("/") for d in a.photo_dirs.split(",") if d.strip()]
    hosted, photos, excluded = decide_media(arch, files, a.max_file_mb * 1e6, skip_ext, photo_dirs)
    out_root = os.path.join(site, "archive")

    pages, diagrams, contributors, seen_hash = [], [], {}, {}
    page_notice = {}
    total = 0
    priority = lambda r: (0 if r.startswith("sam/") else 1 if r.startswith("samnew/") else 2, r)
    for rel in sorted(files, key=priority):
        src = os.path.join(arch, rel)
        dst = os.path.join(out_root, rel)
        is_html = rel.lower().endswith(HTML_EXT)
        if rel in hosted:
            total += os.path.getsize(src)
            if not a.data_only:
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                if is_html:
                    raw = open(src, "rb").read()
                    open(dst, "wb").write(mirror_page(raw, rel, hosted, excluded, photos, a.photos_base))
                elif not (os.path.exists(dst) and os.path.getsize(dst) == os.path.getsize(src)):
                    shutil.copy2(src, dst)
        elif rel in photos and a.photos_site and not a.data_only:
            pdst = os.path.join(a.photos_site, "archive", rel)
            if not (os.path.exists(pdst) and os.path.getsize(pdst) == os.path.getsize(src)):
                os.makedirs(os.path.dirname(pdst), exist_ok=True)
                shutil.copy2(src, pdst)
        if not is_html:
            continue
        raw = open(src, "rb").read()
        title, coll, headings, diags, contribs, notice, hline = extract(rel, raw)
        page_notice[rel] = (notice, hline)
        pages.append({"p": rel, "t": title, "c": coll, "h": headings})
        for name in contribs:
            if 3 <= len(name) <= 50 and not re.search(r"@|http|\d{3}", name):
                contributors[name] = contributors.get(name, 0) + 1
        for i, d in enumerate(diags):
            key = hashlib.sha1(re.sub(r"\s+", "", d["ascii"]).encode()).hexdigest()[:12]
            if key in seen_hash:
                diagrams[seen_hash[key]]["also"].append(rel)
                continue
            parts, refs = describe(d["ascii"])
            seen_hash[key] = len(diagrams)
            diagrams.append({
                "id": key, "page": rel, "title": title, "collection": coll,
                "heading": d["heading"], "anchor": d["anchor"], "context": d["context"],
                "parts": parts, "refs": refs, "ascii": d["ascii"],
                "source": original_url(rel) + (("#" + d["anchor"]) if d["anchor"] else ""),
                "also": [],
            })

    # notices: each diagram's page -> its document's Author and Copyright section
    def parent_doc(rel):
        if rel.startswith("sam/laser") and rel != "sam/laserfaq.htm":
            return "sam/laserfaq.htm"
        return None
    notices = {}
    for d in diagrams:
        own, hline = page_notice.get(d["page"], ("", ""))
        text = own or page_notice.get(parent_doc(d["page"]) or "", ("", ""))[0]
        src_note = "own" if own else "parent" if text else "site"
        text = text or GENERIC_NOTICE
        if hline and hline not in text:
            text = hline + "\n\n" + text
        k = hashlib.sha1(text.encode()).hexdigest()[:10]
        notices[k] = text
        d["notice"] = k
        d["notice_from"] = src_note

    data = os.path.join(site, "companion", "data")
    os.makedirs(data, exist_ok=True)
    json.dump(diagrams, open(os.path.join(data, "diagrams.json"), "w"), separators=(",", ":"))
    json.dump(notices, open(os.path.join(data, "notices.json"), "w"), indent=0)
    json.dump(pages, open(os.path.join(data, "search.json"), "w"), separators=(",", ":"))
    contribs = sorted(contributors.items(), key=lambda kv: (-kv[1], kv[0].lower()))
    json.dump(contribs, open(os.path.join(data, "contributors.json"), "w"), separators=(",", ":"))
    imgs = sum(1 for f in hosted | photos if f.lower().endswith((".gif", ".jpg", ".jpeg", ".png")))
    stats = {
        "pages": len(pages), "diagrams": len(diagrams), "images": imgs,
        "contributors": len(contribs), "hosted_files": len(hosted), "hosted_mb": round(total / 1e6),
        "photo_files": len(photos),
        "photo_mb": round(sum(os.path.getsize(os.path.join(arch, f)) for f in photos) / 1e6),
        "excluded_files": len(excluded),
        "excluded_mb": round(sum(os.path.getsize(os.path.join(arch, f)) for f in excluded) / 1e6),
    }
    json.dump(stats, open(os.path.join(data, "stats.json"), "w"), indent=1)
    print(json.dumps(stats, indent=1))


if __name__ == "__main__":
    main()
