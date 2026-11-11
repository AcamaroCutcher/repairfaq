#!/usr/bin/env python3
"""
Polite, resumable website mirror (stdlib only).

Usage:
    python3 -I mirror.py crawl   OUT_DIR [--workers 4] [--delay 0.5]
    python3 -I mirror.py rewrite OUT_DIR     # make links work offline
    python3 -I mirror.py status  OUT_DIR

Crawl can be stopped (Ctrl-C) and re-run; it resumes from OUT_DIR/_state.json.
"""
import json, os, posixpath, re, sys, threading, time, queue, argparse
import urllib.request, urllib.error
from urllib.parse import urljoin, urlsplit, urlunsplit, quote, unquote

START = "https://www.repairfaq.org/"
CANON_HOST = "www.repairfaq.org"
HOSTS = {"www.repairfaq.org", "repairfaq.org", "www.lasersam.org", "lasersam.org"}
UA = "Mozilla/5.0 (compatible; personal-archive-mirror/1.0)"
MAX_BYTES = 500 * 1024 * 1024  # skip any single file larger than this

LINK_RE = re.compile(
    rb"""(?P<pre>\b(?:href|src|background|lowsrc|data|action)\s*=\s*)"""
    rb"""(?:"(?P<dq>[^"]*)"|'(?P<sq>[^']*)'|(?P<uq>[^\s>"']+))""",
    re.I,
)
HTML_EXT = (".htm", ".html", ".shtml", ".php", ".asp", "")


def normalize(url):
    """Return canonical in-scope URL (no fragment) or None if out of scope."""
    try:
        p = urlsplit(url.strip())
    except ValueError:
        return None
    if p.scheme not in ("http", "https"):
        return None
    host = (p.hostname or "").lower()
    if host not in HOSTS:
        return None
    path = p.path or "/"
    path = quote(unquote(path), safe="/~%:@!$&'()*+,;=-._")
    return urlunsplit(("https", CANON_HOST, path, p.query, ""))


def local_path(url, is_html=False):
    p = urlsplit(url)
    path = unquote(p.path)
    if path.endswith("/"):
        path += "index.html"
    elif is_html and not posixpath.splitext(path)[1]:
        path += "/index.html"
    if p.query:
        base, ext = posixpath.splitext(path)
        q = re.sub(r"[^A-Za-z0-9._=-]", "_", p.query)[:100]
        path = f"{base}@{q}{ext}"
    path = path.lstrip("/")
    # avoid path traversal / weird segments
    parts = [s for s in path.split("/") if s not in ("", ".", "..")]
    return "/".join(parts) or "index.html"


def extract_links(body, base_url):
    for m in LINK_RE.finditer(body):
        raw = m.group("dq") or m.group("sq") or m.group("uq") or b""
        val = raw.decode("latin-1").strip()
        if not val or val.startswith(("#", "mailto:", "javascript:", "news:")):
            continue
        yield urljoin(base_url, val)


