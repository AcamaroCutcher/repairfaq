#!/usr/bin/env python3
"""
Write the companion pages (they share one header and footer, so they're generated, not hand-copied).

    python3 -I pages.py SITE_DIR [PHOTOS_SITE_DIR]

Reads SITE_DIR/companion/data/{stats,diagrams}.json (written by build.py) and writes:
    SITE_DIR/index.html                 landing page
    SITE_DIR/companion/diagrams.html    diagram explorer
    SITE_DIR/companion/diagram.html     one diagram (diagram.html#<id>)
    SITE_DIR/companion/credits.html     the authors, contributors and the terms this mirror follows
    SITE_DIR/404.html
    PHOTOS_SITE_DIR/index.html          a short landing page for the photos repo
"""
import html, json, os, sys

SITE, PHOTOS = sys.argv[1], (sys.argv[2] if len(sys.argv) > 2 else None)
DATA = os.path.join(SITE, "companion", "data")
stats = json.load(open(os.path.join(DATA, "stats.json")))
diagrams = json.load(open(os.path.join(DATA, "diagrams.json")))
FEATURED = next((d for d in diagrams if "Starting Circuit" in d["ascii"]), diagrams[0])
SITE_URL = "https://acamarocutcher.github.io/repairfaq/"
HOME_URL = "https://acamaro.com/"
E = html.escape

THEME_INIT = """<script>(function(){var r=document.documentElement,s=null;try{s=localStorage.getItem('quake-viz-theme')}catch(e){}
r.classList.toggle('dark-mode',s?s==='dark':matchMedia('(prefers-color-scheme: dark)').matches);r.classList.add('js')})();</script>"""


def head(title, desc, root, extra=""):
    return f"""<!DOCTYPE html>
<html lang="en" data-root="{root}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{E(title)}</title>
<meta name="description" content="{E(desc)}">
<meta property="og:title" content="{E(title)}">
<meta property="og:description" content="{E(desc)}">
<meta property="og:type" content="website">
<link rel="icon" href="{root}companion/assets/favicon.svg" type="image/svg+xml">
{THEME_INIT}
<link rel="preconnect" href="https://fonts.googleapis.com"><link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="{root}companion/assets/companion.css">
{extra}
</head>
<body>
<a class="skip" href="#main">Skip to content</a>
"""


def topbar(root, current):
    links = [("Home", f"{root}index.html", "home"), ("Read the FAQs", f"{root}archive/index.html", "archive"),
             ("Diagrams", f"{root}companion/diagrams.html", "diagrams"), ("Credits", f"{root}companion/credits.html", "credits")]
    cur = ' aria-current="page"'
    nav = "".join(f'<a href="{h}"{cur if k == current else ""}>{n}</a>' for n, h, k in links)
    return f"""<header class="topbar"><div class="wrap">
  <a class="brand" href="{root}index.html"><span class="brand-mark" aria-hidden="true"></span>RepairFAQ<small>mirror &amp; companion</small></a>
  <nav class="nav" id="nav" aria-label="Main">{nav}</nav>
  <button class="icon-btn" type="button" data-search-open aria-label="Search all FAQs (press /)"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><circle cx="11" cy="11" r="7"/><path d="m20 20-3.5-3.5"/></svg></button>
  <button class="icon-btn" type="button" id="themeToggle" aria-label="Switch theme"></button>
  <button class="icon-btn menu-btn" type="button" id="menuBtn" aria-label="Menu" aria-expanded="false" aria-controls="nav"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M4 7h16M4 12h16M4 17h16"/></svg></button>
</div></header>
<dialog class="search-dialog" id="searchDialog" aria-label="Search the FAQs">
  <form method="dialog"><input class="search-input" type="search" placeholder="Search {stats['pages']:,} pages: try “flyback”, “HeNe”, “microwave”…" aria-label="Search" autocomplete="off"></form>
  <ul class="search-results"></ul>
</dialog>
"""


