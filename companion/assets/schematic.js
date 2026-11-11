/*
 * schematic.js: reads an ASCII-art circuit diagram and turns it into a geometric model, then an SVG.
 *
 * The ASCII original is always the source of truth: nothing here edits it. The model is a best-effort
 * reading of the drawing conventions used in Sam Goldwasser's FAQs:
 *
 *   -  =      horizontal wire              |        vertical wire
 *   _         wire along the bottom of a cell (grounds, capacitor plates: _|_ and ---)
 *   +  .  '   junction / corner            *        junction dot
 *   o         terminal                      / \     diagonal (resistor zig-zags /\/\, switches o/ o)
 *   > < v ^   arrowheads on a wire          ( )     coil turns next to | or :  (transformers )||( )
 *   :         transformer core (stacked)    | crossing a - run   a crossing without connection (hop)
 *
 * Everything else is text and is kept on the same character grid, so labels stay where Sam put them.
 * Coordinates are in character cells: cell (r, c) covers x in [c, c+1], y in [r, r+1].
 */

const ALNUM = /[A-Za-z0-9]/;
const H_CONN = new Set(['-', '=', '+', '.', "'", '`', '*', 'o', '>', '<', '/', '\\', '|', '(', ')', '[', ']', '_']);

export function parse(ascii) {
  const lines = ascii.replace(/\r/g, '').split('\n').map(expandTabs);
  const rows = lines.length;
  const cols = Math.max(1, ...lines.map((l) => l.length));
  const ch = (r, c) => (r >= 0 && r < rows && c >= 0 && c < lines[r].length ? lines[r][c] : ' ');
  const al = (s) => ALNUM.test(s);

  // pass 1: classify every cell
  const kind = []; // 'w' wire char, 't' text, '' blank
  for (let r = 0; r < rows; r++) {
    kind.push([]);
    for (let c = 0; c < cols; c++) kind[r].push(classify(r, c));
  }

  function classify(r, c) {
    const x = ch(r, c);
    if (x === ' ') return '';
    const L = ch(r, c - 1), R = ch(r, c + 1), U = ch(r - 1, c), D = ch(r + 1, c);
    const wordy = al(L) && al(R);
    switch (x) {
      case '-': case '=':
        if (wordy) return 't';
        if (x === '-' && (L === '-' || R === '-')) return 'w';
        if ((H_CONN.has(L) && !al(L)) || (H_CONN.has(R) && !al(R))) return 'w';
        return 't';
      case '|':
        return wordy ? 't' : 'w';
      case '_':
        return wordy ? 't' : 'w';
      case '/': case '\\': {
        if (wordy || (al(L) && R === ' ') || (al(R) && L === ' ' && !/[-|_]/.test(U + D))) return 't';
        const around = [L, R, U, D, ch(r - 1, c - 1), ch(r - 1, c + 1), ch(r + 1, c - 1), ch(r + 1, c + 1)];
        return around.some((a) => '-|_/\\+o.='.includes(a) && a !== ' ') ? 'w' : 't';
      }
      case 'o':
        if (al(L) || al(R)) return 't';
        return (L === '-' || R === '-' || L === '/' || R === '/' || U === '|' || D === '|' || L === '=' || R === '=') ? 'w' : 't';
      case '+': case '.': case "'": case '`': case '*':
        return connections(r, c).length >= (x === '+' ? 1 : 2) ? 'w' : 't';
      case '>': return L === '-' || L === '=' ? 'w' : 't';
      case '<': return R === '-' || R === '=' ? 'w' : 't';
      case 'v': return U === '|' && !al(L) && !al(R) ? 'w' : 't';
      case '^': return D === '|' && !al(L) && !al(R) ? 'w' : 't';
      case '(': case ')':
        return (U === x || D === x || '|:'.includes(L) || '|:'.includes(R)) && !al(L) && !al(R) ? 'w' : 't';
      case ':':
        return (U === ':' || D === ':') && !al(L) && !al(R) ? 'w' : 't';
      default:
        return 't';
    }
  }

  function connections(r, c) {
    const out = [];
    const L = ch(r, c - 1), R = ch(r, c + 1), U = ch(r - 1, c), D = ch(r + 1, c);
    if ('-=+.\'`*o<_'.includes(L) && L !== ' ' && !(L === 'o' && al(ch(r, c - 2)))) out.push('L');
    if ('-=+.\'`*o>_'.includes(R) && R !== ' ' && !(R === 'o' && al(ch(r, c + 2)))) out.push('R');
    if ('|+.*o^v:'.includes(U) && U !== ' ') out.push('U');
    if ("|+'`*ov^:".includes(D) && D !== ' ') out.push('D');
    return out;
  }

  // pass 2: geometry
  const segs = [];   // {x1,y1,x2,y2,type}
  const curves = []; // {d, a:[x,y], b:[x,y]} SVG path in cell units, endpoints for nets
  const dots = [], terms = [], arrows = [];
  const S = (x1, y1, x2, y2, type = 'w') => segs.push({ x1, y1, x2, y2, type });
  const isH = (s) => s === '-' || s === '=' || s === '+' || s === '.' || s === "'" || s === '*' || s === 'o' || s === '<' || s === '>';

  for (let r = 0; r < rows; r++) {
    for (let c = 0; c < cols; c++) {
      if (kind[r][c] !== 'w') continue;
      const x = ch(r, c);
      const cx = c + 0.5, cy = r + 0.5;
      const L = ch(r, c - 1), R = ch(r, c + 1), U = ch(r - 1, c), D = ch(r + 1, c);
      switch (x) {
        case '-':
          S(c, cy, c + 1, cy);
          break;
        case '=':
          S(c, cy - 0.12, c + 1, cy - 0.12, 'w2'); S(c, cy + 0.12, c + 1, cy + 0.12, 'w2');
          break;
        case '_':
          S(c, r + 1, c + 1, r + 1);
          break;
        case '|': {
          S(cx, r, cx, r + 1);
          const lw = kind[r][c - 1] === 'w' && isH(L) && L !== 'o';
          const rw = kind[r][c + 1] === 'w' && isH(R) && R !== 'o';
          // a '|' interrupting a horizontal run, with wire above or below: a crossing, not a joint
          if (lw && rw && (U === '|' || D === '|')) S(c, cy, c + 1, cy, 'hop');
          else if (lw && !rw) S(c, cy, cx, cy, 'stub');      // wire ending on a box side
          else if (rw && !lw) S(cx, cy, c + 1, cy, 'stub');
          // ground / plate: _|_  (the underscores sit on the cell's bottom edge)
          if (L === '_' || R === '_') S(L === '_' ? c : cx, r + 1, R === '_' ? c + 1 : cx, r + 1);
          break;
        }
        case ':':
          S(cx, r, cx, r + 1, 'core');
          break;
        case '/': case '\\': {
          const up = x === '/';
          const left = up ? [c, r + 1] : [c, r], right = up ? [c + 1, r] : [c + 1, r + 1];
          const top = up ? right : left, bottom = up ? left : right;
          S(left[0], left[1], right[0], right[1]);
          if (kind[r][c - 1] === 'w' && isH(L)) S(c, cy, left[0], left[1]);
          if (kind[r][c + 1] === 'w' && isH(R)) S(c + 1, cy, right[0], right[1]);
          if ('|+'.includes(U) && U !== ' ' && kind[r - 1]?.[c] === 'w') S(cx, r, top[0], top[1]);
          if ('|+'.includes(D) && D !== ' ' && kind[r + 1]?.[c] === 'w') S(cx, r + 1, bottom[0], bottom[1]);
          break;
        }
        case 'o': {
          terms.push([cx, cy]);
          const k = 0.28;
          if ('-=/_'.includes(L)) S(c, cy, cx - k, cy);
          if ('-=/_'.includes(R) || R === '\\') S(cx + k, cy, c + 1, cy);
          if (U === '|') S(cx, r, cx, cy - k);
          if (D === '|') S(cx, cy + k, cx, r + 1);
          // terminal center joins its stubs into one net
          segs.push({ x1: cx, y1: cy, x2: cx, y2: cy, type: 'node' });
          break;
        }
        case '+': case '.': case "'": case '`': case '*': {
          const cn = connections(r, c);
          const edge = { L: [c, cy], R: [c + 1, cy], U: [cx, r], D: [cx, r + 1] };
          const corner = x !== '+' && x !== '*' && cn.length === 2 && (cn.includes('L') || cn.includes('R')) && (cn.includes('U') || cn.includes('D'));
          if (corner) {
            const a = edge[cn.find((d) => d === 'L' || d === 'R')];
            const b = edge[cn.find((d) => d === 'U' || d === 'D')];
            curves.push({ d: `M${a[0]} ${a[1]} Q${cx} ${cy} ${b[0]} ${b[1]}`, a, b });
          } else {
            for (const d of cn) S(cx, cy, edge[d][0], edge[d][1]);
          }
          if (x === '*' || cn.length >= 3) dots.push([cx, cy]);
          break;
        }
        case '>': arrows.push([cx, cy, 0]); S(c, cy, cx, cy); break;
        case '<': arrows.push([cx, cy, 180]); S(cx, cy, c + 1, cy); break;
        case 'v': arrows.push([cx, cy, 90]); S(cx, r, cx, cy); break;
        case '^': arrows.push([cx, cy, -90]); S(cx, cy, cx, r + 1); break;
        case ')': case '(': {
          const s = x === ')' ? 1 : -1;
          const a = [cx - 0.25 * s, r], b = [cx - 0.25 * s, r + 1];
          curves.push({ d: `M${a[0]} ${a[1]} Q${cx + 0.55 * s} ${cy} ${b[0]} ${b[1]}`, a, b, coil: true });
          break;
        }
      }
    }
  }

  // text runs: consecutive text cells on a row, allowing single spaces inside a label
  const text = [];
  for (let r = 0; r < rows; r++) {
    let start = -1, buf = '';
    for (let c = 0; c <= cols; c++) {
      const isT = c < cols && kind[r][c] === 't';
      const gap = c < cols && kind[r][c] === '' && start >= 0 && kind[r][c + 1] === 't';
      if (isT || gap) {
        if (start < 0) start = c;
        buf += isT ? ch(r, c) : ' ';
      } else if (start >= 0) {
        text.push({ r, c: start, s: buf });
        start = -1; buf = '';
      }
    }
  }

  const boxes = findBoxes(ch, rows, cols);
  const model = { rows, cols, segs, curves, dots, terms, arrows, text, boxes };
  model.nets = buildNets(model);
  return model;
}

