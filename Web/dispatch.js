// ---------- STAGE 6: emergencies, priority dispatch, moving ambulances, trip log ----------
const AMB_SPEED = 130;                               // pixels per second on a clear road
const PRIORITY_NAME = ["CRITICAL", "HIGH", "NORMAL"];
const PRIORITY_COLOR = ["#dc1e1e", "#f58c14", "#e6c828"];
const AMB_COLOR = ["#8a5cf6", "#0d9488", "#d9467f"];
const STATE_TEXT = { free: "Free at ", toPatient: "Driving to patient at ",
                     loading: "Loading patient at ", toHospital: "Taking patient to " };

// linked list: the dispatch history (your list.c)
class TripLog {
  constructor() { this.head = null; this.tail = null; this.count = 0; }
  add(record) {
    const n = { record: record, next: null };
    if (this.tail === null) this.head = n; else this.tail.next = n;
    this.tail = n;
    this.count++;
  }
  toArray() {
    const out = [];
    for (let n = this.head; n !== null; n = n.next) out.push(n.record);
    return out;
  }
}
let tripLog = new TripLog();

// one ambulance parked at every hospital
const ambulances = [];
const ambRects = [];
const ambTexts = [];
nodes.forEach((n, i) => {
  if (!n.hospital) return;
  const a = { id: ambulances.length + 1, home: i, node: i, from: i, to: i, t: 0,
              state: "free", target: -1, emergency: null, wait: 0 };
  ambulances.push(a);
  ambRects.push(el("rect", { width: 34, height: 22, rx: 5, fill: AMB_COLOR[a.id - 1],
    stroke: "white", "stroke-width": 2, "pointer-events": "none" }));
  const t = el("text", { "text-anchor": "middle", fill: "white", "font-weight": "bold",
    "font-size": 13, "pointer-events": "none" });
  t.textContent = "A" + a.id;
  ambTexts.push(t);
});

// emergencies wait in a priority queue (your MinHeap again)
const emergencies = [];
const waiting = new MinHeap();
let nextId = 1;
let clock = 0;

function addEmergency(node, priority) {
  if (nodes[node].hospital) return;
  const em = { id: nextId++, node: node, priority: priority, status: "waiting", created: clock };
  em.marker = el("circle", { r: 11, fill: PRIORITY_COLOR[priority], stroke: "black", "pointer-events": "none" });
  em.mark = el("text", { "text-anchor": "middle", "font-weight": "bold", "pointer-events": "none" });
  em.mark.textContent = "!";
  emergencies.push(em);
  // key = priority first, then arrival number: critical (0) always comes out first
  waiting.push(em, priority * 1000 + em.id);
}

function removeMarker(em) {
  if (em.marker) { em.marker.remove(); em.mark.remove(); em.marker = null; }
}


// most urgent emergency -> nearest free ambulance (your dispatch())
function dispatch() {
  const skipped = [];
  while (ambulances.some(a => a.state === "free") && !waiting.isEmpty()) {
    const top = waiting.pop();
    const em = top.node;                       // the heap item holds our emergency
    const { dist } = dijkstra(em.node);        // one Dijkstra run gives the cost to every ambulance
    let best = null;
    for (const a of ambulances) {
      if (a.state === "free" && dist[a.node] < Infinity &&
          (best === null || dist[a.node] < dist[best.node])) best = a;
    }
    if (best === null) { skipped.push(top); continue; }   // nobody can reach it right now
    best.state = "toPatient";
    best.target = em.node;
    best.emergency = em;
    em.status = "assigned";
  }
  for (const s of skipped) waiting.push(s.node, s.priority);
}

function arrive(a) {
  if (a.state === "toPatient") {
    a.state = "loading";
    a.wait = 1;                                // one second to load the patient
  } else {
    const em = a.emergency;
    em.status = "done";
    removeMarker(em);
    tripLog.add({ id: tripLog.count + 1, from: nodes[em.node].name, to: nodes[a.node].name,
                  seconds: clock - em.created, priority: PRIORITY_NAME[em.priority], ambulance: a.id });
    a.emergency = null;
    a.target = -1;
    a.state = "free";
  }
}

