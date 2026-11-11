/*
 * viewer.js: a diagram viewer with three tabs.
 *   Animated  the schematic redrawn as SVG; hover a wire to light up everything connected to it
 *   3-D       the same circuit as a board you can orbit, with current flowing (three.js, loaded on demand)
 *   Original  Sam's ASCII exactly as published, with a copy button
 */
import { mount } from './schematic.js';

let uid = 0;
const ICON = {
  flow: '<svg viewBox="0 0 24 24" width="15" height="15" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M13 2 4 14h7l-1 8 9-12h-7z"/></svg>',
  copy: '<svg viewBox="0 0 24 24" width="15" height="15" fill="none" stroke="currentColor" stroke-width="2"><rect x="9" y="9" width="12" height="12" rx="2"/><path d="M5 15V5a2 2 0 0 1 2-2h10"/></svg>',
  down: '<svg viewBox="0 0 24 24" width="15" height="15" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M12 3v12m0 0-5-5m5 5 5-5M4 21h16"/></svg>',
  reset: '<svg viewBox="0 0 24 24" width="15" height="15" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M3 12a9 9 0 1 0 3-6.7L3 8m0-5v5h5"/></svg>',
};

export function mountViewer(host, ascii, { title = 'diagram', initial = 'svg', flow = true, credit = 'Sci.Electronics.Repair FAQ / Sam\'s Laser FAQ, Copyright Samuel M. Goldwasser and Filip M. Gieszczykiewicz', source = 'https://www.repairfaq.org/' } = {}) {
  const id = `v${++uid}`;
  host.classList.add('viewer');
  host.innerHTML = `
    <div class="viewer-bar">
      <div class="tabs" role="tablist" aria-label="Diagram view">
        <button class="tab" role="tab" id="${id}-t-svg" aria-controls="${id}-svg" data-tab="svg">Animated</button>
        <button class="tab" role="tab" id="${id}-t-3d" aria-controls="${id}-3d" data-tab="3d">3D</button>
        <button class="tab" role="tab" id="${id}-t-ascii" aria-controls="${id}-ascii" data-tab="ascii">Original ASCII</button>
      </div>
      <div class="viewer-tools">
        <button class="tool" data-act="flow" aria-pressed="${flow}">${ICON.flow}Current</button>
        <button class="tool" data-act="reset" hidden>${ICON.reset}Reset view</button>
        <button class="tool" data-act="copy">${ICON.copy}Copy ASCII</button>
        <button class="tool" data-act="svg">${ICON.down}SVG</button>
      </div>
    </div>
    <div class="panel svg-panel" role="tabpanel" id="${id}-svg" aria-labelledby="${id}-t-svg"></div>
    <div class="panel three-panel" role="tabpanel" id="${id}-3d" aria-labelledby="${id}-t-3d" hidden>
      <div class="three-loading">Loading 3D view…</div>
      <div class="three-hint">Drag to orbit · scroll or pinch to zoom · right-drag to pan</div>
    </div>
    <div class="panel" role="tabpanel" id="${id}-ascii" aria-labelledby="${id}-t-ascii" hidden><pre class="ascii"></pre></div>`;

  const $ = (s) => host.querySelector(s);
  const svgPanel = $(`#${id}-svg`), threePanel = $(`#${id}-3d`), asciiPanel = $(`#${id}-ascii`);
  asciiPanel.querySelector('pre').textContent = ascii;
  mount(svgPanel, ascii, { idPrefix: id, title, flow });

  let three = null, flowing = flow, current = null;
  const flowBtn = $('[data-act="flow"]'), resetBtn = $('[data-act="reset"]');

  async function show(tab) {
    current = tab;
    host.querySelectorAll('.tab').forEach((t) => t.setAttribute('aria-selected', String(t.dataset.tab === tab)));
    svgPanel.hidden = tab !== 'svg';
    threePanel.hidden = tab !== '3d';
    asciiPanel.hidden = tab !== 'ascii';
    flowBtn.hidden = tab === 'ascii';
    resetBtn.hidden = tab !== '3d';
    if (tab === '3d' && !three) {
      try {
        const { mount3D } = await import('./schematic3d.js');
        three = mount3D(threePanel, ascii, { flow: flowing });
        threePanel.querySelector('.three-loading')?.remove();
      } catch (err) {
        threePanel.querySelector('.three-loading').textContent = '3D view needs WebGL, which this browser has turned off.';
        console.error(err);
      }
    }
  }
  host.querySelectorAll('.tab').forEach((t) => t.addEventListener('click', () => show(t.dataset.tab)));
  host.querySelector('.tabs').addEventListener('keydown', (e) => {
    if (e.key !== 'ArrowRight' && e.key !== 'ArrowLeft') return;
    const order = ['svg', '3d', 'ascii'];
    const next = order[(order.indexOf(current) + (e.key === 'ArrowRight' ? 1 : 2)) % 3];
    show(next); host.querySelector(`[data-tab="${next}"]`).focus();
  });

  flowBtn.addEventListener('click', () => {
    flowing = !flowing;
    flowBtn.setAttribute('aria-pressed', String(flowing));
    svgPanel.querySelector('svg')?.classList.toggle('flowing', flowing);
    three?.setFlow(flowing);
  });
  resetBtn.addEventListener('click', () => three?.resetView());
  $('[data-act="copy"]').addEventListener('click', async (e) => {
    const b = e.currentTarget;
    try { await navigator.clipboard.writeText(ascii); b.lastChild.textContent = 'Copied'; }
    catch { b.lastChild.textContent = 'Select the text instead'; show('ascii'); }
    setTimeout(() => { b.lastChild.textContent = 'Copy ASCII'; }, 1600);
  });
  $('[data-act="svg"]').addEventListener('click', () => {
    const svg = svgPanel.querySelector('svg').cloneNode(true);
    svg.classList.add('drawn', 'no-anim');
    const cs = getComputedStyle(host);
    const style = `<style>.wire{fill:none;stroke:${cs.getPropertyValue('--wire')};stroke-width:1.7;stroke-linecap:round;stroke-linejoin:round}.flow{display:none}.dot,.arrow{fill:${cs.getPropertyValue('--wire')}}.term{fill:#fff;stroke:${cs.getPropertyValue('--wire')};stroke-width:1.6}text{font-family:'JetBrains Mono',Menlo,monospace;font-size:15px;fill:${cs.getPropertyValue('--ink-2')};white-space:pre}.schem-boxes rect{fill:rgba(15,118,110,.07)}</style>`;
    svg.insertAdjacentHTML('afterbegin', style + `<!-- Rendered from an ASCII diagram in: ${credit.replace(/--/g, '-')}. Source: ${source} . Non-commercial use only, with acknowledgment, per https://www.repairfaq.org/sam/copyright.htm -->`);
    const blob = new Blob([svg.outerHTML], { type: 'image/svg+xml' });
    const a = Object.assign(document.createElement('a'), { href: URL.createObjectURL(blob), download: `${title.replace(/[^a-z0-9]+/gi, '-').toLowerCase().slice(0, 60) || 'diagram'}.svg` });
    a.click();
    setTimeout(() => URL.revokeObjectURL(a.href), 2000);
  });

  show(initial);
  return { show };
}