function expandTabs(line) {
  let out = '';
  for (const c of line) out += c === '\t' ? ' '.repeat(8 - (out.length % 8)) : c;
  return out.replace(/\s+$/, '');
}

/* Rectangles drawn as +----+ / |    | / +----+ : modules, chips, tubes. Used for fills and 3-D blocks. */
function findBoxes(ch, rows, cols) {
  const boxes = [];
  const corner = (s) => s === '+' || s === '.' || s === "'";
  for (let r = 0; r < rows; r++) {
    for (let c = 0; c < cols; c++) {
      if (!corner(ch(r, c)) || ch(r, c + 1) !== '-' || ch(r + 1, c) !== '|') continue;
      for (let c2 = c + 2; c2 < cols && '-+.'.includes(ch(r, c2)); c2++) {
        if (!corner(ch(r, c2)) || ch(r + 1, c2) !== '|') continue;
        let r2 = r + 1;
        while (r2 < rows && '|+'.includes(ch(r2, c)) && ch(r2, c) !== ' ' && !(corner(ch(r2, c)) && ch(r2, c + 1) === '-')) r2++;
        if (r2 >= rows || !corner(ch(r2, c)) || r2 - r < 2 || c2 - c < 3) continue;
        let ok = true;
        for (let k = c + 1; k < c2 && ok; k++) ok = '-+'.includes(ch(r2, k));
        for (let k = r + 1; k < r2 && ok; k++) ok = '|+'.includes(ch(k, c2)) && ch(k, c2) !== ' ';
        if (ok) { boxes.push({ r1: r, c1: c, r2, c2 }); break; }
      }
    }
  }
  return boxes;
}