def footer(root):
    return f"""<footer class="footer"><div class="wrap">
  <div>
    <h4>Credit where it is due</h4>
    <p>All FAQ content is the work of <strong>Samuel M. Goldwasser</strong> and <strong>Filip M. Gieszczykiewicz</strong>,
    and of the many contributors quoted by name throughout, © the original authors, all rights reserved.
    The original and authoritative site is <a href="https://www.repairfaq.org/">https://www.repairfaq.org/</a>.
    This is a free, non-commercial mirror, kept so the information lasts. It carries no ads and charges nothing.</p>
    <p>Mirror and companion maintained by <a href="{HOME_URL}">Acamaro Cutcher</a>.</p>
  </div>
  <div><h4>The FAQs</h4><ul>
    <li><a href="{root}archive/sam/laserfaq.htm">Sam's Laser FAQ</a></li>
    <li><a href="{root}archive/REPAIR/F_Repair.html">S.E.R FAQ table of contents</a></li>
    <li><a href="{root}archive/sam/sammenu.htm">Silicon Sam's Technology Resource</a></li>
    <li><a href="{root}archive/sam/copyright.htm">Copyright and fair use</a></li>
  </ul></div>
  <div><h4>Companion</h4><ul>
    <li><a href="{root}companion/diagrams.html">Diagram explorer</a></li>
    <li><a href="{root}companion/credits.html">Credits and terms</a></li>
    <li><a href="{root}AI_DIAGRAM_GUIDE.md">Guide for AI diagram tools</a></li>
    <li><a href="{HOME_URL}projects/">acamaro.com projects</a></li>
  </ul></div>
</div></footer>
<script src="{root}companion/assets/companion.js"></script>
"""


ICONS = {
    "laser": '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><rect x="2" y="9" width="12" height="6" rx="2"/><path d="M14 12h8M18 8l2-2M18 16l2 2"/></svg>',
    "wrench": '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14.7 6.3a4 4 0 0 0 5 5L21 13l-8 8-5-5 8-8-1.3-1.7z"/><path d="M3 21l5-5"/></svg>',
    "chip": '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><rect x="6" y="6" width="12" height="12" rx="2"/><path d="M9 2v4M15 2v4M9 18v4M15 18v4M2 9h4M2 15h4M18 9h4M18 15h4"/></svg>',
    "home": '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linejoin="round"><path d="M3 11 12 3l9 8v10H3z"/><path d="M9 21v-6h6v6"/></svg>',
    "camera": '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linejoin="round"><path d="M3 7h4l2-3h6l2 3h4v13H3z"/><circle cx="12" cy="13" r="4"/></svg>',
    "scale": '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M12 3v18M5 21h14M6 7h12M6 7l-3 7a3 3 0 0 0 6 0zM18 7l-3 7a3 3 0 0 0 6 0z"/></svg>',
}


def card(href, icon, title, text, meta):
    return f"""<a class="card" href="{href}"><span class="card-icon">{ICONS[icon]}</span><h3>{title}</h3><p>{text}</p><span class="meta">{meta}</span><span class="go">Open</span></a>"""


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    open(path, "w", encoding="utf-8").write(text)


