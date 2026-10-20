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
function findPath(source, target) {
  const { dist, parent } = dijkstra(source);
  if (dist[target] === Infinity) return null;
  return buildPath(parent, source, target, dist[target]);
}
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