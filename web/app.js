const stateNames = [
  "S0 High battery, good link",
  "S1 High battery, poor link",
  "S2 Low battery, good link",
  "S3 Low battery, poor link",
];

const links = [
  { name: "Node 2 relay", quality: 78, energy: 2.2, latency: 16, dropPenalty: 0.08 },
  { name: "Node 3 relay", quality: 63, energy: 2.8, latency: 20, dropPenalty: 0.16 },
  { name: "Gateway direct", quality: 47, energy: 4.6, latency: 12, dropPenalty: 0.28 },
];

const state = {
  episode: 0,
  battery: 100,
  delivered: 0,
  totalLatency: 0,
  totalEnergy: 0,
  currentState: 0,
  currentAction: 0,
  epsilon: 0.1,
  q: Array.from({ length: 4 }, () => Array.from({ length: 3 }, () => 0)),
  pulses: [],
  rngSeed: 7,
};

const elements = {
  canvas: document.querySelector("#networkCanvas"),
  episodesInput: document.querySelector("#episodesInput"),
  epsilonInput: document.querySelector("#epsilonInput"),
  epsilonValue: document.querySelector("#epsilonValue"),
  runButton: document.querySelector("#runButton"),
  stepButton: document.querySelector("#stepButton"),
  resetButton: document.querySelector("#resetButton"),
  qTableBody: document.querySelector("#qTableBody"),
  episodeValue: document.querySelector("#episodeValue"),
  stateValue: document.querySelector("#stateValue"),
  routeValue: document.querySelector("#routeValue"),
  pdrMetric: document.querySelector("#pdrMetric"),
  energyMetric: document.querySelector("#energyMetric"),
  latencyMetric: document.querySelector("#latencyMetric"),
  batteryMetric: document.querySelector("#batteryMetric"),
};

const context = elements.canvas.getContext("2d");

function random() {
  state.rngSeed = (state.rngSeed * 1664525 + 1013904223) >>> 0;
  return state.rngSeed / 4294967296;
}

function encodeState(battery, linkQuality, queueLoad) {
  const lowBattery = battery < 35;
  const poorLink = linkQuality < 55 || queueLoad > 75;
  if (lowBattery && poorLink) return 3;
  if (lowBattery) return 2;
  if (poorLink) return 1;
  return 0;
}

function bestAction(encodedState) {
  const row = state.q[encodedState];
  return row.reduce((best, value, index) => (value > row[best] ? index : best), 0);
}

function selectAction(encodedState) {
  if (random() < state.epsilon) {
    return Math.floor(random() * links.length);
  }
  return bestAction(encodedState);
}

function calculateReward(delivered, energy, latency) {
  return (delivered ? 10 : -10) - energy - latency / 8;
}

function updateQ(encodedState, action, reward, nextState) {
  const current = state.q[encodedState][action];
  const maxNext = Math.max(...state.q[nextState]);
  const target = reward + 0.8 * maxNext;
  state.q[encodedState][action] = current + 0.2 * (target - current);
}

function stepSimulation() {
  const queueLoad = (state.episode * 13) % 100;
  const observedQuality = Math.max(5, links[0].quality - queueLoad * 0.08 + (random() * 14 - 7));
  const encodedState = encodeState(state.battery, observedQuality, queueLoad);
  const action = selectAction(encodedState);
  const link = links[action];
  const linkQuality = Math.max(5, Math.min(96, link.quality - queueLoad * 0.06 + (random() * 18 - 9)));
  const successThreshold = Math.max(0.05, Math.min(0.98, linkQuality / 100 - link.dropPenalty));
  const delivered = random() < successThreshold;
  const latency = link.latency + queueLoad * 0.05 + random() * 3.5;
  const energy = link.energy + (delivered ? 0 : 0.8) + queueLoad * 0.01;
  const reward = calculateReward(delivered, energy, latency);
  const nextState = encodeState(Math.max(0, state.battery - energy * 0.045), linkQuality, queueLoad);

  updateQ(encodedState, action, reward, nextState);

  state.episode += 1;
  state.battery = Math.max(0, state.battery - energy * 0.045);
  state.delivered += delivered ? 1 : 0;
  state.totalLatency += latency;
  state.totalEnergy += energy;
  state.currentState = encodedState;
  state.currentAction = action;
  state.pulses.push({ action, progress: 0, delivered });

  render();
}

function resetSimulation() {
  state.episode = 0;
  state.battery = 100;
  state.delivered = 0;
  state.totalLatency = 0;
  state.totalEnergy = 0;
  state.currentState = 0;
  state.currentAction = 0;
  state.rngSeed = 7;
  state.q = Array.from({ length: 4 }, () => Array.from({ length: 3 }, () => 0));
  state.pulses = [];
  render();
}

