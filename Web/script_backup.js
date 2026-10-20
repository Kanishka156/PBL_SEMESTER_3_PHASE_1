const nodes = [
  { name: "Central Chowk", x: 500, y: 300, hospital: false }, //0
  { name: "Clock Tower", x: 350, y: 200, hospital: false }, //1
  { name: "Railway Station", x: 150, y: 150, hospital: false }, //2
  { name: "Bus Stand", x: 150, y: 350, hospital: false }, //3
  { name: "Market", x: 350, y: 400, hospital: false }, //4
  { name: "University", x: 650, y: 150, hospital: false }, //5
  { name: "Park Gate", x: 650, y: 420, hospital: false }, //6
  { name: "Old Bridge", x: 850, y: 300, hospital: false }, //7
  { name: "Ring Road North", x: 500, y: 80, hospital: false }, //8
  { name: "Ring Road South", x: 500, y: 520, hospital: false }, //9
  { name: "City Hospital", x: 250, y: 520, hospital: true }, //10
  { name: "Metro Hospital", x: 850, y: 130, hospital: true }, //11
  { name: "General Hospital", x: 850, y: 500, hospital: true } //12
];
const roads = [
  { a: 0, b: 1, distance: 1.5 },
  { a: 0, b: 4, distance: 1.8 },
  { a: 0, b: 5, distance: 2.2 },
  { a: 0, b: 6, distance: 2.0 },
  { a: 0, b: 7, distance: 3.4 },
  { a: 1, b: 2, distance: 2.6 },
  { a: 1, b: 3, distance: 3.0 },
  { a: 1, b: 8, distance: 2.3 },
  { a: 2, b: 3, distance: 2.2 },
  { a: 3, b: 4, distance: 2.4 },
  { a: 4, b: 9, distance: 2.5 },
  { a: 4, b: 10, distance: 2.8 },
  { a: 5, b: 7, distance: 2.4 },
  { a: 5, b: 8, distance: 2.6 },
  { a: 5, b: 11, distance: 2.9 },
  { a: 6, b: 7, distance: 2.5 },
  { a: 6, b: 9, distance: 2.3 },
  { a: 6, b: 12, distance: 2.7 },
  { a: 7, b: 11, distance: 1.8 },
  { a: 7, b: 12, distance: 2.0 },
  { a: 9, b: 10, distance: 2.6 }
];
console.log(nodes.length, roads.length);
// ---------- 2. HELPER: creates one shape inside the drawing area ----------
const svg = document.getElementById("map");

function el(tag, attrs) {
  const e = document.createElementNS("http://www.w3.org/2000/svg", tag);
  for (const k in attrs) {
    e.setAttribute(k, attrs[k]);
  }
  svg.appendChild(e);
  return e;
}
// ---------- DRAW THE ROADS AND JUNCTIONS ----------
const roadLines = [];      // the visible line of every road
const closeMarks = [];     // the red X of every road
let selectedRoad = -1;

roads.forEach((road, i) => {
  const from = nodes[road.a];
  const to = nodes[road.b];
  const mx = (from.x + to.x) / 2;
  const my = (from.y + to.y) / 2;

  const line = el("line", { x1: from.x, y1: from.y, x2: to.x, y2: to.y,
    stroke: "#2ea043", "stroke-width": 5, "stroke-linecap": "round" });
  roadLines.push(line);

  const label = el("text", { x: mx, y: my - 8, "text-anchor": "middle",
    "font-size": 13, fill: "#555", "pointer-events": "none" });
  label.textContent = road.distance;

  const mark = el("text", { x: mx, y: my + 8, "text-anchor": "middle",
    "font-size": 22, "font-weight": "bold", fill: "#d23232", "pointer-events": "none" });
  closeMarks.push(mark);

  // an invisible, wider line on top so the road is easy to click
  const hit = el("line", { x1: from.x, y1: from.y, x2: to.x, y2: to.y,
    stroke: "transparent", "stroke-width": 18 });
  hit.style.cursor = "pointer";
  hit.addEventListener("click", () => selectRoad(i));
});