/* Union-find over segment endpoints: each connected net gets an id (for hover and 3-D colors). */
function buildNets(model) {
  const parent = new Map();
  const key = (x, y) => `${Math.round(x * 4)},${Math.round(y * 4)}`;
  const find = (k) => {
    if (!parent.has(k)) parent.set(k, k);
    let p = parent.get(k);
    if (p !== k) { p = find(p); parent.set(k, p); }
    return p;
  };
  const union = (a, b) => { const A = find(a), B = find(b); if (A !== B) parent.set(A, B); };
  const items = [
    ...model.segs.map((s) => [key(s.x1, s.y1), key(s.x2, s.y2)]),
    ...model.curves.map((cv) => [key(...cv.a), key(...cv.b)]),
  ];
  for (const [a, b] of items) union(a, b);
  const ids = new Map();
  const netOf = (k) => { const root = find(k); if (!ids.has(root)) ids.set(root, ids.size); return ids.get(root); };
  model.segs.forEach((s) => { s.net = netOf(key(s.x1, s.y1)); });
  model.curves.forEach((cv) => { cv.net = netOf(key(...cv.a)); });
  return ids.size;
}

/* ------------------------------------------------------------------ SVG */

export const CELL_W = 10;
export const CELL_H = 20;

export function toSVG(model, { idPrefix = 's', title = '' } = {}) {
  const W = CELL_W, H = CELL_H, pad = 12;
  const vw = model.cols * W + pad * 2, vh = model.rows * H + pad * 2;
  const X = (x) => +(x * W + pad).toFixed(2), Y = (y) => +(y * H + pad).toFixed(2);

  const byNet = new Map();
  const add = (net, d) => { if (!byNet.has(net)) byNet.set(net, []); byNet.get(net).push(d); };
  for (const s of mergeSegs(model.segs)) {
    if (s.type === 'node') continue;
    if (s.type === 'stub') { add(s.net, `M${X(s.x1)} ${Y(s.y1)}L${X(s.x2)} ${Y(s.y2)}`); continue; }
    if (s.type === 'hop') {
      const y = Y(s.y1), x1 = X(s.x1), x2 = X(s.x2), m = (x1 + x2) / 2, rr = W * 0.32;
      add(s.net, `M${x1} ${y}H${m - rr}A${rr} ${rr} 0 0 1 ${m + rr} ${y}H${x2}`);
    } else {
      add(s.net, `M${X(s.x1)} ${Y(s.y1)}L${X(s.x2)} ${Y(s.y2)}`);
    }
  }
  for (const cv of model.curves) {
    add(cv.net, cv.d.replace(/(-?\d*\.?\d+) (-?\d*\.?\d+)/g, (_, a, b) => `${X(+a)} ${Y(+b)}`));
  }

  const out = [];
  out.push(`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${vw} ${vh}" class="schem-svg" role="img" aria-label="${esc(title || 'Circuit diagram')}">`);
  out.push(`<defs><filter id="${idPrefix}-glow" x="-20%" y="-20%" width="140%" height="140%"><feGaussianBlur stdDeviation="2.2" result="b"/><feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter></defs>`);
  out.push('<g class="schem-boxes">');
  for (const b of model.boxes) {
    out.push(`<rect x="${X(b.c1 + 0.5)}" y="${Y(b.r1 + 0.5)}" width="${(b.c2 - b.c1) * W}" height="${(b.r2 - b.r1) * H}" rx="3"/>`);
  }
  out.push('</g><g class="schem-wires">');
  for (const [net, ds] of byNet) {
    const d = ds.join('');
    out.push(`<path class="wire" data-net="${net}" d="${d}" pathLength="1"/>`);
  }
  out.push('</g><g class="schem-flow" aria-hidden="true">');
  for (const [net, ds] of byNet) {
    if (ds.length < 2 && ds[0].length < 30) continue;
    out.push(`<path class="flow" data-net="${net}" d="${ds.join('')}" filter="url(#${idPrefix}-glow)"/>`);
  }
  out.push('</g><g class="schem-marks">');
  for (const [x, y] of model.dots) out.push(`<circle class="dot" cx="${X(x)}" cy="${Y(y)}" r="3"/>`);
  for (const [x, y] of model.terms) out.push(`<circle class="term" cx="${X(x)}" cy="${Y(y)}" r="${W * 0.28}"/>`);
  for (const [x, y, a] of model.arrows) {
    out.push(`<path class="arrow" d="M-4 -4L4 0L-4 4Z" transform="translate(${X(x)} ${Y(y)}) rotate(${a})"/>`);
  }
  out.push('</g><g class="schem-text">');
  for (const t of model.text) {
    const s = t.s.replace(/\s+$/, '');
    out.push(`<text x="${X(t.c)}" y="${Y(t.r + 0.72)}" textLength="${s.length * W}" lengthAdjust="spacingAndGlyphs">${esc(s)}</text>`);
  }
  out.push('</g></svg>');
  return out.join('');
}

