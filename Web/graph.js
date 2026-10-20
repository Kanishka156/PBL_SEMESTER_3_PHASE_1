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