const circles = [];
nodes.forEach((n, i) => {
  const c = el("circle", { cx: n.x, cy: n.y, r: 17,
    fill: n.hospital ? "#c8203a" : "#2a62d9", stroke: "white", "stroke-width": 2 });
  c.style.cursor = "pointer";
  c.addEventListener("click", () => selectNode(i));
  circles.push(c);

  if (n.hospital) {
    const h = el("text", { x: n.x, y: n.y + 5, "text-anchor": "middle",
      fill: "white", "font-weight": "bold", "pointer-events": "none" });
    h.textContent = "H";
  }
  const name = el("text", { x: n.x, y: n.y + 36, "text-anchor": "middle",
    "font-size": 13, "pointer-events": "none" });
  name.textContent = n.name;
});
//The Graph (graph.c)
const TRAFFIC_FACTOR = [1, 1.5, 2.5];      // clear, moderate, heavy

const adj = nodes.map(() => []);
const edges = [];                          // every road once, same order as roads[]
for (const road of roads) {
  const edge = { a: road.a, b: road.b, distance: road.distance, traffic: 0, closed: false };
  edges.push(edge);
  adj[road.a].push({ to: road.b, edge: edge });
  adj[road.b].push({ to: road.a, edge: edge });
}

let mode = "traffic";                      // "traffic" = fastest, "distance" = shortest

function cost(edge) {
  if (edge.closed) return Infinity;
  if (mode === "distance") return edge.distance;
  return edge.distance * TRAFFIC_FACTOR[edge.traffic];
}
// ---------- 5. DIJKSTRA'S ALGORITHM-STAGE 4 ----------(heap and dijkstra i.e. heap.c and algorithm.c to find the shortest path)
class MinHeap {
  constructor() { this.items = []; }
  isEmpty() { return this.items.length === 0; }

  push(node, priority) {
    this.items.push({ node: node, priority: priority });
    let i = this.items.length - 1;
    while (i > 0) {
      const parent = Math.floor((i - 1) / 2);
      if (this.items[parent].priority <= this.items[i].priority) break;
      [this.items[parent], this.items[i]] = [this.items[i], this.items[parent]];
      i = parent;
    }
  }

  pop() {
    if (this.isEmpty()) return null;
    const top = this.items[0];
    const last = this.items.pop();
    if (this.items.length > 0) {
      this.items[0] = last;
      let i = 0;
      while (true) {
        const left = 2 * i + 1;
        const right = 2 * i + 2;
        let smallest = i;
        if (left < this.items.length && this.items[left].priority < this.items[smallest].priority) smallest = left;
        if (right < this.items.length && this.items[right].priority < this.items[smallest].priority) smallest = right;
        if (smallest === i) break;
        [this.items[smallest], this.items[i]] = [this.items[i], this.items[smallest]];
        i = smallest;
      }
    }
    return top;
  }
}

function dijkstra(source) {
  const dist = nodes.map(() => Infinity);
  const parent = nodes.map(() => -1);
  const heap = new MinHeap();
  dist[source] = 0;
  heap.push(source, 0);

  while (!heap.isEmpty()) {
    const top = heap.pop();
    const u = top.node;
    const d = top.priority;
    if (d > dist[u]) continue;                 // old entry, skip it
    for (const { to, edge } of adj[u]) {
      const c = cost(edge);
      if (d + c < dist[to]) {
        dist[to] = d + c;
        parent[to] = u;
        heap.push(to, dist[to]);
      }
    }
  }
  return { dist, parent };
}

function buildPath(parent, source, target, totalCost) {
  const stack = [];
  for (let v = target; v !== -1; v = parent[v]) stack.push(v);   // comes out backwards
  const path = [];
  while (stack.length > 0) path.push(stack.pop());               // stack flips it
  if (path[0] !== source) return null;
  return { path: path, cost: totalCost };
}

function findNearestHospital(source) {
  const { dist, parent } = dijkstra(source);
  let best = -1;
  for (let i = 0; i < nodes.length; i++) {
    if (nodes[i].hospital && dist[i] < Infinity && (best === -1 || dist[i] < dist[best])) best = i;
  }
  if (best === -1) return null;
  return buildPath(parent, source, best, dist[best]);
}

console.log(findNearestHospital(2));   // Railway Station: path [2,3,4,10], cost about 7.4
//click, route and message
let selected = -1;
let routeLines = [];

function selectNode(i) {
  selected = i;
  circles.forEach((c, k) => {
    c.setAttribute("stroke", k === i ? "orange" : "white");
    c.setAttribute("stroke-width", k === i ? 5 : 2);
  });
  showRoute();
}

function clearRoute() {
  for (const l of routeLines) l.remove();
  routeLines = [];
}



// ---------- STAGE 5: traffic, closures, mode, reset ----------
const TRAFFIC_COLOR = ["#2ea043", "#f0a01e", "#d23232"];
const TRAFFIC_NAME = ["Clear", "Moderate", "Heavy"];