# ------------------------------------------------------------------ landing
featured_json = json.dumps(FEATURED["ascii"])
landing = head("RepairFAQ: Sam's Repair & Laser FAQs, preserved",
               "An unaltered mirror of repairfaq.org (the Sci.Electronics.Repair FAQ and Sam's Laser FAQ by Samuel M. Goldwasser and Filip M. Gieszczykiewicz) with a modern companion: search, and every ASCII circuit diagram redrawn in animated 2D and 3D.",
               "./") + topbar("./", "home") + f"""
<main id="main">
<section class="hero">
  <div class="wrap">
    <div class="reveal">
      <span class="eyebrow"><span class="pulse"></span>Unaltered mirror · modern companion</span>
      <h1>Three decades of repair wisdom, <span class="glow">preserved.</span></h1>
      <p class="lede">The Sci.Electronics.Repair FAQ and Sam's Laser FAQ are among the largest free collections of practical
      electronics knowledge ever written: how to troubleshoot TVs, microwave ovens, power supplies, CD players and lasers,
      with thousands of hand-drawn ASCII schematics. This site keeps a faithful copy online and adds a companion for exploring it.</p>
      <div class="byline">Written by <strong>Samuel M. Goldwasser</strong> and <strong>Filip M. Gieszczykiewicz</strong>,
      with contributions from <strong>{stats['contributors']:,}</strong> people credited by name in the text.
      The original lives at <a href="https://www.repairfaq.org/">repairfaq.org</a>.</div>
      <div class="cta">
        <a class="btn primary" href="archive/index.html">Read the FAQs</a>
        <a class="btn" href="companion/diagrams.html">Explore {stats['diagrams']} diagrams</a>
        <button class="btn" type="button" data-search-open>Search</button>
      </div>
    </div>
    <div class="hero-3d reveal" id="hero3d" aria-label="Animated 3D model of a helium-neon laser tube; hover over the parts to see their names">
      <div class="part-label" id="partLabel"></div>
      <span class="hint">drag to rotate · hover the parts</span>
    </div>
  </div>
</section>

<section class="section">
  <div class="wrap">
    <div class="section-head reveal"><span class="kicker">What is here</span>
      <h2>The whole site, page for page.</h2>
      <p>Every page is a byte-for-byte copy of the original. The only additions are a line naming the source and
      a stylesheet that adjusts fonts and colors, which the authors' terms allow. Everything in the companion is new and clearly separate.</p></div>
    <div class="stats reveal-stagger">
      <div class="stat"><b data-count="{stats['pages']}">{stats['pages']:,}</b><span>pages mirrored</span></div>
      <div class="stat"><b data-count="{stats['diagrams']}">{stats['diagrams']:,}</b><span>ASCII circuit diagrams, redrawn</span></div>
      <div class="stat"><b data-count="{stats['images']}">{stats['images']:,}</b><span>photos and schematics</span></div>
      <div class="stat"><b data-count="{stats['contributors']}">{stats['contributors']:,}</b><span>contributors credited</span></div>
    </div>
  </div>
</section>

<section class="section alt">
  <div class="wrap">
    <div class="section-head reveal"><span class="kicker">Start reading</span><h2>The collections</h2></div>
    <div class="cards three reveal-stagger">
      {card("archive/sam/laserfaq.htm", "laser", "Sam's Laser FAQ", "A practical guide to lasers for experimenters and hobbyists: safety, helium-neon, diode, argon/krypton ion and CO2 lasers, power supplies, and home-built lasers.", "archive/sam/laserfaq.htm")}
      {card("archive/REPAIR/F_Repair.html", "wrench", "Sci.Electronics.Repair FAQ", "The main table of contents of the S.E.R FAQ: the “Notes on the Troubleshooting and Repair of…” series and material from many other contributors.", "archive/REPAIR/F_Repair.html")}
      {card("archive/sam/sammenu.htm", "chip", "Silicon Sam's Technology Resource", "Sam Goldwasser's repair guides, component testing, schematics, and the rest of his technology notes in one menu.", "archive/sam/sammenu.htm")}
      {card("archive/sam/laserpic.htm", "camera", "Laser Equipment Gallery", "Photographs of laser tubes, heads, power supplies and optics, plus dissections of lenses and equipment.", "archive/sam/laserpic.htm")}
      {card("archive/index.html", "home", "The original home page", "The S.E.R FAQ “Galactic Homepage”, exactly as it appears on repairfaq.org.", "archive/index.html")}
      {card("archive/sam/copyright.htm", "scale", "Copyright and fair use", "The authors' terms for using this material. This mirror follows them: see the credits page for how.", "archive/sam/copyright.htm")}
    </div>
  </div>
</section>

<section class="section">
  <div class="wrap">
    <div class="section-head reveal"><span class="kicker">Companion</span>
      <h2>Every ASCII schematic, redrawn.</h2>
      <p>Sam drew his circuits with nothing but keyboard characters, so they would survive any newsgroup, mail client or screen.
      The companion reads each one and draws it again as a vector schematic with animated current, and as a 3D circuit board you can orbit.
      Hover a wire to light up everything connected to it. The original ASCII is always one click away.</p></div>
    <div class="reveal" id="featured"></div>
    <p class="reveal" style="margin-top:14px;color:var(--muted);font-size:.92rem">“{E(FEATURED['heading'])}”, from <a href="archive/{E(FEATURED['page'])}{('#' + E(FEATURED['anchor'])) if FEATURED['anchor'] else ''}">{E(FEATURED['title'])}</a>,
    by Samuel M. Goldwasser. <a href="companion/diagram.html#{FEATURED['id']}">Diagram page with the full copyright notice</a>.</p>
    <div class="cta reveal" style="margin-top:22px"><a class="btn primary" href="companion/diagrams.html">Browse all {stats['diagrams']} diagrams</a></div>
  </div>
</section>

<section class="section alt">
  <div class="wrap">
    <div class="section-head reveal"><span class="kicker">With respect</span><h2>How this mirror honors the authors' wishes</h2>
      <p>The site's <a href="archive/sam/copyright.htm">Copyright and Fair Use</a> page sets out how the FAQs may be reproduced. This mirror follows it:</p></div>
    <div class="cards reveal-stagger">
      <div class="card"><h3>Unaltered</h3><p>Pages are copied byte for byte. Only fonts and colors are restyled, “to be consistent with other pages on the Web site,” as the terms allow.</p></div>
      <div class="card"><h3>Credited and linked</h3><p>Every page starts with an HTML comment naming where it came from, and a visible line linking the original on repairfaq.org.</p></div>
      <div class="card"><h3>Notices kept</h3><p>Each document's “Author and Copyright” section stays at the top. Each diagram page in the companion repeats it in full.</p></div>
      <div class="card"><h3>Free, no ads</h3><p>Nothing is sold, and there are no ads or sponsored links: non-commercial use only, as the authors require.</p></div>
    </div>
  </div>
</section>
</main>
""" + footer("./") + f"""
<script type="module">
import {{ mountViewer }} from './companion/assets/viewer.js';
mountViewer(document.getElementById('featured'), {featured_json}, {{ title: {json.dumps(FEATURED['heading'])}, source: {json.dumps(FEATURED['source'])} }});
const hero = document.getElementById('hero3d');
let started = false;
const go = () => {{ if (started) return; started = true; import('./companion/assets/hero3d.js').then(m => m.mountHero(hero, document.getElementById('partLabel'))).catch(e => {{ console.error(e); hero.remove(); }}); }};
'requestIdleCallback' in window ? requestIdleCallback(go, {{ timeout: 1200 }}) : setTimeout(go, 300);
</script>
</body></html>
"""
write(os.path.join(SITE, "index.html"), landing)

