/*
 * Shared behaviour for the companion pages: theme, menu, reveal-on-scroll, count-up numbers, search.
 * <html data-root="./"> (or "../") tells this script where the site root is.
 */
(function () {
  'use strict';
  var root = document.documentElement;
  var ROOT = root.getAttribute('data-root') || './';
  var THEME_KEY = 'quake-viz-theme'; // shared with acamaro.com, so one choice covers both sites

  // ------------------------------------------------------------ theme
  var toggle = document.getElementById('themeToggle');
  function paintIcon() {
    if (!toggle) return;
    var dark = root.classList.contains('dark-mode');
    toggle.innerHTML = dark
      ? '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><circle cx="12" cy="12" r="4.5"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/></svg>'
      : '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 12.8A9 9 0 1 1 11.2 3a7 7 0 0 0 9.8 9.8z"/></svg>';
    toggle.setAttribute('aria-label', dark ? 'Switch to light theme' : 'Switch to dark theme');
  }
  paintIcon();
  if (toggle) toggle.addEventListener('click', function () {
    var dark = !root.classList.contains('dark-mode');
    root.classList.toggle('dark-mode', dark);
    try { localStorage.setItem(THEME_KEY, dark ? 'dark' : 'light'); } catch (e) { /* ignore */ }
    paintIcon();
    window.dispatchEvent(new CustomEvent('themechange', { detail: { dark: dark } }));
  });

  // ------------------------------------------------------------ menu
  var menuBtn = document.getElementById('menuBtn');
  var nav = document.getElementById('nav');
  if (menuBtn && nav) menuBtn.addEventListener('click', function () {
    var open = nav.classList.toggle('open');
    menuBtn.setAttribute('aria-expanded', String(open));
  });

  // ------------------------------------------------------------ reveal + count-up
  var reduce = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  function countUp(el) {
    var target = parseInt(el.getAttribute('data-count'), 10);
    if (!target || reduce) { el.textContent = (target || 0).toLocaleString(); return; }
    var t0 = performance.now(), dur = 1400;
    (function step(t) {
      var k = Math.min(1, (t - t0) / dur);
      el.textContent = Math.round(target * (1 - Math.pow(1 - k, 3))).toLocaleString();
      if (k < 1) requestAnimationFrame(step);
    })(t0);
  }
  window.observeReveal = function (scope) {
    var els = (scope || document).querySelectorAll('.reveal, .reveal-stagger, [data-count]');
    if (!('IntersectionObserver' in window)) {
      els.forEach(function (el) { el.classList.add('in'); if (el.hasAttribute('data-count')) countUp(el); });
      return;
    }
    var io = new IntersectionObserver(function (entries) {
      entries.forEach(function (e) {
        if (!e.isIntersecting) return;
        e.target.classList.add('in');
        if (e.target.hasAttribute('data-count')) countUp(e.target);
        io.unobserve(e.target);
      });
    }, { threshold: 0.12, rootMargin: '0px 0px -40px 0px' });
    els.forEach(function (el) { io.observe(el); });
  };
  window.observeReveal();

  // ------------------------------------------------------------ search
  var dialog = document.getElementById('searchDialog');
  var openers = document.querySelectorAll('[data-search-open]');
  var input, list, index = null, sel = -1;
  function loadIndex() {
    if (index) return Promise.resolve(index);
    return fetch(ROOT + 'companion/data/search.json').then(function (r) { return r.json(); }).then(function (pages) {
      index = [];
      pages.forEach(function (p) {
        if (p.c === 'gallery') { index.push({ t: p.t, sub: 'Photo gallery', href: p.p, k: p.t.toLowerCase(), w: 0.6 }); return; }
        index.push({ t: p.t, sub: p.p, href: p.p, k: p.t.toLowerCase(), w: 1.2 });
        p.h.forEach(function (h) {
          index.push({ t: h[0], sub: p.t, href: p.p + (h[1] ? '#' + h[1] : ''), k: h[0].toLowerCase(), w: 1 });
        });
      });
      return index;
    });
  }
  function esc(s) { return s.replace(/[&<>"]/g, function (c) { return { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[c]; }); }
  function mark(s, words) {
    var out = esc(s);
    words.forEach(function (w) {
      if (w.length < 2) return;
      out = out.replace(new RegExp('(' + w.replace(/[.*+?^${}()|[\]\\]/g, '\\$&') + ')', 'ig'), '<mark>$1</mark>');
    });
    return out;
  }
  function run(q) {
    var words = q.toLowerCase().split(/\s+/).filter(Boolean);
    if (!words.length) { list.innerHTML = '<li><small style="padding:10px 12px">Search titles and section headings across every FAQ page.</small></li>'; return; }
    var res = [];
    for (var i = 0; i < index.length; i++) {
      var it = index[i], s = 0, ok = true;
      for (var j = 0; j < words.length; j++) {
        var at = it.k.indexOf(words[j]);
        if (at < 0) { ok = false; break; }
        s += at === 0 ? 3 : (it.k.charAt(at - 1) === ' ' ? 2 : 1);
      }
      if (ok) res.push([s * it.w - it.k.length / 200, it]);
      if (res.length > 4000) break;
    }
    res.sort(function (a, b) { return b[0] - a[0]; });
    sel = -1;
    list.innerHTML = res.slice(0, 60).map(function (r) {
      return '<li><a href="' + ROOT + 'archive/' + encodeURI(r[1].href).replace(/%23/g, '#') + '">' + mark(r[1].t, words) + '<small>' + esc(r[1].sub) + '</small></a></li>';
    }).join('') || '<li><small style="padding:10px 12px">No matches.</small></li>';
  }
  if (dialog) {
    input = dialog.querySelector('input');
    list = dialog.querySelector('.search-results');
    openers.forEach(function (b) {
      b.addEventListener('click', function () {
        dialog.showModal();
        input.focus();
        loadIndex().then(function () { run(input.value); });
      });
    });
    input.addEventListener('input', function () { if (index) run(input.value); });
    dialog.querySelector('form').addEventListener('submit', function (e) {
      e.preventDefault();
      var links = list.querySelectorAll('a');
      var a = links[Math.max(0, sel)];
      if (a) location.href = a.href;
    });
    input.addEventListener('keydown', function (e) {
      var links = list.querySelectorAll('a');
      if (e.key === 'ArrowDown' || e.key === 'ArrowUp') {
        e.preventDefault();
        sel = Math.max(0, Math.min(links.length - 1, sel + (e.key === 'ArrowDown' ? 1 : -1)));
        links.forEach(function (a, i) { a.classList.toggle('sel', i === sel); });
        if (links[sel]) links[sel].scrollIntoView({ block: 'nearest' });
      }
    });
    dialog.addEventListener('click', function (e) { if (e.target === dialog) dialog.close(); });
    document.addEventListener('keydown', function (e) {
      if ((e.key === '/' || (e.key === 'k' && (e.metaKey || e.ctrlKey))) && !dialog.open && !/input|textarea/i.test(document.activeElement.tagName)) {
        e.preventDefault();
        openers[0] && openers[0].click();
      }
    });
  }
})();