function refreshRoad(i) {                  // redraw one road to match its data
  const e = edges[i];
  roadLines[i].setAttribute("stroke", e.closed ? "#777" : TRAFFIC_COLOR[e.traffic]);
  roadLines[i].setAttribute("stroke-dasharray", e.closed ? "4 8" : "none");
  closeMarks[i].textContent = e.closed ? "✕" : "";
}

function updateRoadInfo() {
  const p = document.getElementById("roadInfo");
  if (selectedRoad === -1) {
    p.textContent = "Click a road to change its traffic or close it.";
    return;
  }
  const e = edges[selectedRoad];
  p.textContent = "Road: " + nodes[e.a].name + " ↔ " + nodes[e.b].name + " | " + e.distance +
    " km | " + TRAFFIC_NAME[e.traffic] + " traffic | " + (e.closed ? "CLOSED" : "open");
}

function selectRoad(i) {
  selectedRoad = i;
  roadLines.forEach((l, k) => l.setAttribute("stroke-width", k === i ? 10 : 5));
  updateRoadInfo();
}

function setTraffic(level) {
  if (selectedRoad === -1) {
    document.getElementById("roadInfo").textContent = "Select a road first.";
    return;
  }
  edges[selectedRoad].traffic = level;
  refreshRoad(selectedRoad);
  updateRoadInfo();
  showRoute();                             // the route is recalculated here
}

function toggleClosed() {
  if (selectedRoad === -1) {
    document.getElementById("roadInfo").textContent = "Select a road first.";
    return;
  }
  edges[selectedRoad].closed = !edges[selectedRoad].closed;
  refreshRoad(selectedRoad);
  updateRoadInfo();
  showRoute();
}

function toggleMode() {
  mode = (mode === "traffic") ? "distance" : "traffic";
  document.getElementById("modeBtn").textContent =
    (mode === "traffic") ? "Mode: Fastest (uses traffic)" : "Mode: Shortest (distance only)";
  showRoute();
}

function resetAll() {
  edges.forEach((e, i) => { e.traffic = 0; e.closed = false; refreshRoad(i); });
  selectedRoad = -1;
  roadLines.forEach(l => l.setAttribute("stroke-width", 5));
  selected = -1;
  circles.forEach(c => { c.setAttribute("stroke", "white"); c.setAttribute("stroke-width", 2); });
  mode = "traffic";
  document.getElementById("modeBtn").textContent = "Mode: Fastest (uses traffic)";
  updateRoadInfo();
  showRoute();
  resetSim(); 
}

document.getElementById("modeBtn").addEventListener("click", toggleMode);
document.getElementById("closeBtn").addEventListener("click", toggleClosed);
document.getElementById("resetBtn").addEventListener("click", resetAll);
document.querySelectorAll("[data-t]").forEach(b =>
  b.addEventListener("click", () => setTraffic(Number(b.dataset.t))));

updateRoadInfo();

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