/* Join touching collinear horizontal / vertical pieces so wires draw as long smooth strokes. */
function mergeSegs(segs) {
  const out = [], hz = new Map(), vt = new Map();
  for (const s of segs) {
    if ((s.type === 'w') && s.y1 === s.y2 && s.x1 !== s.x2) {
      const k = `${s.y1}|${s.net}`; (hz.get(k) || hz.set(k, []).get(k)).push([Math.min(s.x1, s.x2), Math.max(s.x1, s.x2)]);
    } else if ((s.type === 'w' || s.type === 'core') && s.x1 === s.x2 && s.y1 !== s.y2) {
      const k = `${s.x1}|${s.net}|${s.type}`; (vt.get(k) || vt.set(k, []).get(k)).push([Math.min(s.y1, s.y2), Math.max(s.y1, s.y2)]);
    } else out.push(s);
  }
  const merge = (list) => {
    list.sort((a, b) => a[0] - b[0]);
    const res = [];
    for (const [a, b] of list) {
      if (res.length && a <= res[res.length - 1][1] + 1e-6) res[res.length - 1][1] = Math.max(res[res.length - 1][1], b);
      else res.push([a, b]);
    }
    return res;
  };
  for (const [k, list] of hz) {
    const [y, net] = k.split('|').map(Number);
    for (const [a, b] of merge(list)) out.push({ x1: a, y1: y, x2: b, y2: y, type: 'w', net });
  }
  for (const [k, list] of vt) {
    const [x, net, type] = k.split('|');
    for (const [a, b] of merge(list)) out.push({ x1: +x, y1: a, x2: +x, y2: b, type, net: +net });
  }
  return out;
}

function esc(s) {
  return String(s).replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]));
}

/* ------------------------------------------------------------------ behaviour */

/* Mount an animated schematic into `el`. Returns the model. */
export function mount(el, ascii, opts = {}) {
  const model = parse(ascii);
  el.innerHTML = toSVG(model, opts);
  const svg = el.querySelector('svg');
  const wires = svg.querySelectorAll('.wire');
  wires.forEach((w) => {
    w.addEventListener('pointerenter', () => highlight(svg, w.dataset.net));
    w.addEventListener('pointerleave', () => highlight(svg, null));
  });
  if (opts.animate !== false) {
    const io = new IntersectionObserver((entries) => {
      for (const e of entries) if (e.isIntersecting) { svg.classList.add('drawn'); io.disconnect(); }
    }, { threshold: 0.15 });
    io.observe(svg);
  } else svg.classList.add('drawn', 'no-anim');
  if (opts.flow) svg.classList.add('flowing');
  return model;
}

function highlight(svg, net) {
  svg.classList.toggle('has-hot', net != null);
  svg.querySelectorAll('[data-net]').forEach((p) => p.classList.toggle('hot', p.dataset.net === net));
}