# ------------------------------------------------------------------ explorer
explorer = head("Diagram explorer · RepairFAQ companion",
                f"All {stats['diagrams']} ASCII circuit diagrams from the Sci.Electronics.Repair FAQ and Sam's Laser FAQ, redrawn as animated schematics and 3D boards.",
                "../") + topbar("../", "diagrams") + f"""
<main id="main">
<section class="section" style="padding-top:44px">
  <div class="wrap">
    <div class="section-head reveal"><span class="kicker">Diagram explorer</span>
      <h1 style="font-size:clamp(2rem,4.5vw,3.2rem)">{stats['diagrams']} circuits, drawn in characters</h1>
      <p>Every ASCII diagram in the FAQs, found automatically and redrawn. Click one for the animated and 3D views, the original text,
      the passage it comes from, and its copyright notice. Drawings are by Samuel M. Goldwasser unless the FAQ credits someone else.</p></div>
    <div class="toolbar">
      <input class="search-input" id="q" type="search" placeholder="Filter by words in the diagram, its heading or its page: “flyback”, “Q1”, “ballast”…" aria-label="Filter diagrams">
      <span class="count" id="count" aria-live="polite"></span>
    </div>
    <div class="toolbar">
      <div class="chips" id="colls" role="group" aria-label="Collection"></div>
    </div>
    <div class="toolbar"><div class="chips" id="parts" role="group" aria-label="Contains"></div></div>
    <div class="dgrid" id="grid"></div>
    <div class="more"><button class="btn" id="more" type="button">Show more</button></div>
  </div>
</section>
</main>
""" + footer("../") + """
<script type="module">
import { mount } from './assets/schematic.js';
const COLL = { all: 'All', laser: "Sam's Laser FAQ", sam: 'Repair guides', repair: 'S.E.R FAQ', other: 'Other' };
const PARTS = ['resistor','capacitor','diode','transistor','transformer','inductor','ground','switch','fuse','lamp/tube','ic','led','motor','battery'];
const state = { q: '', coll: 'all', parts: new Set(), shown: 0 };
const grid = document.getElementById('grid'), more = document.getElementById('more'), count = document.getElementById('count');
const data = await fetch('data/diagrams.json').then(r => r.json());
const esc = s => s.replace(/[&<>"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const params = new URLSearchParams(location.search);
state.q = params.get('q') || ''; state.coll = params.get('c') || 'all';
document.getElementById('q').value = state.q;

function chips(el, items, isOn, onClick) {
  el.innerHTML = items.map(([k, label]) => `<button class="chip" type="button" data-k="${k}" aria-pressed="${isOn(k)}">${label}</button>`).join('');
  el.querySelectorAll('.chip').forEach(b => b.addEventListener('click', () => { onClick(b.dataset.k); render(true); }));
}
function drawChips() {
  const present = new Set(data.map(d => d.collection));
  chips(document.getElementById('colls'), Object.entries(COLL).filter(([k]) => k === 'all' || present.has(k)), k => k === state.coll, k => { state.coll = k; drawChips(); });
  chips(document.getElementById('parts'), PARTS.map(p => [p, p]), k => state.parts.has(k), k => { state.parts.has(k) ? state.parts.delete(k) : state.parts.add(k); drawChips(); });
}
let list = [];
const io = new IntersectionObserver(es => es.forEach(e => {
  if (!e.isIntersecting) return;
  const el = e.target; io.unobserve(el);
  try { mount(el, data[+el.dataset.i].ascii, { idPrefix: 'g' + el.dataset.i, title: el.dataset.t }); } catch (err) { el.innerHTML = '<pre class="ascii">' + esc(data[+el.dataset.i].ascii) + '</pre>'; }
}), { rootMargin: '300px' });
function render(reset) {
  const words = state.q.toLowerCase().split(/\\s+/).filter(Boolean);
  list = data.map((d, i) => [d, i]).filter(([d]) =>
    (state.coll === 'all' || d.collection === state.coll) &&
    [...state.parts].every(p => d.parts.includes(p)) &&
    words.every(w => (d.heading + ' ' + d.title + ' ' + d.page + ' ' + d.ascii + ' ' + d.context).toLowerCase().includes(w)));
  if (reset) { grid.innerHTML = ''; state.shown = 0; }
  const next = list.slice(state.shown, state.shown + 24);
  grid.insertAdjacentHTML('beforeend', next.map(([d, i]) => `
    <a class="dcard" href="diagram.html#${d.id}">
      <div class="thumb" data-i="${i}" data-t="${esc(d.heading)}"></div>
      <div class="body"><h3>${esc(d.heading)}</h3><div class="src">${esc(d.title)}</div>
      <div class="tags">${d.parts.slice(0, 5).map(p => `<span class="tag">${p}</span>`).join('')}</div></div>
    </a>`).join(''));
  grid.querySelectorAll('.thumb:not([data-seen])').forEach(t => { t.dataset.seen = 1; io.observe(t); });
  state.shown += next.length;
  count.textContent = `${list.length} of ${data.length} diagrams`;
  more.hidden = state.shown >= list.length;
}
let timer;
document.getElementById('q').addEventListener('input', e => { clearTimeout(timer); timer = setTimeout(() => { state.q = e.target.value; render(true); }, 160); });
more.addEventListener('click', () => render(false));
drawChips(); render(true);
</script>
</body></html>
"""
write(os.path.join(SITE, "companion", "diagrams.html"), explorer)

