"use strict";
const sim = { reservoir: 160, bowl: 0, consumed: 0, eating: false, pending: 0, temperature: 26 };
const el = id => document.getElementById(id);
const scene = el("feederScene");
const camera = el("virtualCamera").getContext("2d");
el("sourceMode").value = simulationMode ? "sim" : "mqtt";
el("sourceMode").addEventListener("change", event => {
  const url = new URL(location.href);
  url.searchParams.set("modo", event.target.value);
  location.href = url.href;
});

function renderFeeder(level, remaining) {
  el("hopperFood").style.height = `${level ?? 0}%`;
  el("bowlFood").style.transform = `scaleY(${(remaining ?? 0) / 100})`;
  camera.fillStyle = "#182638";
  camera.fillRect(0, 0, 480, 280);
  camera.fillStyle = "#cbd5e1";
  camera.beginPath(); camera.ellipse(240, 146, 150, 105, 0, 0, Math.PI * 2); camera.fill();
  camera.fillStyle = "#edf2f7";
  camera.beginPath(); camera.ellipse(240, 146, 130, 88, 0, 0, Math.PI * 2); camera.fill();
  const count = Math.round((remaining ?? 0) * 1.8);
  for (let i = 0; i < count; i++) {
    const angle = i * 2.39996, radius = Math.sqrt((i + 0.5) / 180);
    const x = 240 + Math.cos(angle) * radius * 119;
    const y = 146 + Math.sin(angle) * radius * 78;
    camera.fillStyle = i % 2 ? "#a66d35" : "#805027";
    camera.beginPath(); camera.ellipse(x, y, 5, 3.5, angle, 0, Math.PI * 2); camera.fill();
  }
  camera.fillStyle = "#ffffff"; camera.font = "14px Arial";
  camera.fillText("VISÃO VIRTUAL · SEM VÍDEO REAL", 18, 26);
  camera.fillText(remaining === null ? "Aguardando leitura" : `Prato: ${Math.round(remaining)}%`, 18, 264);
  el("cameraCaption").textContent = simulationMode
    ? `Prato simulado: ${sim.bowl.toFixed(1)} g de 40 g · consumido nesta sessão: ${sim.consumed.toFixed(1)} g.`
    : "Representação gerada a partir da telemetria. Não é uma imagem capturada pela câmera.";
  if (!simulationMode) {
    el("sceneCaption").textContent = remaining === null ? "Aguardando telemetria" : "Visualização baseada na telemetria";
  }
}

function announce(text) { el("simMessage").textContent = text; log(text); }
function emitSimulation() {
  const level = sim.reservoir / 200 * 100;
  const remaining = sim.bowl / 40 * 100;
  const status = level < 20 || remaining <= 15 ? "racao_baixa" : sim.temperature > 32 ? "temperatura_elevada" : "normal";
  updateDashboard({ modulo: "comedouro", origem: "simulacao_navegador", nivelRacao: level,
    percentualConsumido: 100 - remaining, pesoConsumidoG: sim.consumed,
    temperaturaAmbiente: sim.temperature, status, alerta: status !== "normal", modoDemo: true,
    cameraDisponivel: false, wifi: false, mqtt: false });
  elements.freshness.textContent = "Simulação local ativa · funciona sem ESP32, Wokwi ou internet.";
  elements.demoHint.textContent = "Na simulação, o percentual indica a parte vazia do prato; o peso consumido acumula o que o pet comeu nesta sessão.";
  elements.cameraStatus.textContent = "Virtual";
  elements.wifiStatus.textContent = "Não necessário";
  elements.mqttStatus.textContent = "Não utilizado";
  elements.consumedValue.textContent = `${Math.round(100 - remaining)}% vazio`;
  scene.classList.toggle("dispensing", sim.pending > 0);
  scene.classList.toggle("eating", sim.eating);
  el("dispense").disabled = sim.pending > 0 || sim.reservoir <= 0 || sim.bowl >= 40;
  el("eat").disabled = sim.bowl <= 0 || sim.pending > 0;
  el("eat").textContent = sim.eating ? "Pausar refeição" : "Pet comer";
  el("eat").setAttribute("aria-pressed", String(sim.eating));
  el("sceneCaption").textContent = sim.pending > 0 ? "Dispensando ração…" : sim.eating ? "O pet está comendo…" : sim.bowl >= 40 ? "Prato cheio" : sim.bowl <= 0 ? "Prato vazio · dispense uma porção" : "Refeição disponível";
}

function dispense() {
  if (sim.pending > 0) return;
  const requested = Number(el("portion").value);
  const amount = Math.min(requested, sim.reservoir, 40 - sim.bowl);
  if (amount <= 0) return;
  sim.eating = false;
  sim.pending = amount;
  announce(`Dispensando ${amount.toFixed(1)} g${amount < requested ? " (limitado pelo espaço no prato ou pelo reservatório)" : ""}.`);
  emitSimulation();
}

function tickSimulation() {
  if (sim.pending > 0) {
    const amount = Math.min(2, sim.pending, sim.reservoir, 40 - sim.bowl);
    sim.reservoir -= amount; sim.bowl += amount; sim.pending -= amount;
    if (sim.pending <= 0) announce("Porção servida. O pet já pode comer.");
  } else if (sim.eating) {
    const amount = Math.min(1, sim.bowl);
    sim.bowl -= amount; sim.consumed += amount;
    if (sim.bowl <= 0) { sim.eating = false; announce("O pet terminou a ração do prato."); }
  }
  emitSimulation();
}

if (simulationMode) {
  connection("Simulação local", "connected");
  el("consumedLabel").textContent = "Parte vazia do prato";
  el("weightLabel").textContent = "Consumido nesta sessão";
  document.querySelector(".topic").textContent = "Dados gerados neste navegador";
  el("dispense").addEventListener("click", dispense);
  el("eat").addEventListener("click", () => { sim.eating = !sim.eating; emitSimulation(); });
  el("refill").addEventListener("click", () => { sim.reservoir = 200; announce("Reservatório reabastecido: 200 g."); emitSimulation(); });
  el("resetSim").addEventListener("click", () => {
    Object.assign(sim, { reservoir: 160, bowl: 0, consumed: 0, eating: false, pending: 0, temperature: 26 });
    el("simTemperature").value = 26; el("simTempValue").textContent = "26 °C";
    state.history = []; state.status = null;
    announce("Simulação reiniciada."); emitSimulation();
  });
  el("simTemperature").addEventListener("input", event => {
    sim.temperature = Number(event.target.value);
    el("simTempValue").textContent = `${sim.temperature} °C`; emitSimulation();
  });
  emitSimulation();
  setInterval(tickSimulation, 500);
} else {
  el("simControls").hidden = true;
  el("virtualPet").hidden = true;
  el("sceneCaption").textContent = "Aguardando telemetria";
  el("simMessage").textContent = "Modo MQTT: inicie o projeto no Wokwi. A visualização acompanha as leituras; o firmware atual não recebe comandos para dispensar pelo navegador.";
  renderFeeder(null, null);
}
