"use strict";

const simulationMode = new URLSearchParams(location.search).get("modo") !== "mqtt";
const CONFIG = {
  topic: "petcarehub/fiap/comedouro01/telemetria",
  brokerUrl: "wss://broker.hivemq.com:8884/mqtt",
  offlineTimeoutMs: 6000,
  maxChartPoints: 40
};
const ids = ["connectionBadge", "freshness", "lastUpdate", "statusPill", "statusDescription",
  "levelValue", "levelProgress", "temperatureValue", "temperatureHint", "remainingValue",
  "remainingProgress", "consumedValue", "weightValue", "demoHint", "demoMode", "cameraStatus",
  "wifiStatus", "mqttStatus", "alertStatus", "jsonOutput", "eventLog", "feedingChart"];
const elements = Object.fromEntries(ids.map(id => [id, document.getElementById(id)]));
const state = { lastMessageAt: null, history: [], status: null };
const context = elements.feedingChart.getContext("2d");
const time = () => new Date().toLocaleTimeString("pt-BR");
const number = value => typeof value === "number" && Number.isFinite(value) ? value : null;
const percent = value => number(value) !== null && value >= 0 && value <= 100 ? value : null;
const format = (value, suffix = "", digits = 1) => value === null ? "---" : `${value.toLocaleString("pt-BR", { maximumFractionDigits: digits })}${suffix}`;

function log(message) {
  const item = document.createElement("div");
  item.className = "log-item";
  item.textContent = `[${time()}] ${message}`;
  elements.eventLog.prepend(item);
  while (elements.eventLog.children.length > 10) elements.eventLog.lastChild.remove();
}

function connection(label, className) {
  elements.connectionBadge.textContent = label;
  elements.connectionBadge.className = `badge ${className}`;
}

function progress(element, value, low) {
  element.style.width = `${value ?? 0}%`;
  element.className = `progress-fill ${value === null ? "" : low ? "danger" : "good"}`;
}

function drawChart() {
  const { width, height } = elements.feedingChart;
  const left = 52, top = 20, bottom = height - 32, right = width - 16;
  context.clearRect(0, 0, width, height);
  context.font = "14px Arial";
  for (let value = 0; value <= 100; value += 25) {
    const y = bottom - value / 100 * (bottom - top);
    context.fillStyle = "#64748b";
    context.fillText(`${value}%`, 5, y + 5);
    context.strokeStyle = "#e2e8f0";
    context.lineWidth = 1;
    context.beginPath(); context.moveTo(left, y); context.lineTo(right, y); context.stroke();
  }
  if (!state.history.length) {
    context.fillText("Aguardando dados do comedouro...", left + 16, 48);
    return;
  }
  for (const [key, color] of [["level", "#2563eb"], ["remaining", "#16a34a"]]) {
    context.strokeStyle = color;
    context.fillStyle = color;
    context.lineWidth = 3;
    context.beginPath();
    let connected = false;
    state.history.forEach((point, index) => {
      if (point[key] === null) { connected = false; return; }
      const x = left + index / (CONFIG.maxChartPoints - 1) * (right - left);
      const y = bottom - point[key] / 100 * (bottom - top);
      if (connected) context.lineTo(x, y); else context.moveTo(x, y);
      context.fillRect(x - 2, y - 2, 4, 4);
      connected = true;
    });
    context.stroke();
  }
  context.fillStyle = "#64748b";
  context.fillText(state.history[0].time, left, height - 8);
  if (state.history.length > 1) context.fillText(state.history[state.history.length - 1].time, right - 68, height - 8);
}