# ------------------------------------------------------------------ one diagram
detail = head("Diagram · RepairFAQ companion", "An ASCII circuit diagram from the Sci.Electronics.Repair FAQ / Sam's Laser FAQ, redrawn.",
              "../") + topbar("../", "diagrams") + """
<main id="main">
  <div class="wrap">
    <div class="detail-head">
      <div class="crumbs"><a href="diagrams.html">Diagram explorer</a> / <span id="crumbPage"></span></div>
      <h1 id="title">Loading…</h1>
      <p id="sub" style="color:var(--ink-2)"></p>
    </div>
    <div class="aside-card attrib" id="notice"></div>
    <div class="detail-grid">
      <div>
        <div id="viewer"></div>
        <div class="pager"><a class="btn" id="prev" href="#">← Previous</a><a class="btn" id="next" href="#">Next →</a></div>
      </div>
      <aside>
        <div class="aside-card"><h3>Read it in context</h3>
          <p>This diagram appears in <a id="ctxLink" href="#"></a>.</p>
          <p><a class="btn primary" id="readHere" href="#">Open in the mirror</a></p>
          <p style="margin:0"><a id="srcLink" href="#">On repairfaq.org</a></p></div>
        <div class="aside-card" id="ctxCard"><h3>The passage just before it</h3><blockquote id="ctx"></blockquote></div>
        <div class="aside-card"><h3>About this drawing</h3>
          <p>The animated and 3D views are drawn automatically from the ASCII text, as an interpretation of the original.
          If they ever disagree, the original ASCII is authoritative.</p>
          <p id="parts"></p></div>
      </aside>
    </div>
  </div>
</main>
""" + footer("../") + """
<script type="module">
import { mountViewer } from './assets/viewer.js';
const [data, notices] = await Promise.all([fetch('data/diagrams.json').then(r => r.json()), fetch('data/notices.json').then(r => r.json())]);
const esc = s => s.replace(/[&<>"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
function show() {
  const [id, view] = location.hash.slice(1).split('/');
  let i = data.findIndex(d => d.id === id); if (i < 0) i = 0;
  const d = data[i];
  const local = '../archive/' + encodeURI(d.page) + (d.anchor ? '#' + encodeURIComponent(d.anchor) : '');
  document.title = `${d.heading} · RepairFAQ companion`;
  document.getElementById('title').textContent = d.heading;
  document.getElementById('sub').innerHTML = `From <a href="${local}">${esc(d.title)}</a>`;
  document.getElementById('crumbPage').textContent = d.title;
  document.getElementById('notice').innerHTML = `<h3>Author and Copyright</h3><p style="margin:0 0 6px;font-size:.85rem;color:var(--muted)">From the document this diagram belongs to${d.notice_from === 'site' ? ' (this page has no notice of its own, so these are the site-wide terms from repairfaq.org/sam/copyright.htm)' : ''}, reproduced in full as its terms require:</p><div class="notice">${esc(notices[d.notice] || '')}</div>`;
  document.getElementById('ctxLink').textContent = d.title; document.getElementById('ctxLink').href = local;
  document.getElementById('readHere').href = local;
  document.getElementById('srcLink').href = d.source;
  document.getElementById('ctx').textContent = d.context ? '…' + d.context : '';
  document.getElementById('ctxCard').hidden = !d.context;
  document.getElementById('parts').textContent = d.parts.length ? 'Recognized parts: ' + d.parts.join(', ') + (d.refs.length ? '. Reference designators: ' + d.refs.join(', ') : '') + '.' : '';
  const v = document.getElementById('viewer'); v.className = ''; v.innerHTML = '';
  mountViewer(v, d.ascii, { title: d.heading, source: d.source, credit: d.title, initial: { '3d': '3d', ascii: 'ascii' }[view] || 'svg' });
  document.getElementById('prev').href = '#' + data[(i - 1 + data.length) % data.length].id;
  document.getElementById('next').href = '#' + data[(i + 1) % data.length].id;
  window.scrollTo({ top: 0, behavior: 'instant' in window ? 'instant' : 'auto' });
}
addEventListener('hashchange', show);
show();
</script>
</body></html>
"""
write(os.path.join(SITE, "companion", "diagram.html"), detail)

