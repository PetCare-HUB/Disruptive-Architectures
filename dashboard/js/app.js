// =====================================================
// CONFIGURAÇÕES
// =====================================================

const CONFIG = {
  topic: "petcarehub/fiap/coleira01/telemetria",
  brokerUrl: "wss://broker.hivemq.com:8884/mqtt",
  offlineTimeoutMs: 6000,
  maxChartPoints: 40,
  maxMovementForProgress: 3.0,
  veryActiveThreshold: 2.1
};

// =====================================================
// ELEMENTOS DA TELA
// =====================================================

const elements = {
  topicText: document.getElementById("topicText"),
  connectionBadge: document.getElementById("connectionBadge"),
  lastUpdate: document.getElementById("lastUpdate"),

  statusPill: document.getElementById("statusPill"),
  statusDescription: document.getElementById("statusDescription"),

  movementValue: document.getElementById("movementValue"),
  movementProgress: document.getElementById("movementProgress"),

  batteryValue: document.getElementById("batteryValue"),
  batteryProgress: document.getElementById("batteryProgress"),
  batteryHint: document.getElementById("batteryHint"),

  demoMode: document.getElementById("demoMode"),
  rawStatus: document.getElementById("rawStatus"),
  windowAverage: document.getElementById("windowAverage"),
  alertStatus: document.getElementById("alertStatus"),
  wifiStatus: document.getElementById("wifiStatus"),
  deviceMqttStatus: document.getElementById("deviceMqttStatus"),

  jsonOutput: document.getElementById("jsonOutput"),
  eventLog: document.getElementById("eventLog"),

  chart: document.getElementById("movementChart")
};

elements.topicText.textContent = CONFIG.topic;

// =====================================================
// ESTADO DO DASHBOARD
// =====================================================

const state = {
  lastMessageAt: null,
  movementHistory: []
};

const chartContext = elements.chart.getContext("2d");

// =====================================================
// FUNÇÕES AUXILIARES
// =====================================================

function formatTime(date = new Date()) {
  return date.toLocaleTimeString("pt-BR", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit"
  });
}

function formatNumber(value, digits = 2) {
  const number = Number(value);

  if (!Number.isFinite(number)) {
    return "---";
  }

  return number.toFixed(digits);
}

function setConnectionStatus(status) {
  const statusMap = {
    connected: {
      label: "Conectado",
      className: "badge connected"
    },
    connecting: {
      label: "Reconectando",
      className: "badge connecting"
    },
    disconnected: {
      label: "Desconectado",
      className: "badge disconnected"
    },
    error: {
      label: "Erro",
      className: "badge disconnected"
    }
  };

  const config = statusMap[status] ?? statusMap.disconnected;

  elements.connectionBadge.className = config.className;
  elements.connectionBadge.innerHTML = `<span class="dot"></span>${config.label}`;
}

function addLog(message) {
  const item = document.createElement("div");
  item.className = "log-item";
  item.textContent = `[${formatTime()}] ${message}`;

  elements.eventLog.prepend(item);

  while (elements.eventLog.children.length > 10) {
    elements.eventLog.removeChild(elements.eventLog.lastChild);
  }
}

function getStatusDescription(status) {
  const descriptions = {
    repouso: "O pet está com baixa movimentação. Situação normal para descanso.",
    ativo: "O pet está em atividade moderada, semelhante a passeio ou movimentação leve.",
    muito_ativo: "O pet está com movimento intenso por tempo contínuo, indicando corrida ou agitação."
  };

  return descriptions[status] ?? "Status ainda não identificado.";
}