function updateDashboard(data) {
  if (!data || typeof data !== "object" || Array.isArray(data) ||
      (data.modulo !== undefined && data.modulo !== "comedouro") ||
      !["nivelRacao", "percentualConsumido", "temperaturaAmbiente", "porcoesDispensadas"].some(key => number(data[key]) !== null)) {
    throw new Error("Telemetria do comedouro inválida");
  }
  const level = percent(data.nivelRacao);
  const consumed = percent(data.percentualConsumido);
  const remaining = consumed === null ? null : 100 - consumed;
  const temperature = number(data.temperaturaAmbiente);
  const weight = number(data.pesoConsumidoG);
  state.lastMessageAt = Date.now();
  elements.lastUpdate.textContent = time();
  elements.freshness.textContent = "Recebendo dados do comedouro em tempo real.";
  elements.levelValue.textContent = format(level);
  elements.remainingValue.textContent = format(remaining);
  elements.consumedValue.textContent = format(consumed, "%");
  elements.weightValue.textContent = format(weight !== null && weight >= 0 ? weight : null, " g");
  elements.temperatureValue.textContent = format(temperature);
  elements.temperatureHint.textContent = temperature === null ? "Leitura indisponível." : temperature > 32 ? "Temperatura ambiente acima de 32 °C." : "Temperatura do ambiente, não do pet. Limite: 32 °C.";
  progress(elements.levelProgress, level, level < 20);
  progress(elements.remainingProgress, remaining, remaining <= 15);
  const statuses = {
    falha_motor: ["Falha no motor", "Movimento interrompido. Verifique o microinterruptor e rearme a falha no dispositivo.", "status-alerta"],
    falha_sensor: ["Sensor indisponível", "Não foi possível ler a temperatura. Isso não aciona o motor.", "status-alerta"],
    normal: ["Normal", "Leituras dentro dos limites configurados.", "status-moderado"],
    racao_baixa: ["Ração baixa", "Verifique o reservatório e a ração restante no prato.", "status-alerta"],
    temperatura_elevada: ["Temperatura alta", "A temperatura ambiente ultrapassou o limite de 32 °C.", "status-alerta"]
  };
  const [label, description, className] = statuses[data.status] ?? ["Sem status", "Status não identificado na telemetria.", "status-sedentario"];
  elements.statusPill.textContent = label;
  elements.statusPill.className = `status-pill ${data.alerta === true ? "status-alerta" : className}`;
  elements.statusDescription.textContent = description;
  if (state.status !== data.status) { log(`Status do comedouro: ${label}.`); state.status = data.status; }
  for (const [id, value, yes, no] of [
    ["demoMode", data.modoDemo, "Ativado", "Desativado"],
    ["cameraStatus", data.cameraDisponivel, "Disponível", "Indisponível"],
    ["wifiStatus", data.wifi, "Conectado", "Desconectado"],
    ["mqttStatus", data.mqtt, "Conectado", "Desconectado"],
    ["alertStatus", data.alerta, "Alerta ativo", "Sem alerta"]
  ]) elements[id].textContent = value === true ? yes : value === false ? no : "---";
  elements.demoHint.textContent = data.modoDemo === true
    ? "Modo demo: o consumo inclui uma curva simulada; não representa uma refeição real."
    : data.modoDemo === false ? "Consumo estimado pelo dispositivo a partir do brilho do prato."
      : "Modo de leitura não informado.";
  if (data.consumoDisponivel === false) {
    elements.demoHint.textContent = "Consumo não medido. A contagem de porções confirma ciclos do mecanismo, não o peso ou a ingestão de ração.";
  }
  for (const [id, value] of [
    ["humidityValue", format(number(data.umidadeAmbiente), "%")],
    ["portionCount", format(number(data.porcoesDispensadas), "", 0)],
    ["motorState", data.estadoMotor ?? "---"]
  ]) { const target = document.getElementById(id); if (target) target.textContent = value; }
  const levelHint = document.getElementById("levelHint");
  if (levelHint) levelHint.textContent = data.nivelDisponivel === false ? "Não medido: este protótipo não possui sensor no reservatório." : "Alerta quando o reservatório fica abaixo de 20%.";
  elements.jsonOutput.textContent = JSON.stringify(data, null, 2);
  state.history.push({ level, remaining, time: time() });
  if (state.history.length > CONFIG.maxChartPoints) state.history.shift();
  drawChart();
  if (typeof renderFeeder === "function") renderFeeder(level, remaining);
  if (!simulationMode && data.consumoDisponivel === false) {
    document.getElementById("sceneCaption").textContent = `Motor: ${data.estadoMotor ?? "---"} · quantidade no prato não medida`;
    document.getElementById("cameraCaption").textContent = "Quantidade no prato indisponível. O desenho não indica prato vazio e não é vídeo real.";
  }
}

drawChart();
function startMqtt() {
if (typeof mqtt === "undefined") {
  connection("Erro", "disconnected");
  elements.freshness.textContent = "Não foi possível carregar a conexão MQTT. Verifique a internet e recarregue a página.";
  log("Biblioteca MQTT indisponível.");
} else {
  connection("Conectando", "connecting");
  const client = mqtt.connect(CONFIG.brokerUrl, {
    clientId: `dashboard_comedouro_${Math.random().toString(16).slice(2)}`,
    clean: true, connectTimeout: 5000, reconnectPeriod: 2000
  });
  client.on("connect", () => {
    client.subscribe(CONFIG.topic, (error, granted) => {
      if (error || !granted?.length || granted.some(item => item.qos === 128)) {
        connection("Erro na assinatura", "disconnected"); log("Não foi possível assinar o tópico."); return;
      }
      connection("Conectado", "connected"); log("Conectado ao tópico do comedouro.");
    });
  });
  client.on("reconnect", () => connection("Reconectando", "connecting"));
  client.on("close", () => {
    connection("Desconectado", "disconnected");
    elements.freshness.textContent = "Conexão interrompida. Aguardando reconexão; os valores exibidos são as últimas leituras.";
  });
  client.on("error", error => { connection("Erro", "disconnected"); log(`Erro MQTT: ${error.message}`); });
  client.on("message", (topic, message) => {
    if (topic !== CONFIG.topic) return;
    try { updateDashboard(JSON.parse(message.toString())); }
    catch { log("Mensagem inválida ignorada; aguardando telemetria válida."); }
  });
}
}
if (!simulationMode) {
  const script = document.createElement("script");
  script.src = "https://unpkg.com/mqtt/dist/mqtt.min.js";
  script.onload = startMqtt;
  script.onerror = startMqtt;
  document.head.append(script);
}
setInterval(() => {
  if (!simulationMode && state.lastMessageAt !== null && Date.now() - state.lastMessageAt > CONFIG.offlineTimeoutMs) {
    elements.freshness.textContent = "Sem dados recentes do comedouro. Os valores abaixo são as últimas leituras recebidas.";
    elements.lastUpdate.textContent = "sem dados recentes";
  }
}, 1000);