# ------------------------------------------------------------------ credits
credits = head("Credits & terms · RepairFAQ companion", "Who wrote the Sci.Electronics.Repair FAQ and Sam's Laser FAQ, everyone credited in it, and the terms this mirror follows.",
               "../") + topbar("../", "credits") + f"""
<main id="main">
<section class="section" style="padding-top:44px"><div class="wrap">
  <div class="section-head reveal"><span class="kicker">Credits</span>
    <h1 style="font-size:clamp(2rem,4.5vw,3.2rem)">The people who wrote it</h1>
    <p>This mirror exists out of respect for their work and gratitude for making it free to everyone.</p></div>
  <div class="honor reveal-stagger">
    <div class="card"><h3>Samuel M. Goldwasser</h3>
      <p>Author of Sam's Laser FAQ (“A Practical Guide to Lasers for Experimenters and Hobbyists”), the “Notes on the
      Troubleshooting and Repair of…” series of repair guides, and Silicon Sam's Technology Resource. In his words, the
      resource “was developed specifically for the Do-It-Yourselfer, electronics experimenter, hobbyist, technician,
      engineer, and possibly even including the dentist and poet!”</p>
      <span class="meta"><a href="../archive/sam/faqsam.htm">His welcome page</a> · <a href="https://www.repairfaq.org/sam/">repairfaq.org/sam/</a></span></div>
    <div class="card"><h3>Filip M. Gieszczykiewicz</h3>
      <p>Co-holder of the copyright on the Sci.Electronics.Repair FAQ site with Sam Goldwasser. The site's terms credit
      him with the S.E.R FAQ material and the HTML look and feel of its pages, which this mirror keeps intact.</p>
      <span class="meta"><a href="https://www.repairfaq.org/filipg/">repairfaq.org/filipg/</a></span></div>
  </div>

  <div class="prose" style="margin-top:56px">
    <h2 class="reveal">Contributors</h2>
    <p>The FAQs quote hundreds of people from the sci.electronics.repair newsgroup and by email, each marked
    “(From: …)” in the text, and that material is copyright its contributing authors. Below are the
    <span id="ncontrib">{stats['contributors']:,}</span> names found in those credits, with how many times each appears.
    The list is generated automatically, so a few entries are partial names or the same person spelled two ways.</p>
    <input class="search-input" id="cq" type="search" placeholder="Find a name" aria-label="Find a contributor" style="margin:8px 0 16px;width:100%">
  </div>
  <ul class="people" id="people"></ul>
  <div class="more"><button class="btn" id="allPeople" type="button">Show everyone</button></div>

  <div class="prose">
    <h2 id="terms">The terms, and how this mirror follows them</h2>
    <p>From the site's <a href="../archive/sam/copyright.htm">Copyright and Fair Use Considerations</a> (version 1.04):</p>
    <ul>
      <li><strong>“The entire section ‘About the Author and Copyright’ MUST be included when any sections are excerpted.”</strong>
        Mirrored pages are complete, so their notices are intact. Each diagram page in the companion is an excerpt, and shows its document's notice in full at the top.</li>
      <li><strong>“Documents or excerpts must NOT be altered in any way that affects their meaning.”</strong>
        The pages are byte-for-byte copies. The diagram views are clearly labeled as new drawings, with the original ASCII beside them.</li>
      <li><strong>“There must be a header, as an HTML comment, at the top of each page showing where the material originated.”</strong> Added to every page.</li>
      <li><strong>“An active link to the original document… clearly labeled as the source.”</strong> Every page has a “Source:” line linking its original URL.</li>
      <li><strong>The HTML look and feel “should not be altered. Changes to font and color schemes are permitted to the extent required to be consistent with other pages on the Web site.”</strong>
        One stylesheet sets fonts and colors to match this site. It changes nothing else.</li>
      <li><strong>No ads, no charge, non-commercial.</strong> This site carries no ads or sponsored links and charges nothing.</li>
    </ul>
    <p>Sam's Laser FAQ adds that “non-commercial use of this material is permitted as long as the source is acknowledged in an appropriate manner.”
    If you reuse anything you find here, please follow the same terms and link to <a href="https://www.repairfaq.org/">https://www.repairfaq.org/</a>.</p>

    <h2>About this mirror</h2>
    <p>The site was downloaded in November 2026 so that this knowledge survives even if the original goes offline.
    What is hosted where:</p>
    <ul>
      <li>Pages, schematics, PDFs and most photos are hosted here.</li>
      <li>The three large photo galleries (laser equipment, repair photos, brochures; {stats.get('photo_files', 0):,} photos) are hosted at
        <a href="https://acamarocutcher.github.io/repairfaq-photos/">acamarocutcher.github.io/repairfaq-photos</a>, because GitHub Pages limits each site to 1 GB.</li>
      <li>{stats['excluded_files']} large videos, slide shows and programs are not hosted. Their links point to repairfaq.org, and a full copy is kept offline.</li>
    </ul>
    <p>If you are one of the authors and would like anything changed or removed, please contact <a href="{HOME_URL}#contact">Acamaro Cutcher</a>; it will be done right away.</p>
  </div>
</div></section>
</main>
""" + footer("../") + """
<script>
(function () {
  var ul = document.getElementById('people'), all = false, people = [];
  function esc(s) { return s.replace(/[&<>"]/g, function (c) { return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]; }); }
  function draw() {
    var q = document.getElementById('cq').value.toLowerCase();
    var list = people.filter(function (p) { return !q || p[0].toLowerCase().indexOf(q) >= 0; });
    var shown = all || q ? list : list.slice(0, 150);
    ul.innerHTML = shown.map(function (p) { return '<li>' + esc(p[0]) + '<span>' + p[1] + '</span></li>'; }).join('');
    document.getElementById('allPeople').hidden = all || !!q || list.length <= 150;
  }
  fetch('data/contributors.json').then(function (r) { return r.json(); }).then(function (p) { people = p; draw(); });
  document.getElementById('cq').addEventListener('input', draw);
  document.getElementById('allPeople').addEventListener('click', function () { all = true; draw(); });
})();
</script>
</body></html>
"""
write(os.path.join(SITE, "companion", "credits.html"), credits)