// moves one ambulance for dt seconds (your amb_step)
function step(a, dt) {
  if (a.state === "free") return;

  if (a.state === "loading") {
    a.wait -= dt;
    if (a.wait > 0) return;
    const h = findNearestHospital(a.node);
    if (h === null) { a.wait = 0; return; }    // no way out: try again next frame
    a.target = h.path[h.path.length - 1];
    a.state = "toHospital";
    return;
  }

  if (a.from === a.to) {                       // standing on a junction
    if (a.node === a.target) { arrive(a); return; }
    const r = findPath(a.node, a.target);      // plan with the CURRENT map
    if (r === null || r.path.length < 2) return;
    a.from = a.node;
    a.to = r.path[1];
    a.t = 0;
  }

  const edge = adj[a.from].find(x => x.to === a.to).edge;
  const len = Math.hypot(nodes[a.to].x - nodes[a.from].x, nodes[a.to].y - nodes[a.from].y);
  a.t += dt * AMB_SPEED / TRAFFIC_FACTOR[edge.traffic] / len;   // heavy traffic = slower
  if (a.t >= 1) {
    a.node = a.to; a.from = a.to; a.t = 0;
    if (a.node === a.target) arrive(a);
  }
}

function ambPosition(a) {
  if (a.from === a.to) return [nodes[a.node].x + (a.id - 2) * 34, nodes[a.node].y - 34];
  const p = nodes[a.from], q = nodes[a.to];
  return [p.x + (q.x - p.x) * a.t, p.y + (q.y - p.y) * a.t];
}

function render() {
  ambulances.forEach((a, i) => {
    const [x, y] = ambPosition(a);
    ambRects[i].setAttribute("x", x - 17);
    ambRects[i].setAttribute("y", y - 11);
    ambTexts[i].setAttribute("x", x);
    ambTexts[i].setAttribute("y", y + 5);
  });
  for (const em of emergencies) {
    if (!em.marker) continue;
    const x = nodes[em.node].x + 24, y = nodes[em.node].y - 24;
    em.marker.setAttribute("cx", x);
    em.marker.setAttribute("cy", y);
    em.marker.setAttribute("r", em.status === "waiting" ? 11 + 2.5 * Math.sin(clock * 7) : 11);
    em.mark.setAttribute("x", x);
    em.mark.setAttribute("y", y + 5);
  }
  renderPanels();
}

function renderPanels() {
  let html = "";
  for (const a of ambulances) {
    const where = (a.state === "free" || a.state === "loading") ? nodes[a.node].name : nodes[a.target].name;
    html += "<b style='color:" + AMB_COLOR[a.id - 1] + "'>A" + a.id + "</b> " + STATE_TEXT[a.state] + where + "<br>";
  }
  html += "Waiting: " + waiting.items.length + " | Completed: " + tripLog.count;
  document.getElementById("status").innerHTML = html;

  const recent = tripLog.toArray().slice(-5).reverse();
  document.getElementById("tripLog").innerHTML = recent.length === 0
    ? "No completed trips yet."
    : recent.map(r => "#" + r.id + " " + r.from + " → " + r.to + " (" +
        r.seconds.toFixed(1) + " s, " + r.priority + ", A" + r.ambulance + ")").join("<br>");
}

function resetSim() {
  for (const em of emergencies) removeMarker(em);
  emergencies.length = 0;
  waiting.items = [];
  nextId = 1;
  clock = 0;
  tripLog = new TripLog();
  for (const a of ambulances) {
    a.node = a.home; a.from = a.home; a.to = a.home; a.t = 0;
    a.state = "free"; a.target = -1; a.emergency = null; a.wait = 0;
  }
}

document.getElementById("reportBtn").addEventListener("click", () => {
  if (selected === -1) return;
  addEmergency(selected, Number(document.getElementById("prioSel").value));
});
document.getElementById("randomBtn").addEventListener("click", () => {
  const junctions = [];
  nodes.forEach((n, i) => { if (!n.hospital) junctions.push(i); });
  addEmergency(junctions[Math.floor(Math.random() * junctions.length)], Math.floor(Math.random() * 3));
});

// the animation loop: runs about 60 times a second
let lastTime = 0;
function loop(ts) {
  const dt = Math.min((ts - lastTime) / 1000, 0.1) || 0;
  lastTime = ts;
  clock += dt;
  dispatch();
  ambulances.forEach(a => step(a, dt));
  render();
  requestAnimationFrame(loop);
}
requestAnimationFrame(loop);
