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
