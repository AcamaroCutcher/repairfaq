/*
 * hero3d.js: an animated 3-D helium-neon laser tube for the landing page.
 * Parts follow the description in Sam's Laser FAQ (archive/sam/laserhen.htm): a glass envelope,
 * a thin capillary bore where the He-Ne discharge glows, an aluminum cathode can, an anode,
 * a high reflector (HR) mirror at one end and the output coupler (OC) at the other, and the
 * 632.8 nm red beam. Photons bounce between the mirrors; a small fraction leaks out as the beam.
 */
import * as THREE from '../vendor/three.module.min.js';
import { OrbitControls } from '../vendor/OrbitControls.js';
import { RoomEnvironment } from '../vendor/RoomEnvironment.js';

export function mountHero(container, labelEl) {
  const reduce = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setPixelRatio(Math.min(2, window.devicePixelRatio || 1));
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  container.appendChild(renderer.domElement);

  const scene = new THREE.Scene();
  const pmrem = new THREE.PMREMGenerator(renderer);
  scene.environment = pmrem.fromScene(new RoomEnvironment(renderer), 0.04).texture;
  const camera = new THREE.PerspectiveCamera(32, 1, 0.1, 200);
  camera.position.set(4.5, 4.2, 15);

  const controls = new OrbitControls(camera, renderer.domElement);
  controls.enableZoom = false;
  controls.enablePan = false;
  controls.enableDamping = true;
  controls.autoRotate = !reduce;
  controls.autoRotateSpeed = 0.6;
  controls.minPolarAngle = Math.PI * 0.2;
  controls.maxPolarAngle = Math.PI * 0.7;
  controls.target.set(1.2, 0, 0);
  controls.addEventListener('start', () => { controls.autoRotate = false; });

  scene.add(new THREE.HemisphereLight(0xffffff, 0x302018, 1.2));
  const key = new THREE.DirectionalLight(0xffffff, 2.4);
  key.position.set(5, 8, 6);
  scene.add(key);
  const glowLight = new THREE.PointLight(0xff3b4a, 6, 14, 1.6);
  scene.add(glowLight);

  const laser = new THREE.Group();
  laser.rotation.z = 0.06;
  scene.add(laser);
  const pickable = [];
  const named = (mesh, label) => { mesh.userData.label = label; pickable.push(mesh); return mesh; };
  const X = (len, r, seg = 48, open = false) => {
    const g = new THREE.CylinderGeometry(r, r, len, seg, 1, open);
    g.rotateZ(Math.PI / 2);
    return g;
  };

  const L = 8; // tube length
  // glass envelope
  const glass = new THREE.MeshPhysicalMaterial({ color: 0xdfeff5, roughness: 0.05, metalness: 0, transmission: 0.0, transparent: true, opacity: 0.16, clearcoat: 1, side: THREE.DoubleSide, depthWrite: false });
  laser.add(named(new THREE.Mesh(X(L, 0.95, 64, true), glass), 'Glass envelope: filled with helium and neon at low pressure'));
  for (const s of [-1, 1]) {
    const cap = new THREE.Mesh(new THREE.SphereGeometry(0.95, 48, 16, 0, Math.PI * 2, 0, Math.PI / 2), glass);
    cap.rotation.z = -s * Math.PI / 2; cap.scale.y = 0.25; cap.position.x = s * L / 2;
    laser.add(cap);
  }
  // capillary bore and its glow
  const boreMat = new THREE.MeshBasicMaterial({ color: 0xff6b7a, toneMapped: false });
  laser.add(named(new THREE.Mesh(X(L - 0.6, 0.07, 24), boreMat), 'Capillary bore: the helium-neon discharge glows here'));
  const boreGlass = new THREE.MeshPhysicalMaterial({ color: 0xeef6fa, roughness: 0.1, transparent: true, opacity: 0.22, depthWrite: false });
  laser.add(new THREE.Mesh(X(L - 0.6, 0.2, 32), boreGlass));
  const haloMat = new THREE.MeshBasicMaterial({ color: 0xff2d55, transparent: true, opacity: 0.16, blending: THREE.AdditiveBlending, depthWrite: false, toneMapped: false });
  const halo = new THREE.Mesh(X(L - 0.6, 0.38, 32), haloMat);
  laser.add(halo);
  // cathode can
  const alu = new THREE.MeshStandardMaterial({ color: 0xc9cdd2, metalness: 0.92, roughness: 0.32 });
  const cathode = named(new THREE.Mesh(X(3.6, 0.62, 48, true), alu), 'Cathode: an aluminum can that supplies electrons');
  cathode.material.side = THREE.DoubleSide;
  cathode.position.x = -1.7;
  laser.add(cathode);
  // anode pin
  const metal = new THREE.MeshStandardMaterial({ color: 0x9aa0a6, metalness: 0.95, roughness: 0.25 });
  const anode = named(new THREE.Mesh(new THREE.CylinderGeometry(0.07, 0.07, 1.0, 16), metal), 'Anode: the positive high-voltage connection');
  anode.position.set(3.0, 0.75, 0);
  laser.add(anode);
  // mirror mounts and mirrors
  for (const s of [-1, 1]) {
    const mount = new THREE.Mesh(X(0.5, 0.42, 40), metal);
    mount.position.x = s * (L / 2 + 0.25);
    laser.add(mount);
  }
  const hr = named(new THREE.Mesh(X(0.06, 0.3, 40), new THREE.MeshStandardMaterial({ color: 0xffd9a0, metalness: 1, roughness: 0.05 })), 'High reflector (HR) mirror: usually better than 99.9% reflective');
  hr.position.x = -(L / 2 + 0.53);
  const oc = named(new THREE.Mesh(X(0.06, 0.3, 40), new THREE.MeshPhysicalMaterial({ color: 0xff8f8f, metalness: 0.3, roughness: 0.05, transparent: true, opacity: 0.85, clearcoat: 1 })), 'Output coupler (OC) mirror: about 99% to 99.3% reflective; the rest becomes the beam');
  oc.position.x = L / 2 + 0.53;
  laser.add(hr, oc);
  // leads to the power supply
  const lead = (from, to) => {
    const curve = new THREE.CatmullRomCurve3([from, from.clone().add(new THREE.Vector3(0, 0.8, 0)), to.clone().add(new THREE.Vector3(0, 1.5, 0)), to]);
    return new THREE.Mesh(new THREE.TubeGeometry(curve, 40, 0.045, 8), new THREE.MeshStandardMaterial({ color: 0xc23a2b, roughness: 0.5 }));
  };
  laser.add(lead(new THREE.Vector3(3.0, 1.25, 0), new THREE.Vector3(1.8, -2.6, 1.5)));
  laser.add(lead(new THREE.Vector3(-1.7, 0.6, 0), new THREE.Vector3(-0.2, -2.6, 1.5)));
  const psu = named(new THREE.Mesh(new THREE.BoxGeometry(3, 0.9, 1.4), new THREE.MeshStandardMaterial({ color: 0x24262b, metalness: 0.4, roughness: 0.5 })), 'HV power supply: 1,000 to 3,000 VDC at 3 to 8 mA to run, 5 to 12 kV to start');
  psu.position.set(0.8, -3.0, 1.5);
  laser.add(psu);

  // the beam
  const beamLen = 14;
  const stripes = document.createElement('canvas');
  stripes.width = 256; stripes.height = 4;
  const sg = stripes.getContext('2d');
  const grad = sg.createLinearGradient(0, 0, 256, 0);
  grad.addColorStop(0, 'rgba(255,40,30,0.55)'); grad.addColorStop(0.5, 'rgba(255,90,70,1)'); grad.addColorStop(1, 'rgba(255,40,30,0.55)');
  sg.fillStyle = grad; sg.fillRect(0, 0, 256, 4);
  const stripeTex = new THREE.CanvasTexture(stripes);
  stripeTex.wrapS = THREE.RepeatWrapping;
  stripeTex.repeat.set(6, 1);
  const beam = named(new THREE.Mesh(X(beamLen, 0.045, 16), new THREE.MeshBasicMaterial({ map: stripeTex, color: 0xff3b2f, toneMapped: false })), 'Laser beam: 632.8 nm red');
  beam.position.x = L / 2 + 0.56 + beamLen / 2;
  const beamGlow = new THREE.Mesh(X(beamLen, 0.16, 16), new THREE.MeshBasicMaterial({ color: 0xff2a1a, transparent: true, opacity: 0.22, blending: THREE.AdditiveBlending, depthWrite: false, toneMapped: false }));
  beamGlow.position.copy(beam.position);
  laser.add(beam, beamGlow);
  const spot = new THREE.Mesh(new THREE.CircleGeometry(0.35, 32), new THREE.MeshBasicMaterial({ color: 0xff4433, transparent: true, opacity: 0.6, blending: THREE.AdditiveBlending, depthWrite: false, toneMapped: false }));
  spot.rotation.y = -Math.PI / 2; spot.position.x = L / 2 + 0.56 + beamLen;
  laser.add(spot);

  // photons bouncing in the cavity
  const NPH = 90;
  const ph = new THREE.InstancedMesh(new THREE.SphereGeometry(0.045, 8, 6), new THREE.MeshBasicMaterial({ color: 0xff5040, blending: THREE.AdditiveBlending, transparent: true, toneMapped: false, depthWrite: false }), NPH);
  laser.add(ph);
  const xMin = -(L / 2 + 0.5), xMax = L / 2 + 0.5;
  const photons = Array.from({ length: NPH }, () => ({ x: xMin + Math.random() * (xMax - xMin), d: Math.random() < 0.5 ? 1 : -1, y: (Math.random() - 0.5) * 0.08, z: (Math.random() - 0.5) * 0.08, v: 5 + Math.random() * 4, out: false }));

  // dust
  const dustGeo = new THREE.BufferGeometry();
  const N = 500, pos = new Float32Array(N * 3);
  for (let i = 0; i < N; i++) { pos[i * 3] = (Math.random() - 0.5) * 40; pos[i * 3 + 1] = (Math.random() - 0.5) * 16; pos[i * 3 + 2] = (Math.random() - 0.5) * 20; }
  dustGeo.setAttribute('position', new THREE.BufferAttribute(pos, 3));
  const dust = new THREE.Points(dustGeo, new THREE.PointsMaterial({ color: 0xff7a6a, size: 0.05, transparent: true, opacity: 0.45, depthWrite: false }));
  scene.add(dust);

  // hover labels
  const ray = new THREE.Raycaster(), mouse = new THREE.Vector2();
  let hoverPt = null;
  renderer.domElement.addEventListener('pointermove', (e) => {
    const r = renderer.domElement.getBoundingClientRect();
    mouse.set(((e.clientX - r.left) / r.width) * 2 - 1, -((e.clientY - r.top) / r.height) * 2 + 1);
    ray.setFromCamera(mouse, camera);
    const hit = ray.intersectObjects(pickable, false)[0];
    if (hit && labelEl) {
      labelEl.textContent = hit.object.userData.label;
      labelEl.style.left = (e.clientX - r.left) + 'px';
      labelEl.style.top = (e.clientY - r.top) + 'px';
      labelEl.classList.add('on');
      hoverPt = hit;
    } else if (labelEl) { labelEl.classList.remove('on'); hoverPt = null; }
  });
  renderer.domElement.addEventListener('pointerleave', () => labelEl && labelEl.classList.remove('on'));

  function resize() {
    const w = container.clientWidth, h = container.clientHeight;
    renderer.setSize(w, h, false);
    camera.aspect = w / h;
    camera.position.z = w < 600 ? 21 : 15;
    camera.updateProjectionMatrix();
  }
  new ResizeObserver(resize).observe(container);
  resize();
  let visible = true;
  new IntersectionObserver((en) => { visible = en[0].isIntersecting; }).observe(container);

  const m4 = new THREE.Matrix4();
  let last = performance.now();
  function frame(t) {
    requestAnimationFrame(frame);
    if (!visible) return;
    const dt = Math.min(0.05, (t - last) / 1000); last = t;
    const flick = 0.85 + Math.sin(t / 90) * 0.04 + Math.sin(t / 37) * 0.03;
    haloMat.opacity = 0.14 * flick + 0.04;
    glowLight.intensity = 5 * flick;
    stripeTex.offset.x -= dt * 2.2;
    for (let i = 0; i < NPH; i++) {
      const p = photons[i];
      p.x += p.d * p.v * dt;
      if (p.out) {
        if (p.x > xMax + beamLen) { p.out = false; p.x = xMin + Math.random() * 2; p.d = 1; }
      } else if (p.x > xMax) {
        if (Math.random() < 0.06) p.out = true; else { p.x = xMax; p.d = -1; }
      } else if (p.x < xMin) { p.x = xMin; p.d = 1; }
      m4.makeTranslation(p.x, p.y, p.z);
      ph.setMatrixAt(i, m4);
    }
    ph.instanceMatrix.needsUpdate = true;
    dust.rotation.y += dt * 0.01;
    laser.position.y = Math.sin(t / 1600) * 0.12;
    controls.update();
    renderer.render(scene, camera);
  }
  if (reduce) { resize(); controls.update(); renderer.render(scene, camera); }
  else requestAnimationFrame(frame);
}