function resizeCanvas() {
  const rect = elements.canvas.getBoundingClientRect();
  const ratio = Math.min(window.devicePixelRatio || 1, 2);
  elements.canvas.width = Math.max(1, Math.floor(rect.width * ratio));
  elements.canvas.height = Math.max(1, Math.floor(rect.height * ratio));
  context.setTransform(ratio, 0, 0, ratio, 0, 0);
  drawNetwork();
}

function drawNode(node, active) {
  context.beginPath();
  context.arc(node.x, node.y, node.radius, 0, Math.PI * 2);
  context.fillStyle = active ? "#6be4b7" : "#101617";
  context.fill();
  context.lineWidth = 2;
  context.strokeStyle = active ? "#f4f7f4" : "rgba(244, 247, 244, 0.42)";
  context.stroke();
  context.fillStyle = active ? "#04110e" : "#f4f7f4";
  context.font = "800 13px Inter, system-ui, sans-serif";
  context.textAlign = "center";
  context.fillText(node.label, node.x, node.y + 4);
}

function drawNetwork() {
  const width = elements.canvas.clientWidth;
  const height = elements.canvas.clientHeight;
  const nodes = [
    { label: "N1", x: width * 0.16, y: height * 0.5, radius: 34 },
    { label: "N2", x: width * 0.47, y: height * 0.26, radius: 34 },
    { label: "N3", x: width * 0.47, y: height * 0.68, radius: 34 },
    { label: "GW", x: width * 0.82, y: height * 0.48, radius: 38 },
  ];
  const routes = [
    [nodes[0], nodes[1], nodes[3]],
    [nodes[0], nodes[2], nodes[3]],
    [nodes[0], nodes[3]],
  ];

  context.clearRect(0, 0, width, height);
  context.fillStyle = "rgba(244, 247, 244, 0.04)";
  for (let x = 36; x < width; x += 56) {
    for (let y = 36; y < height; y += 56) {
      context.fillRect(x, y, 1.5, 1.5);
    }
  }

  routes.forEach((route, index) => {
    context.beginPath();
    context.moveTo(route[0].x, route[0].y);
    route.slice(1).forEach((node) => context.lineTo(node.x, node.y));
    context.lineWidth = index === state.currentAction ? 5 : 2;
    context.strokeStyle = index === state.currentAction ? "#6be4b7" : "rgba(244, 247, 244, 0.18)";
    context.stroke();
  });

  state.pulses = state.pulses
    .map((pulse) => ({ ...pulse, progress: pulse.progress + 0.018 }))
    .filter((pulse) => pulse.progress <= 1.05);

  state.pulses.forEach((pulse) => {
    const route = routes[pulse.action];
    const from = route[0];
    const to = route[route.length - 1];
    const x = from.x + (to.x - from.x) * pulse.progress;
    const y = from.y + (to.y - from.y) * pulse.progress;
    context.beginPath();
    context.arc(x, y, 7, 0, Math.PI * 2);
    context.fillStyle = pulse.delivered ? "#ffd166" : "#ff6b6b";
    context.fill();
  });

  nodes.forEach((node, index) => drawNode(node, index === 0 || index === 3 || index === state.currentAction + 1));
}

function renderTable() {
  elements.qTableBody.innerHTML = state.q
    .map((row, index) => {
      const best = bestAction(index);
      const cells = row
        .map((value, action) => `<td class="${action === best ? "best" : ""}">${value.toFixed(2)}</td>`)
        .join("");
      return `<tr><td>${stateNames[index].slice(0, 2)}</td>${cells}</tr>`;
    })
    .join("");
}

function render() {
  const episodes = Math.max(1, state.episode);
  elements.episodeValue.textContent = `Episode ${state.episode}`;
  elements.stateValue.textContent = stateNames[state.currentState];
  elements.routeValue.textContent = links[state.currentAction].name;
  elements.pdrMetric.textContent = (state.delivered / episodes).toFixed(3);
  elements.energyMetric.textContent = (state.totalEnergy / episodes).toFixed(2);
  elements.latencyMetric.textContent = `${(state.totalLatency / episodes).toFixed(2)} ms`;
  elements.batteryMetric.textContent = `${state.battery.toFixed(1)}%`;
  renderTable();
  drawNetwork();
}

elements.runButton.addEventListener("click", () => {
  const count = Math.max(25, Math.min(1000, Number(elements.episodesInput.value) || 240));
  for (let index = 0; index < count; index += 1) {
    stepSimulation();
  }
});

elements.stepButton.addEventListener("click", stepSimulation);
elements.resetButton.addEventListener("click", resetSimulation);
elements.epsilonInput.addEventListener("input", () => {
  state.epsilon = Number(elements.epsilonInput.value) / 100;
  elements.epsilonValue.textContent = `${elements.epsilonInput.value}%`;
});

window.addEventListener("resize", resizeCanvas);

resizeCanvas();
render();
setInterval(drawNetwork, 1000 / 60);