class Crawler:
    def __init__(self, out, workers, delay):
        self.out = out
        self.workers = workers
        self.delay = delay
        self.state_file = os.path.join(out, "_state.json")
        self.lock = threading.Lock()
        self.q = queue.Queue()
        self.seen = set()
        self.done = {}      # url -> local path
        self.redirects = {} # url -> final url
        self.failed = {}    # url -> error
        self.html = set()   # urls that are html
        self.stop = False
        self.count = 0
        self.bytes = 0
        self._load()

    def _load(self):
        os.makedirs(self.out, exist_ok=True)
        if os.path.exists(self.state_file):
            s = json.load(open(self.state_file))
            self.done = s["done"]
            self.redirects = s.get("redirects", {})
            self.failed = {u: e for u, e in s.get("failed", {}).items()
                           if u not in s["done"]}
            self.html = set(s.get("html", []))
            pending = s.get("pending", [])
            self.seen = set(self.done) | set(self.failed) | set(pending)
            for u in pending:
                self.q.put(u)
            print(f"Resuming: {len(self.done)} done, {len(pending)} pending")
        else:
            self.enqueue(START)

    def rescan(self):
        """Re-read saved HTML and enqueue any links not yet seen."""
        before = self.q.qsize()
        for page_url in list(self.html):
            lp = self.done.get(page_url)
            fp = os.path.join(self.out, lp) if lp else None
            if fp and os.path.exists(fp):
                for link in extract_links(open(fp, "rb").read(), page_url):
                    self.enqueue(link)
        print(f"Rescan queued {self.q.qsize() - before} new URLs")

    def save(self):
        with self.lock:
            pending = list(self.q.queue)
            s = {"done": dict(self.done), "redirects": dict(self.redirects),
                 "failed": dict(self.failed), "html": sorted(self.html),
                 "pending": pending}
        tmp = self.state_file + ".tmp"
        with open(tmp, "w") as f:
            json.dump(s, f)
        os.replace(tmp, self.state_file)

    def enqueue(self, url):
        u = normalize(url)
        if not u:
            return
        with self.lock:
            if u in self.seen:
                return
            self.seen.add(u)
        self.q.put(u)

    def fetch(self, url):
        req = urllib.request.Request(url, headers={"User-Agent": UA})
        for attempt in range(3):
            try:
                with urllib.request.urlopen(req, timeout=60) as r:
                    final = normalize(r.geturl()) or url
                    ctype = r.headers.get("Content-Type", "")
                    size = int(r.headers.get("Content-Length") or 0)
                    if size > MAX_BYTES:
                        raise RuntimeError(f"too large ({size} bytes)")
                    return final, ctype, r.read()
            except urllib.error.HTTPError as e:
                if e.code in (404, 403, 410, 401):
                    raise
                err = e
            except Exception as e:
                err = e
            time.sleep(2 * (attempt + 1))
        raise err

    def worker(self):
        while not self.stop:
            try:
                url = self.q.get(timeout=5)
            except queue.Empty:
                if self.q.empty():
                    return
                continue
            try:
                final, ctype, body = self.fetch(url)
                is_html = "html" in ctype.lower()
                lp = local_path(final, is_html)
                dest = os.path.join(self.out, lp)
                if os.path.isdir(dest):
                    dest = os.path.join(dest, "index.html")
                    lp += "/index.html"
                os.makedirs(os.path.dirname(dest), exist_ok=True)
                with open(dest, "wb") as f:
                    f.write(body)
                with self.lock:
                    self.done[url] = lp
                    if final != url:
                        self.redirects[url] = final
                        self.done[final] = lp
                        self.seen.add(final)
                    if is_html:
                        self.html.add(final)
                    self.count += 1
                    self.bytes += len(body)
                    n = self.count
                if is_html:
                    for link in extract_links(body, final):
                        self.enqueue(link)
                if n % 50 == 0:
                    print(f"[{n}] {self.bytes/1e6:.1f} MB this run, "
                          f"queue={self.q.qsize()}  {lp}", flush=True)
                if n % 200 == 0:
                    self.save()
            except Exception as e:
                with self.lock:
                    self.failed[url] = str(e)[:200]
                print(f"FAIL {url}: {e}", flush=True)
            finally:
                self.q.task_done()
            time.sleep(self.delay)

    def run(self):
        threads = [threading.Thread(target=self.worker, daemon=True)
                   for _ in range(self.workers)]
        for t in threads:
            t.start()
        try:
            while any(t.is_alive() for t in threads):
                time.sleep(1)
        except KeyboardInterrupt:
            print("\nStopping, saving state...")
            self.stop = True
            for t in threads:
                t.join(timeout=70)
        self.save()
        print(f"Done this run: {self.count} files, {self.bytes/1e6:.1f} MB. "
              f"Total archived: {len(set(self.done.values()))} files, "
              f"failed: {len(self.failed)}, pending: {self.q.qsize()}")


def rewrite(out):
    """Rewrite in-scope links in saved HTML to relative local paths."""
    s = json.load(open(os.path.join(out, "_state.json")))
    done = s["done"]
    changed = 0
    for page_url in s.get("html", []):
        lp = done.get(page_url)
        if not lp:
            continue
        fp = os.path.join(out, lp)
        if not os.path.exists(fp):
            continue
        body = open(fp, "rb").read()
        here = posixpath.dirname(lp)

        def sub(m):
            raw = m.group("dq") or m.group("sq") or m.group("uq") or b""
            val = raw.decode("latin-1").strip()
            if not val or val.startswith(("#", "mailto:", "javascript:")):
                return m.group(0)
            absu = urljoin(page_url, val)
            frag = urlsplit(absu).fragment
            n = normalize(absu)
            target = done.get(n) if n else None
            if not target:
                return m.group(0)
            rel = posixpath.relpath(target, here or ".")
            if frag:
                rel += "#" + frag
            return m.group("pre") + b'"' + quote(rel, safe="/#%._-~").encode("latin-1") + b'"'

        new = LINK_RE.sub(sub, body)
        if new != body:
            open(fp, "wb").write(new)
            changed += 1
    print(f"Rewrote links in {changed} HTML files. Open {os.path.join(out, 'index.html')}")


def status(out):
    s = json.load(open(os.path.join(out, "_state.json")))
    files = set(s["done"].values())
    total = sum(os.path.getsize(os.path.join(out, f)) for f in files
                if os.path.exists(os.path.join(out, f)))
    print(f"files: {len(files)}  size: {total/1e6:.1f} MB  "
          f"pending: {len(s.get('pending', []))}  failed: {len(s['failed'])}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("cmd", choices=["crawl", "rewrite", "status"])
    ap.add_argument("out")
    ap.add_argument("--workers", type=int, default=4)
    ap.add_argument("--delay", type=float, default=0.5)
    ap.add_argument("--rescan", action="store_true",
                    help="re-extract links from saved pages before crawling")
    a = ap.parse_args()
    if a.cmd == "crawl":
        c = Crawler(a.out, a.workers, a.delay)
        if a.rescan:
            c.rescan()
        c.run()
    elif a.cmd == "rewrite":
        rewrite(a.out)
    else:
        status(a.out)