# ------------------------------------------------------------------ 404
write(os.path.join(SITE, "404.html"), head("Not found · RepairFAQ", "Page not found.", "/repairfaq/") + topbar("/repairfaq/", "") + """
<main id="main"><section class="section"><div class="wrap prose">
<h1>That page isn't here.</h1>
<p>The original site had some broken links of its own, and a few large files are only on repairfaq.org.
Try <a href="/repairfaq/index.html">the home page</a>, search (press <kbd>/</kbd>), or the same path on
<a id="orig" href="https://www.repairfaq.org/">repairfaq.org</a>.</p>
<script>var p=location.pathname.replace(/^\\/repairfaq\\/(archive\\/)?/,'');document.getElementById('orig').href='https://www.repairfaq.org/'+p;</script>
</div></section></main>""" + footer("/repairfaq/") + "</body></html>")

# ------------------------------------------------------------------ photos repo landing
if PHOTOS:
    write(os.path.join(PHOTOS, "index.html"), f"""<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>RepairFAQ photo galleries</title>
<style>body{{font-family:system-ui,sans-serif;max-width:42em;margin:0 auto;padding:40px 16px;line-height:1.6;color:#15140f;background:#f6f5f0}}
@media (prefers-color-scheme:dark){{body{{color:#f2f0ea;background:#0a0b0a}}a{{color:#ff8a6b}}}}</style></head>
<body><h1>RepairFAQ photo galleries</h1>
<p>This site holds the photos from three galleries of <a href="https://www.repairfaq.org/">repairfaq.org</a>
(the Laser Equipment Gallery, the repair photo dissections and the brochures), for the mirror at
<a href="{SITE_URL}">{SITE_URL}</a>. The pages that show them are on the mirror.</p>
<p>All photos are © Samuel M. Goldwasser or their credited owners, all rights reserved, and are reproduced here
unaltered and free of charge under the site's <a href="https://www.repairfaq.org/sam/copyright.htm">fair-use terms</a>.</p>
<p><a href="{SITE_URL}archive/sam/laserpic.htm">Open the Laser Equipment Gallery</a></p>
</body></html>
""")
print("pages written")