function updateStatusCard(status, hasAlert) {
  const normalizedStatus = status || "---";

  elements.statusPill.textContent = normalizedStatus.replace("_", " ");
  elements.statusPill.className = "status-pill";

  if (hasAlert) {
    elements.statusPill.classList.add("status-alerta");
    elements.statusDescription.textContent =
      "Alerta de inatividade detectado. O tutor ou clínica deve acompanhar.";
    return;
  }

  if (normalizedStatus === "repouso") {
    elements.statusPill.classList.add("status-repouso");
  } else if (normalizedStatus === "ativo") {
    elements.statusPill.classList.add("status-ativo");
  } else if (normalizedStatus === "muito_ativo") {
    elements.statusPill.classList.add("status-muito_ativo");
  } else {
    elements.statusPill.classList.add("status-repouso");
  }

  elements.statusDescription.textContent = getStatusDescription(normalizedStatus);
}

function updateProgressBar(progressElement, value, maxValue) {
  const number = Number(value);

  if (!Number.isFinite(number)) {
    progressElement.style.width = "0%";
    return;
  }

  const percent = Math.min(100, Math.max(0, (number / maxValue) * 100));
  progressElement.style.width = `${percent}%`;
}

function updateBattery(battery) {
  const number = Number(battery);

  if (!Number.isFinite(number)) {
    elements.batteryValue.textContent = "---";
    elements.batteryProgress.style.width = "0%";
    elements.batteryHint.textContent = "Aguardando informação da coleira.";
    return;
  }

  elements.batteryValue.textContent = Math.round(number);
  elements.batteryProgress.style.width = `${Math.min(100, Math.max(0, number))}%`;

  elements.batteryProgress.className = "progress-fill";

  if (number >= 60) {
    elements.batteryProgress.classList.add("good");
    elements.batteryHint.textContent = "Bateria em nível seguro para operação.";
  } else if (number >= 25) {
    elements.batteryProgress.classList.add("warn");
    elements.batteryHint.textContent = "Bateria em nível intermediário.";
  } else {
    elements.batteryProgress.classList.add("danger");
    elements.batteryHint.textContent = "Bateria baixa. Recomenda-se recarga.";
  }
}

function getMovementMetric(data) {
  return (
    data.velocidadeMovimentoFiltrada ??
    data.velocidadeMediaJanela ??
    data.movimentoFiltrado ??
    data.movimento ??
    0
  );
}

// =====================================================
// GRÁFICO
// =====================================================

