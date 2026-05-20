/*
  PetCare Hub — Coleira Smart
  ESP32 + MPU6050 + Wi-Fi + MQTT

  Objetivo:
  - Detectar atividade do pet usando MPU6050
  - Classificar:
      repouso
      ativo
      muito_ativo

*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <math.h>

// =====================================================
// CONFIGURAÇÃO WI-FI
// =====================================================

// Para Wokwi:
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// Para ESP32 físico, comente as linhas acima e use estas:
// const char* WIFI_SSID = "NOME_DO_SEU_WIFI";
// const char* WIFI_PASSWORD = "SENHA_DO_SEU_WIFI";

// =====================================================
// CONFIGURAÇÃO MQTT
// =====================================================

const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* TOPICO_TELEMETRIA = "petcarehub/fiap/coleira01/telemetria";
const char* TOPICO_STATUS = "petcarehub/fiap/coleira01/status";
const char* TOPICO_ALERTA = "petcarehub/fiap/coleira01/alerta";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// =====================================================
// CONFIGURAÇÃO DO MPU6050
// =====================================================

byte MPU6050_ADDR = 0x68;

const int PINO_SDA = 21;
const int PINO_SCL = 22;

// =====================================================
// MODO DEMO
// =====================================================

// true  = simulação Wokwi com movimento artificial
// false = físico real usando só o MPU6050
const bool MODO_DEMO = true;

// =====================================================
// LIMITES DE MOVIMENTO
// =====================================================

const float ZONA_MORTA = 0.18;

const float LIMITE_REPOUSO = 0.35;
const float LIMITE_ATIVO = 0.90;
const float LIMITE_MUITO_ATIVO = 2.10;
const float FATOR_SUAVIZACAO = 0.20;

// =====================================================
// JANELA DE MÉDIA
// =====================================================

const unsigned long JANELA_MEDIA_MS = 5000;

float somaVelocidadeJanela = 0.0;
int quantidadeLeiturasJanela = 0;
unsigned long inicioJanelaMedia = 0;

float velocidadeMediaJanela = 0.0;

// =====================================================
// CONFIRMAÇÃO DE STATUS
// =====================================================

const unsigned long TEMPO_CONFIRMAR_ATIVO = 2000;
const unsigned long TEMPO_CONFIRMAR_REPOUSO = 3000;
const unsigned long TEMPO_CORRIDA_CONTINUA = 8000;

String statusAtual = "repouso";
String statusCandidato = "repouso";

unsigned long inicioStatusCandidato = 0;
unsigned long inicioMuitoAtivo = 0;

// =====================================================
// INATIVIDADE
// =====================================================

const unsigned long TEMPO_INATIVIDADE = 20000;

unsigned long ultimoMovimentoConfirmado = 0;

// =====================================================
// CONTROLE DE TEMPO
// =====================================================

const unsigned long INTERVALO_LEITURA = 200;
const unsigned long INTERVALO_ENVIO_MQTT = 1000;
const unsigned long INTERVALO_RECONEXAO_WIFI = 5000;
const unsigned long INTERVALO_RECONEXAO_MQTT = 5000;

unsigned long ultimaLeitura = 0;
unsigned long ultimoEnvioMQTT = 0;
unsigned long ultimaTentativaWifi = 0;
unsigned long ultimaTentativaMqtt = 0;

// =====================================================
// VARIÁVEIS DE SENSOR
// =====================================================

float ultimoAx = 0.0;
float ultimoAy = 0.0;
float ultimoAz = 0.0;

bool primeiraLeitura = true;

unsigned long tempoUltimaLeituraSensor = 0;

float axAtual = 0.0;
float ayAtual = 0.0;
float azAtual = 0.0;

float variacaoAceleracao = 0.0;
float velocidadeMovimento = 0.0;
float velocidadeMovimentoFiltrada = 0.0;

String statusBrutoAtual = "repouso";

bool alertaInatividadeAtual = false;

// Bateria simulada para dashboard
float bateriaSimulada = 100.0;

// =====================================================
// FUNÇÕES MPU6050
// =====================================================

bool existeDispositivoI2C(byte endereco) {
  Wire.beginTransmission(endereco);
  return Wire.endTransmission() == 0;
}

bool detectarMPU6050() {
  if (existeDispositivoI2C(0x68)) {
    MPU6050_ADDR = 0x68;
    return true;
  }

  if (existeDispositivoI2C(0x69)) {
    MPU6050_ADDR = 0x69;
    return true;
  }

  return false;
}

void escreverRegistroMPU(byte registro, byte valor) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(registro);
  Wire.write(valor);
  Wire.endTransmission(true);
}

int16_t lerInt16MPU() {
  int16_t valor = Wire.read() << 8 | Wire.read();
  return valor;
}

bool iniciarMPU6050() {
  if (!detectarMPU6050()) {
    return false;
  }

  Serial.print("MPU6050 encontrado no endereco 0x");
  Serial.println(MPU6050_ADDR, HEX);

  // Tira o MPU6050 do modo sleep
  escreverRegistroMPU(0x6B, 0x00);
  delay(100);

  // Configura acelerometro em +/- 2g
  escreverRegistroMPU(0x1C, 0x00);
  delay(100);

  return true;
}

bool lerAceleracao(float &ax, float &ay, float &az) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x3B);

  byte erro = Wire.endTransmission(false);

  if (erro != 0) {
    return false;
  }

  byte recebidos = Wire.requestFrom(MPU6050_ADDR, (byte)6, (byte)true);

  if (recebidos < 6) {
    return false;
  }

  int16_t rawX = lerInt16MPU();
  int16_t rawY = lerInt16MPU();
  int16_t rawZ = lerInt16MPU();

  ax = rawX / 16384.0;
  ay = rawY / 16384.0;
  az = rawZ / 16384.0;

  return true;
}

// =====================================================
// DEMO WOKWI
// =====================================================

float gerarVelocidadeDemo() {
  if (!MODO_DEMO) {
    return 0.0;
  }

  unsigned long fase = (millis() / 1000) % 70;

  if (fase < 20) {
    return 0.0;
  }

  if (fase < 40) {
    return 1.10; 
  }

  if (fase < 62) {
    return 2.60; 
  }

  return 0.0;
}

// =====================================================
// CÁLCULO DE MÉDIA
// =====================================================

void atualizarMediaJanela(float velocidade) {
  unsigned long agora = millis();

  if (inicioJanelaMedia == 0) {
    inicioJanelaMedia = agora;
  }

  somaVelocidadeJanela += velocidade;
  quantidadeLeiturasJanela++;

  if (agora - inicioJanelaMedia >= JANELA_MEDIA_MS) {
    if (quantidadeLeiturasJanela > 0) {
      velocidadeMediaJanela = somaVelocidadeJanela / quantidadeLeiturasJanela;
    } else {
      velocidadeMediaJanela = 0.0;
    }

    somaVelocidadeJanela = 0.0;
    quantidadeLeiturasJanela = 0;
    inicioJanelaMedia = agora;
  }
}

// =====================================================
// CLASSIFICAÇÃO
// =====================================================

String classificarStatusBruto(float velocidadeFiltrada, float velocidadeMedia) {

  if (velocidadeFiltrada < LIMITE_REPOUSO && velocidadeMedia < LIMITE_REPOUSO) {
    return "repouso";
  }

  if (velocidadeMedia >= LIMITE_MUITO_ATIVO || velocidadeFiltrada >= (LIMITE_MUITO_ATIVO + 0.50)) {
    return "muito_ativo";
  }

  if (velocidadeFiltrada >= LIMITE_ATIVO || velocidadeMedia >= LIMITE_ATIVO) {
    return "ativo";
  }

  return "repouso";
}

String confirmarStatus(String statusBruto) {
  unsigned long agora = millis();

  if (statusBruto == "muito_ativo") {
    if (inicioMuitoAtivo == 0) {
      inicioMuitoAtivo = agora;
    }

    if (agora - inicioMuitoAtivo >= TEMPO_CORRIDA_CONTINUA) {
      statusAtual = "muito_ativo";
      statusCandidato = "muito_ativo";
      inicioStatusCandidato = agora;
      return statusAtual;
    }

    statusAtual = "ativo";
    return statusAtual;
  }
  inicioMuitoAtivo = 0;

  if (statusBruto != statusCandidato) {
    statusCandidato = statusBruto;
    inicioStatusCandidato = agora;
    return statusAtual;
  }

  unsigned long tempoCandidato = agora - inicioStatusCandidato;

  if (statusBruto == "ativo" && tempoCandidato >= TEMPO_CONFIRMAR_ATIVO) {
    statusAtual = "ativo";
  }

  if (statusBruto == "repouso" && tempoCandidato >= TEMPO_CONFIRMAR_REPOUSO) {
    statusAtual = "repouso";
  }

  return statusAtual;
}

// =====================================================
// WI-FI
// =====================================================

void conectarWiFiInicial() {
  Serial.print("Conectando no Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi conectado.");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi nao conectado.");
    Serial.println("A coleira continua lendo o sensor e tentara reconectar.");
  }
}

void manterWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  unsigned long agora = millis();

  if (agora - ultimaTentativaWifi >= INTERVALO_RECONEXAO_WIFI) {
    ultimaTentativaWifi = agora;

    Serial.println("Wi-Fi desconectado. Tentando reconectar...");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

// =====================================================
// MQTT
// =====================================================

void configurarMQTT() {
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setBufferSize(768);
}

void manterMQTT() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (mqttClient.connected()) {
    mqttClient.loop();
    return;
  }

  unsigned long agora = millis();

  if (agora - ultimaTentativaMqtt >= INTERVALO_RECONEXAO_MQTT) {
    ultimaTentativaMqtt = agora;

    String clientId = "petcarehub-coleira-";
    clientId += String(random(0xffff), HEX);

    Serial.print("Conectando ao MQTT... ");

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("conectado.");
      mqttClient.publish(TOPICO_STATUS, "coleira_online", true);
    } else {
      Serial.print("falhou. Codigo: ");
      Serial.println(mqttClient.state());
    }
  }
}

// =====================================================
// BATERIA SIMULADA
// =====================================================

void atualizarBateriaSimulada() {
  bateriaSimulada -= 0.03;

  if (bateriaSimulada < 15.0) {
    bateriaSimulada = 100.0;
  }
}

// =====================================================
// PROCESSAMENTO DO SENSOR
// =====================================================

void processarLeituraSensor() {
  unsigned long agora = millis();

  if (agora - ultimaLeitura < INTERVALO_LEITURA) {
    return;
  }

  ultimaLeitura = agora;

  float ax, ay, az;

  if (!lerAceleracao(ax, ay, az)) {
    Serial.println("Erro ao ler MPU6050.");
    return;
  }

  axAtual = ax;
  ayAtual = ay;
  azAtual = az;

  if (primeiraLeitura) {
    ultimoAx = ax;
    ultimoAy = ay;
    ultimoAz = az;
    primeiraLeitura = false;
    tempoUltimaLeituraSensor = agora;
    return;
  }

  float deltaTempoSegundos = (agora - tempoUltimaLeituraSensor) / 1000.0;

  if (deltaTempoSegundos <= 0.0) {
    deltaTempoSegundos = 0.2;
  }

  tempoUltimaLeituraSensor = agora;

  float deltaX = ax - ultimoAx;
  float deltaY = ay - ultimoAy;
  float deltaZ = az - ultimoAz;

  ultimoAx = ax;
  ultimoAy = ay;
  ultimoAz = az;

  variacaoAceleracao = sqrt(
    (deltaX * deltaX) +
    (deltaY * deltaY) +
    (deltaZ * deltaZ)
  );

  velocidadeMovimento = variacaoAceleracao / deltaTempoSegundos;

  velocidadeMovimento += gerarVelocidadeDemo();

  if (velocidadeMovimento < ZONA_MORTA) {
    velocidadeMovimento = 0.0;
  }
  velocidadeMovimentoFiltrada =
    (velocidadeMovimentoFiltrada * (1.0 - FATOR_SUAVIZACAO)) +
    (velocidadeMovimento * FATOR_SUAVIZACAO);

  atualizarMediaJanela(velocidadeMovimentoFiltrada);

  statusBrutoAtual = classificarStatusBruto(
    velocidadeMovimentoFiltrada,
    velocidadeMediaJanela
  );

  String statusConfirmado = confirmarStatus(statusBrutoAtual);

  if (statusConfirmado == "ativo" || statusConfirmado == "muito_ativo") {
    ultimoMovimentoConfirmado = agora;
  }

  alertaInatividadeAtual = (agora - ultimoMovimentoConfirmado) > TEMPO_INATIVIDADE;

  atualizarBateriaSimulada();
}

// =====================================================
// JSON
// =====================================================

String boolJson(bool valor) {
  return valor ? "true" : "false";
}

String montarPayload() {
  String payload = "{";

  payload += "\"modulo\":\"coleira_smart\",";
  payload += "\"petId\":\"rex-001\",";
  payload += "\"sensor\":\"MPU6050\",";

  payload += "\"ax\":";
  payload += String(axAtual, 3);
  payload += ",";

  payload += "\"ay\":";
  payload += String(ayAtual, 3);
  payload += ",";

  payload += "\"az\":";
  payload += String(azAtual, 3);
  payload += ",";

  payload += "\"variacaoAceleracao\":";
  payload += String(variacaoAceleracao, 3);
  payload += ",";

  payload += "\"velocidadeMovimento\":";
  payload += String(velocidadeMovimento, 3);
  payload += ",";

  payload += "\"velocidadeMovimentoFiltrada\":";
  payload += String(velocidadeMovimentoFiltrada, 3);
  payload += ",";

  payload += "\"velocidadeMediaJanela\":";
  payload += String(velocidadeMediaJanela, 3);
  payload += ",";

  payload += "\"statusBruto\":\"";
  payload += statusBrutoAtual;
  payload += "\",";

  payload += "\"status\":\"";
  payload += statusAtual;
  payload += "\",";

  payload += "\"bateria\":";
  payload += String(bateriaSimulada, 0);
  payload += ",";

  payload += "\"alertaInatividade\":";
  payload += boolJson(alertaInatividadeAtual);
  payload += ",";

  payload += "\"wifi\":";
  payload += boolJson(WiFi.status() == WL_CONNECTED);
  payload += ",";

  payload += "\"mqtt\":";
  payload += boolJson(mqttClient.connected());
  payload += ",";

  payload += "\"modoDemo\":";
  payload += boolJson(MODO_DEMO);
  payload += ",";

  payload += "\"uptimeMs\":";
  payload += String(millis());

  payload += "}";

  return payload;
}

// =====================================================
// PUBLICAÇÃO MQTT
// =====================================================

void publicarDados() {
  unsigned long agora = millis();

  if (agora - ultimoEnvioMQTT < INTERVALO_ENVIO_MQTT) {
    return;
  }

  ultimoEnvioMQTT = agora;

  String payload = montarPayload();

  Serial.println(payload);

  if (WiFi.status() == WL_CONNECTED && mqttClient.connected()) {
    mqttClient.publish(TOPICO_TELEMETRIA, payload.c_str());
    mqttClient.publish(TOPICO_STATUS, statusAtual.c_str());

    if (alertaInatividadeAtual) {
      mqttClient.publish(TOPICO_ALERTA, "inatividade_detectada");
    } else {
      mqttClient.publish(TOPICO_ALERTA, "sem_alerta");
    }
  } else {
    Serial.println("Sem Wi-Fi/MQTT. Leitura mantida apenas no Serial Monitor.");
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=========================================");
  Serial.println("PetCare Hub — Coleira Smart");
  Serial.println("ESP32 + MPU6050 + MQTT");
  Serial.println("Classificacao por velocidade de movimento");
  Serial.println("=========================================");

  randomSeed(micros());

  Wire.begin(PINO_SDA, PINO_SCL);

  if (!iniciarMPU6050()) {
    Serial.println("Erro: MPU6050 nao encontrado.");
    Serial.println("Verifique as ligacoes:");
    Serial.println("VCC -> 3V3");
    Serial.println("GND -> GND");
    Serial.println("SDA -> GPIO 21");
    Serial.println("SCL -> GPIO 22");

    while (true) {
      delay(1000);
    }
  }

  unsigned long agora = millis();

  ultimoMovimentoConfirmado = agora;
  inicioStatusCandidato = agora;
  inicioJanelaMedia = agora;

  conectarWiFiInicial();
  configurarMQTT();

  Serial.println("Coleira iniciada.");
  Serial.println("Topico MQTT principal:");
  Serial.println(TOPICO_TELEMETRIA);
  Serial.println("=========================================");
}

// =====================================================
// LOOP
// =====================================================

void loop() {
  manterWiFi();
  manterMQTT();

  processarLeituraSensor();
  publicarDados();
}