function findPath(source, target) {
  const { dist, parent } = dijkstra(source);
  if (dist[target] === Infinity) return null;
  return buildPath(parent, source, target, dist[target]);
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

// ---------- STAGE 7: BFS, DFS, alternate route ----------
class Queue {                                   // your queue.c
  constructor() { this.items = []; this.head = 0; }
  isEmpty() { return this.head >= this.items.length; }
  enqueue(x) { this.items.push(x); }
  dequeue() { return this.items[this.head++]; }
}

// BFS: visits junctions ring by ring. hops[i] = roads from the source (-1 = cannot be reached)
function bfs(source) {
  const hops = nodes.map(() => -1);
  const order = [];
  const q = new Queue();
  hops[source] = 0;
  q.enqueue(source);
  while (!q.isEmpty()) {
    const u = q.dequeue();
    order.push(u);
    for (const { to, edge } of adj[u]) {
      if (!edge.closed && hops[to] === -1) {
        hops[to] = hops[u] + 1;
        q.enqueue(to);
      }
    }
  }
  return { order, hops };
}

// DFS with an explicit stack: goes as deep as it can first
function dfsOrder(source) {
  const visited = nodes.map(() => false);
  const order = [];
  const stack = [source];
  while (stack.length > 0) {
    const u = stack.pop();
    if (visited[u]) continue;
    visited[u] = true;
    order.push(u);
    for (const { to, edge } of adj[u]) {
      if (!edge.closed && !visited[to]) stack.push(to);
    }
  }
  return order;
}

// DFS with backtracking: counts every possible route between two junctions
function countRoutes(u, target, visited = new Set()) {
  if (u === target) return 1;
  visited.add(u);
  let total = 0;
  for (const { to, edge } of adj[u]) {
    if (!edge.closed && !visited.has(to)) total += countRoutes(to, target, visited);
  }
  visited.delete(u);                            // backtrack
  return total;
}

// alternate route: block each road of the main route in turn, keep the cheapest result
function findAlternate(source, target, main) {
  let best = null;
  for (let k = 0; k < main.path.length - 1; k++) {
    const edge = adj[main.path[k]].find(x => x.to === main.path[k + 1]).edge;
    edge.closed = true;
    const r = findPath(source, target);
    edge.closed = false;
    if (r !== null && (best === null || r.cost < best.cost)) best = r;
  }
  return best;
}

function drawRoute(path, color, width, beforeNode, dashed) {
  for (let k = 0; k < path.length - 1; k++) {
    const p = nodes[path[k]];
    const q = nodes[path[k + 1]];
    const line = el("line", { x1: p.x, y1: p.y, x2: q.x, y2: q.y, stroke: color,
      "stroke-width": width, "stroke-linecap": "round", "pointer-events": "none" });
    if (dashed) line.setAttribute("stroke-dasharray", "12 10");
    svg.insertBefore(line, beforeNode);          // put it underneath the roads
    routeLines.push(line);
  }
}

// REPLACES the old showRoute from Stage 4
function showRoute() {
  clearRoute();
  const info = document.getElementById("info");
  if (selected === -1) {
    info.textContent = "Click a junction to choose the emergency location.";
    return;
  }
  const main = findNearestHospital(selected);
  if (main === null) {
    info.textContent = nodes[selected].name + ": NO ROUTE to any hospital";
    return;
  }
  const dest = main.path[main.path.length - 1];
  const alt = findAlternate(selected, dest, main);

  drawRoute(main.path, "gold", 18, svg.firstChild, false);
  if (alt !== null) drawRoute(alt.path, "#8a5cf6", 11, roadLines[0], true);

  const names = path => path.map(id => nodes[id].name).join(" → ");
  let html = "<b>Emergency at " + nodes[selected].name + "</b> → nearest hospital: <b>" + nodes[dest].name + "</b><br>" +
    "Best route (gold): " + names(main.path) + "<br>" +
    "Cost: " + main.cost.toFixed(1) + (mode === "distance" ? " km" : " (traffic-weighted)") +
    " | roads on route: " + (main.path.length - 1) + "<br>" +
    "Fewest roads (BFS): " + bfs(selected).hops[dest] + " | possible routes (DFS): " + countRoutes(selected, dest) + "<br>";
  html += alt === null ? "Alternate route: none"
    : "Alternate route (purple, dashed): " + names(alt.path) + " | cost " + alt.cost.toFixed(1) +
      " (+" + (alt.cost - main.cost).toFixed(1) + ")";
  info.innerHTML = html;
}

// numbered badges show the order in which BFS / DFS visit the junctions
let traversalMarks = [];
let traversalRun = 0;

function clearTraversal() {
  traversalRun++;                                // cancels badges that are still waiting to appear
  for (const m of traversalMarks) m.remove();
  traversalMarks = [];
}
function showTraversal(kind) {
  const out = document.getElementById("travInfo");
  if (selected === -1) { out.textContent = "Select a junction first."; return; }
  clearTraversal();
  const run = traversalRun;
  const order = (kind === "bfs") ? bfs(selected).order : dfsOrder(selected);
  out.textContent = kind.toUpperCase() + " order from " + nodes[selected].name + ": " +
    order.map(id => nodes[id].name).join(", ");
  order.forEach((id, k) => {
    setTimeout(() => {
      if (run !== traversalRun) return;
      const n = nodes[id];
      const badge = el("circle", { cx: n.x - 26, cy: n.y - 24, r: 11, fill: "#ff8a00",
        stroke: "black", "pointer-events": "none" });
      const num = el("text", { x: n.x - 26, y: n.y - 20, "text-anchor": "middle",
        "font-size": 12, "font-weight": "bold", "pointer-events": "none" });
      num.textContent = k + 1;
      traversalMarks.push(badge, num);
    }, k * 350);
  });
}

document.getElementById("bfsBtn").addEventListener("click", () => showTraversal("bfs"));
document.getElementById("dfsBtn").addEventListener("click", () => showTraversal("dfs"));

showRoute();