function drawChart() {
  const canvas = elements.chart;
  const ctx = chartContext;
  const width = canvas.width;
  const height = canvas.height;

  ctx.clearRect(0, 0, width, height);

  ctx.fillStyle = "#f8fafc";
  ctx.fillRect(0, 0, width, height);

  ctx.strokeStyle = "#e2e8f0";
  ctx.lineWidth = 1;

  for (let i = 1; i < 5; i++) {
    const y = (height / 5) * i;

    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(width, y);
    ctx.stroke();
  }

  const data = state.movementHistory;

  if (data.length < 2) {
    ctx.fillStyle = "#64748b";
    ctx.font = "16px Arial";
    ctx.fillText("Aguardando dados para montar o gráfico...", 24, 38);
    return;
  }

  const maxValue = Math.max(3, ...data);

  ctx.strokeStyle = "#2563eb";
  ctx.lineWidth = 4;
  ctx.lineJoin = "round";
  ctx.lineCap = "round";

  ctx.beginPath();

  data.forEach((value, index) => {
    const x = (index / (CONFIG.maxChartPoints - 1)) * width;
    const y = height - ((value / maxValue) * (height - 28)) - 14;

    if (index === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  });

  ctx.stroke();

  const thresholdY =
    height - ((CONFIG.veryActiveThreshold / maxValue) * (height - 28)) - 14;

  ctx.strokeStyle = "#ca8a04";
  ctx.lineWidth = 2;
  ctx.setLineDash([8, 8]);

  ctx.beginPath();
  ctx.moveTo(0, thresholdY);
  ctx.lineTo(width, thresholdY);
  ctx.stroke();

  ctx.setLineDash([]);

  ctx.fillStyle = "#ca8a04";
  ctx.font = "13px Arial";
  ctx.fillText("limite muito ativo", 14, thresholdY - 8);
}

// =====================================================
// ATUALIZAÇÃO DA TELA
// =====================================================

function updateDashboard(data) {
  const now = new Date();

  state.lastMessageAt = Date.now();

  const movement = Number(getMovementMetric(data));
  const windowAverage = Number(
    data.velocidadeMediaJanela ?? data.movimentoFiltrado ?? movement
  );

  const status = data.status ?? "---";
  const hasAlert = Boolean(data.alertaInatividade);

  elements.lastUpdate.textContent = formatTime(now);

  updateStatusCard(status, hasAlert);

  elements.movementValue.textContent = formatNumber(movement, 2);
  updateProgressBar(
    elements.movementProgress,
    movement,
    CONFIG.maxMovementForProgress
  );

  updateBattery(data.bateria);

  elements.demoMode.textContent = data.modoDemo === true ? "Ativado" : "Desativado";
  elements.rawStatus.textContent = data.statusBruto ?? "---";

  elements.windowAverage.textContent = Number.isFinite(windowAverage)
    ? `${formatNumber(windowAverage, 2)} g/s`
    : "---";

  elements.alertStatus.textContent = hasAlert
    ? "Inatividade detectada"
    : "Sem alerta";

  elements.alertStatus.style.color = hasAlert ? "var(--red)" : "var(--green)";

  elements.wifiStatus.textContent =
    data.wifi === true ? "Conectado" : "Desconectado";

  elements.wifiStatus.style.color =
    data.wifi === true ? "var(--green)" : "var(--red)";

  elements.deviceMqttStatus.textContent =
    data.mqtt === true ? "Conectado" : "Desconectado";

  elements.deviceMqttStatus.style.color =
    data.mqtt === true ? "var(--green)" : "var(--red)";

  elements.jsonOutput.textContent = JSON.stringify(data, null, 2);

  if (Number.isFinite(movement)) {
    state.movementHistory.push(movement);

    if (state.movementHistory.length > CONFIG.maxChartPoints) {
      state.movementHistory.shift();
    }

    drawChart();
  }
}

function verifyDeviceOffline() {
  if (!state.lastMessageAt) {
    return;
  }

  const elapsed = Date.now() - state.lastMessageAt;

  if (elapsed > CONFIG.offlineTimeoutMs) {
    elements.lastUpdate.textContent = "sem dados recentes";
  }
}

// =====================================================
// MQTT
// =====================================================

const client = mqtt.connect(CONFIG.brokerUrl, {
  clientId: `dashboard_coleira_${Math.random().toString(16).slice(2)}`,
  clean: true,
  connectTimeout: 5000,
  reconnectPeriod: 2000
});

setConnectionStatus("connecting");
addLog("Iniciando conexão com broker MQTT...");
drawChart();

client.on("connect", () => {
  setConnectionStatus("connected");
  addLog("Conectado ao broker MQTT.");

  client.subscribe(CONFIG.topic, (error) => {
    if (error) {
      addLog("Erro ao assinar tópico MQTT.");
      setConnectionStatus("error");
      return;
    }

    addLog(`Assinando tópico: ${CONFIG.topic}`);
  });
});

client.on("reconnect", () => {
  setConnectionStatus("connecting");
  addLog("Tentando reconectar ao broker MQTT...");
});

client.on("close", () => {
  setConnectionStatus("disconnected");
  addLog("Conexão MQTT fechada.");
});

client.on("error", (error) => {
  setConnectionStatus("error");
  addLog(`Erro MQTT: ${error.message}`);
});

client.on("message", (topic, message) => {
  const text = message.toString();

  try {
    const data = JSON.parse(text);
    updateDashboard(data);
  } catch (error) {
    elements.jsonOutput.textContent =
      `Mensagem recebida, mas não é JSON válido:\n\n${text}`;

    addLog("Mensagem MQTT inválida recebida.");
  }
});

setInterval(verifyDeviceOffline, 1000);