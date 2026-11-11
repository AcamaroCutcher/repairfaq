/*
 * schematic3d.js: the same model that schematic.js builds, shown as a 3-D circuit board.
 * Wires are copper traces raised off the board, each electrically connected net has its own tint,
 * boxes (+----+) become translucent modules, crossings hop over, and particles of "current" run
 * along every net. Loaded only when someone opens the 3-D tab.
 */
import * as THREE from '../vendor/three.module.min.js';
import { OrbitControls } from '../vendor/OrbitControls.js';
import { RoomEnvironment } from '../vendor/RoomEnvironment.js';
import { parse } from './schematic.js';

const SX = 0.5;   // world units per character column
const SZ = 1.0;   // world units per row (characters are about twice as tall as wide)
const BOARD_TOP = 0.15;
const WIRE_Y = BOARD_TOP + 0.09;

export function mount3D(container, ascii, opts = {}) {
  const model = parse(ascii);
  const reduce = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  const isDark = () => document.documentElement.classList.contains('dark-mode');

  const W = model.cols * SX, H = model.rows * SZ;
  const ox = -W / 2, oz = -H / 2;
  const P = (x, y) => new THREE.Vector3(ox + x * SX, WIRE_Y, oz + y * SZ);

  const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setPixelRatio(Math.min(2, window.devicePixelRatio || 1));
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;
  container.appendChild(renderer.domElement);

  const scene = new THREE.Scene();
  const pmrem = new THREE.PMREMGenerator(renderer);
  scene.environment = pmrem.fromScene(new RoomEnvironment(renderer), 0.04).texture;
  const camera = new THREE.PerspectiveCamera(38, 1, 0.1, 1000);
  const span = Math.max(W, H * 1.2, 8);
  // far enough back that the whole board fits, whatever its proportions
  const fit = (0.5 * Math.hypot(W + 2, H + 2)) / Math.tan((camera.fov * Math.PI) / 360);
  const home = () => camera.position.set(fit * 0.03, fit * 0.68, fit * 0.6);
  home();

  const controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;
  controls.dampingFactor = 0.08;
  controls.maxPolarAngle = Math.PI * 0.47;
  controls.minDistance = span * 0.25;
  controls.maxDistance = fit * 3;
  controls.autoRotate = !reduce;
  controls.autoRotateSpeed = 0.45;
  controls.addEventListener('start', () => { controls.autoRotate = false; });

  // ------------------------------------------------------------ lights
  const hemi = new THREE.HemisphereLight(0xffffff, 0x223322, 1.1);
  scene.add(hemi);
  const sun = new THREE.DirectionalLight(0xffffff, 2.2);
  sun.position.set(-W * 0.4, span * 1.2, H * 0.6);
  sun.castShadow = true;
  sun.shadow.mapSize.set(2048, 2048);
  const sc = sun.shadow.camera;
  sc.left = -W / 2 - 2; sc.right = W / 2 + 2; sc.top = H / 2 + 2; sc.bottom = -H / 2 - 2;
  sc.near = 0.5; sc.far = span * 4;
  scene.add(sun);
  const rim = new THREE.PointLight(0xff5a3c, 1.2, span * 2.5);
  rim.position.set(W * 0.45, span * 0.3, -H * 0.5);
  scene.add(rim);

  // ------------------------------------------------------------ board
  const boardMat = new THREE.MeshStandardMaterial({ roughness: 0.75, metalness: 0.05 });
  const board = new THREE.Mesh(new THREE.BoxGeometry(W + 2, 0.3, H + 2), boardMat);
  board.receiveShadow = true;
  scene.add(board);
  const grid = new THREE.GridHelper(Math.max(W, H) + 2, Math.round((Math.max(W, H) + 2) / SZ), 0xffffff, 0xffffff);
  grid.material.transparent = true;
  grid.position.y = BOARD_TOP + 0.002;
  grid.scale.set((W + 2) / (Math.max(W, H) + 2), 1, (H + 2) / (Math.max(W, H) + 2));
  scene.add(grid);

  // ------------------------------------------------------------ wires
  const pieces = []; // [Vector3 a, Vector3 b, net, kind]
  for (const s of model.segs) {
    if (s.type === 'node') continue;
    const a = P(s.x1, s.y1), b = P(s.x2, s.y2);
    if (s.type === 'hop') {
      const m1 = a.clone().lerp(b, 0.3), m2 = a.clone().lerp(b, 0.7);
      m1.y += 0.35; m2.y += 0.35;
      pieces.push([a, m1, s.net, 'w'], [m1, m2, s.net, 'w'], [m2, b, s.net, 'w']);
    } else pieces.push([a, b, s.net, s.type === 'core' ? 'core' : 'w']);
  }
  for (const cv of model.curves) {
    const m = cv.d.match(/-?\d*\.?\d+/g).map(Number); // M ax ay Q qx qy bx by
    const A = P(m[0], m[1]), Q = P(m[2], m[3]), B = P(m[4], m[5]);
    let prev = A;
    for (let i = 1; i <= 5; i++) {
      const t = i / 5;
      const p = A.clone().multiplyScalar((1 - t) * (1 - t)).add(Q.clone().multiplyScalar(2 * (1 - t) * t)).add(B.clone().multiplyScalar(t * t));
      if (cv.coil) p.y += Math.sin(t * Math.PI) * 0.22;
      pieces.push([prev, p, cv.net, cv.coil ? 'coil' : 'w']);
      prev = p;
    }
  }

  const netColor = (net) => new THREE.Color().setHSL(((net * 0.137) % 1), 0.75, 0.6);
  const copper = new THREE.Color(0xd98c4a);
  const wireGeo = new THREE.CylinderGeometry(0.075, 0.075, 1, 10, 1);
  const wireMat = new THREE.MeshStandardMaterial({ metalness: 0.7, roughness: 0.28, emissive: 0x000000 });
  const wires = new THREE.InstancedMesh(wireGeo, wireMat, Math.max(1, pieces.length));
  wires.castShadow = true;
  const up = new THREE.Vector3(0, 1, 0), m4 = new THREE.Matrix4(), q = new THREE.Quaternion();
  pieces.forEach(([a, b, net, kind], i) => {
    const d = b.clone().sub(a), len = Math.max(d.length(), 0.001);
    q.setFromUnitVectors(up, d.clone().normalize());
    m4.compose(a.clone().add(b).multiplyScalar(0.5), q, new THREE.Vector3(kind === 'core' ? 0.6 : 1, len + 0.07, kind === 'core' ? 0.6 : 1));
    wires.setMatrixAt(i, m4);
    const col = kind === 'core' ? new THREE.Color(0x8a8f98) : copper.clone().lerp(netColor(net), 0.55);
    wires.setColorAt(i, col);
  });
  scene.add(wires);

  // junction dots, terminals, arrows
  const dotMesh = new THREE.InstancedMesh(new THREE.SphereGeometry(0.17, 16, 12), new THREE.MeshStandardMaterial({ color: 0xf1c27d, metalness: 0.8, roughness: 0.25 }), Math.max(1, model.dots.length));
  model.dots.forEach(([x, y], i) => { m4.makeTranslation(P(x, y).x, WIRE_Y, P(x, y).z); dotMesh.setMatrixAt(i, m4); });
  dotMesh.count = model.dots.length;
  scene.add(dotMesh);

  const termMesh = new THREE.InstancedMesh(new THREE.TorusGeometry(0.16, 0.055, 10, 24), new THREE.MeshStandardMaterial({ color: 0xe8e2d0, metalness: 0.9, roughness: 0.2 }), Math.max(1, model.terms.length));
  const flat = new THREE.Quaternion().setFromEuler(new THREE.Euler(Math.PI / 2, 0, 0));
  model.terms.forEach(([x, y], i) => { m4.compose(P(x, y), flat, new THREE.Vector3(1, 1, 1)); termMesh.setMatrixAt(i, m4); });
  termMesh.count = model.terms.length;
  termMesh.castShadow = true;
  scene.add(termMesh);

  const arrowMesh = new THREE.InstancedMesh(new THREE.ConeGeometry(0.16, 0.4, 12), new THREE.MeshStandardMaterial({ color: 0xffd08a, metalness: 0.5, roughness: 0.4 }), Math.max(1, model.arrows.length));
  model.arrows.forEach(([x, y, ang], i) => {
    const rot = new THREE.Quaternion().setFromEuler(new THREE.Euler(0, -ang * Math.PI / 180, -Math.PI / 2));
    m4.compose(P(x, y), rot, new THREE.Vector3(1, 1, 1));
    arrowMesh.setMatrixAt(i, m4);
  });
  arrowMesh.count = model.arrows.length;
  scene.add(arrowMesh);

  // ------------------------------------------------------------ modules (+----+ boxes)
  const boxGroup = new THREE.Group();
  const boxH = 0.75;
  const boxMats = [];
  for (const b of model.boxes) {
    const w = (b.c2 - b.c1) * SX - 0.12, h = (b.r2 - b.r1) * SZ - 0.12;
    const cx = ox + (b.c1 + b.c2 + 1) / 2 * SX, cz = oz + (b.r1 + b.r2 + 1) / 2 * SZ;
    const mat = new THREE.MeshPhysicalMaterial({ color: 0x1b2a2a, roughness: 0.35, metalness: 0.1, transparent: true, opacity: 0.55, clearcoat: 1 });
    boxMats.push(mat);
    const mesh = new THREE.Mesh(new THREE.BoxGeometry(w, boxH, h), mat);
    mesh.position.set(cx, BOARD_TOP + boxH / 2, cz);
    mesh.castShadow = true;
    boxGroup.add(mesh);
    const edges = new THREE.LineSegments(new THREE.EdgesGeometry(mesh.geometry), new THREE.LineBasicMaterial({ color: 0x3dd6c3, transparent: true, opacity: 0.85 }));
    edges.position.copy(mesh.position);
    boxGroup.add(edges);
  }
  scene.add(boxGroup);

  // ------------------------------------------------------------ labels (text drawn on canvases)
  const inBox = (r, c) => model.boxes.findIndex((b) => r > b.r1 && r < b.r2 && c > b.c1 && c < b.c2);
  const PX = 14; // canvas pixels per column
  const textPlanes = [];
  function textCanvas(cols, rows, runs, color) {
    const scale = Math.min(1, 4096 / (cols * PX), 4096 / (rows * PX * 2));
    const cw = PX * scale, chh = PX * 2 * scale;
    const cv = document.createElement('canvas');
    cv.width = Math.max(2, Math.ceil(cols * cw)); cv.height = Math.max(2, Math.ceil(rows * chh));
    const g = cv.getContext('2d');
    g.font = `600 ${Math.round(Math.min(chh * 0.8, cw * 1.62))}px "JetBrains Mono", Menlo, monospace`;
    g.textAlign = 'center';
    g.fillStyle = color;
    g.textBaseline = 'middle';
    for (const t of runs) {
      for (let i = 0; i < t.s.length; i++) {
        if (t.s[i] !== ' ') g.fillText(t.s[i], (t.c + i + 0.5) * cw, (t.r + 0.55) * chh);
      }
    }
    const tex = new THREE.CanvasTexture(cv);
    tex.colorSpace = THREE.SRGBColorSpace;
    tex.anisotropy = 8;
    return tex;
  }
  function buildText() {
    textPlanes.forEach((m) => { scene.remove(m); m.geometry.dispose(); m.material.map.dispose(); m.material.dispose(); });
    textPlanes.length = 0;
    const color = isDark() ? '#eaf6ee' : '#f4fff8';
    const outside = [], inside = model.boxes.map(() => []);
    for (const t of model.text) {
      for (let i = 0; i < t.s.length; i++) {
        const k = inBox(t.r, t.c + i);
        (k >= 0 ? inside[k] : outside).push({ r: t.r, c: t.c + i, s: t.s[i] });
      }
    }
    const mk = (cols, rows, runs, x, y, z) => {
      const mat = new THREE.MeshBasicMaterial({ map: textCanvas(cols, rows, runs, color), transparent: true, depthWrite: false });
      const mesh = new THREE.Mesh(new THREE.PlaneGeometry(cols * SX, rows * SZ), mat);
      mesh.rotation.x = -Math.PI / 2;
      mesh.position.set(x, y, z);
      scene.add(mesh); textPlanes.push(mesh);
    };
    mk(model.cols, model.rows, outside, 0, BOARD_TOP + 0.01, 0);
    model.boxes.forEach((b, k) => {
      const cols = b.c2 - b.c1 - 1, rows = b.r2 - b.r1 - 1;
      if (!inside[k].length || cols < 1 || rows < 1) return;
      const runs = inside[k].map((t) => ({ r: t.r - b.r1 - 1, c: t.c - b.c1 - 1, s: t.s }));
      mk(cols, rows, runs, ox + (b.c1 + 1 + cols / 2) * SX, BOARD_TOP + boxH + 0.01, oz + (b.r1 + 1 + rows / 2) * SZ);
    });
  }

  function paint() {
    const dark = isDark();
    boardMat.color.set(dark ? 0x0d3326 : 0x1e5a43);
    grid.material.opacity = dark ? 0.07 : 0.1;
    boxMats.forEach((m) => m.color.set(dark ? 0x10201f : 0x24403d));
    buildText();
  }
  paint();
  const onTheme = () => paint();
  window.addEventListener('themechange', onTheme);
  if (document.fonts && document.fonts.ready) document.fonts.ready.then(() => { if (!disposed) buildText(); });

  // ------------------------------------------------------------ current particles
  const key = (v) => `${Math.round(v.x * 40)},${Math.round(v.z * 40)},${Math.round(v.y * 10)}`;
  const adj = new Map();
  const edges = pieces.filter((p) => p[3] !== 'core');
  edges.forEach((e, i) => {
    for (const end of [0, 1]) {
      const k = key(e[end]);
      if (!adj.has(k)) adj.set(k, []);
      adj.get(k).push(i);
    }
  });
  const NP = edges.length ? Math.min(420, Math.max(30, Math.round(edges.length * 1.2))) : 0;
  const pMat = new THREE.MeshBasicMaterial({ color: 0xff6a3d, transparent: true, opacity: 0.95, blending: THREE.AdditiveBlending, depthWrite: false, toneMapped: false });
  const parts = new THREE.InstancedMesh(new THREE.SphereGeometry(0.075, 10, 8), pMat, Math.max(1, NP));
  parts.count = NP;
  const halo = new THREE.InstancedMesh(new THREE.SphereGeometry(0.2, 10, 8), new THREE.MeshBasicMaterial({ color: 0xff3b2f, transparent: true, opacity: 0.18, blending: THREE.AdditiveBlending, depthWrite: false, toneMapped: false }), Math.max(1, NP));
  halo.count = NP;
  scene.add(parts, halo);
  const P_ = [];
  for (let i = 0; i < NP; i++) P_.push({ e: (Math.random() * edges.length) | 0, t: Math.random(), dir: Math.random() < 0.5 ? 1 : -1, v: 1.6 + Math.random() * 1.6 });
  const tmp = new THREE.Vector3();
  function stepParticles(dt) {
    for (let i = 0; i < NP; i++) {
      const p = P_[i];
      const e = edges[p.e];
      const len = Math.max(0.05, e[0].distanceTo(e[1]));
      p.t += (p.dir * p.v * dt) / len;
      if (p.t > 1 || p.t < 0) {
        const endPt = p.t > 1 ? e[1] : e[0];
        const opts2 = (adj.get(key(endPt)) || []).filter((j) => j !== p.e);
        if (opts2.length) {
          const j = opts2[(Math.random() * opts2.length) | 0];
          const n = edges[j];
          p.e = j;
          if (key(n[0]) === key(endPt)) { p.t = 0; p.dir = 1; } else { p.t = 1; p.dir = -1; }
        } else { p.dir *= -1; p.t = Math.min(1, Math.max(0, p.t)); }
      }
      const ee = edges[p.e];
      tmp.copy(ee[0]).lerp(ee[1], p.t);
      tmp.y += 0.03;
      m4.makeTranslation(tmp.x, tmp.y, tmp.z);
      parts.setMatrixAt(i, m4);
      halo.setMatrixAt(i, m4);
    }
    parts.instanceMatrix.needsUpdate = true;
    halo.instanceMatrix.needsUpdate = true;
  }

  // ------------------------------------------------------------ loop
  let running = true, visible = true, disposed = false, last = performance.now();
  let flow = opts.flow !== false;
  parts.visible = halo.visible = flow;
  function resize() {
    const w = container.clientWidth || 600, h = container.clientHeight || 400;
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
  }
  const ro = new ResizeObserver(resize);
  ro.observe(container);
  resize();
  const io = new IntersectionObserver((en) => { visible = en[0].isIntersecting; });
  io.observe(container);

  stepParticles(0);
  function frame(t) {
    if (disposed) return;
    requestAnimationFrame(frame);
    const dt = Math.min(0.05, (t - last) / 1000); last = t;
    if (!visible || !running) return;
    if (flow && !reduce) stepParticles(dt);
    rim.intensity = 1.0 + Math.sin(t / 700) * 0.3;
    controls.update();
    renderer.render(scene, camera);
  }
  requestAnimationFrame(frame);

  return {
    model,
    setFlow(on) { flow = on; parts.visible = halo.visible = on; },
    resetView() { home(); controls.target.set(0, 0, 0); },
    dispose() {
      disposed = true;
      ro.disconnect(); io.disconnect();
      window.removeEventListener('themechange', onTheme);
      controls.dispose();
      scene.traverse((o) => {
        if (o.geometry) o.geometry.dispose();
        if (o.material) [].concat(o.material).forEach((m) => { if (m.map) m.map.dispose(); m.dispose(); });
      });
      renderer.dispose();
      renderer.domElement.remove();
    },
  };
}
