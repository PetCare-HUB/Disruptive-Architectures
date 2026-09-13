/*
  PetCare Hub — Comedouro Inteligente (PetCare Vision)
  ESP32-CAM (AI-Thinker) + DHT22 + HC-SR04 + Servo + Wi-Fi + MQTT

  Substitui a leitura de peso por célula de carga (HX711, versão
  anterior) por uma câmera (OV2640, embutida no ESP32-CAM): compara o
  brilho médio do prato capturado agora contra uma foto de referência
  (prato cheio, tirada no boot/reabastecimento) e calcula o percentual
  de ração consumida. O peso em gramas continua existindo como campo
  (peso_consumido_g), mas passa a ser calculado no próprio ESP32 a
  partir desse percentual e de uma constante de calibração do pote —
  ver seção "PetCare Vision" do PETCARE_AI_1.pdf.

  Importante:
  - A temperatura medida aqui é temperatura AMBIENTE, não corporal.
  - No Wokwi, a câmera simulada devolve sempre o mesmo quadro de
    teste (não muda com o "conteúdo do prato"), então o percentual
    calculado a partir da imagem real fica travado perto de 0%. O
    MODO_DEMO sobrepõe uma curva de consumo só pra apresentação —
    em hardware físico (MODO_DEMO = false) o valor vem 100% do
    brilho capturado pela câmera de verdade.
  - Nível do reservatório (HC-SR04) e temperatura (DHT22) continuam
    sendo leitura real de sensor, sem mudança desta versão.

  Bibliotecas necessárias:
  - DHT sensor library for ESPx
  - PubSubClient
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_camera.h"
#include <DHTesp.h>

// =====================================================
// CONFIGURAÇÃO WI-FI
// =====================================================

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// Para ESP32-CAM físico, comente a linha acima e use:
// const char* WIFI_SSID = "NOME_DO_SEU_WIFI";
// const char* WIFI_PASSWORD = "SENHA_DO_SEU_WIFI";

// =====================================================
// CONFIGURAÇÃO MQTT
// =====================================================

const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* TOPICO_TELEMETRIA = "petcarehub/fiap/comedouro01/telemetria";
const char* TOPICO_STATUS = "petcarehub/fiap/comedouro01/status";
const char* TOPICO_ALERTA = "petcarehub/fiap/comedouro01/alerta";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// =====================================================
// PINOS DOS SENSORES (o resto dos GPIOs do ESP32-CAM já
// está ocupado pela câmera — ver mapa logo abaixo)
// =====================================================

#define DHT_PIN 14
#define TRIG_PIN 13
#define ECHO_PIN 12
#define SERVO_PIN 15

// =====================================================
// PINAGEM FIXA DA CÂMERA OV2640 NO MÓDULO AI-THINKER
// (não mexer — é a pinagem real do módulo, não uma escolha nossa)
// =====================================================

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// =====================================================
// CALIBRAÇÃO
// =====================================================

const float ALTURA_RESERVATORIO_CM = 30.0;
const float DISTANCIA_CHEIO_CM = 5.0;

const int NIVEL_MINIMO_RACAO = 20;
const float TEMPERATURA_MAXIMA = 32.0;

// Pote cheio = 40g = 100% (mesmo exemplo do PETCARE_AI_1.pdf).
const float POTE_CHEIO_G = 40.0;
const int PERCENTUAL_MINIMO_RESTANTE = 15;

// Diferença de brilho (0-255) esperada entre prato cheio e vazio.
// Calibrar este valor olhando o brilho real do prato vazio em
// hardware físico — no simulador ele só afeta o modo não-demo.
const float DIFERENCA_BRILHO_MAXIMA = 60.0;

// =====================================================
// MODO DEMO
// =====================================================

// true  = câmera simulada do Wokwi devolve sempre o mesmo quadro,
//         então o percentual real ficaria travado perto de 0% — o
//         modo demo sobrepõe uma curva de consumo pra apresentação.
// false = usa só o percentual calculado a partir do brilho
//         capturado pela câmera (comportamento do hardware físico).
const bool MODO_DEMO = true;

// =====================================================
// CONTROLE DE TEMPO
// =====================================================

const unsigned long INTERVALO_LEITURA = 2000;
const unsigned long INTERVALO_LIBERACAO = 15000;
const unsigned long INTERVALO_RECONEXAO_WIFI = 5000;
const unsigned long INTERVALO_RECONEXAO_MQTT = 5000;

unsigned long ultimaLeitura = 0;
unsigned long ultimaLiberacao = 0;
unsigned long ultimaTentativaWifi = 0;
unsigned long ultimaTentativaMqtt = 0;

// =====================================================
// ESTADO
// =====================================================

DHTesp dhtSensor;

bool cameraDisponivel = false;
float brilhoReferenciaPratoCheio = -1.0;

int nivelRacaoAtual = 0;
float percentualConsumidoAtual = 0.0;
float pesoConsumidoGAtual = 0.0;
float temperaturaAtual = 0.0;
String statusAtual = "normal";
bool alertaAtual = false;

// =====================================================
// CÂMERA
// =====================================================

bool iniciarCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  // Preto e branco, resolução baixa: o suficiente pra comparar o
  // brilho médio do prato, sem gastar memória/CPU decodificando JPEG.
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QQVGA; // 160x120
  config.fb_count = 1;

  esp_err_t erro = esp_camera_init(&config);

  if (erro != ESP_OK) {
    Serial.printf("Falha ao iniciar a camera (0x%x)\n", erro);
    return false;
  }

  return true;
}

// Média de brilho (0-255) dos pixels do quadro atual, ou -1 se a
// captura falhar.
float capturarBrilhoMedio() {
  camera_fb_t *quadro = esp_camera_fb_get();

  if (!quadro) {
    Serial.println("Falha ao capturar quadro da camera.");
    return -1.0;
  }

  unsigned long soma = 0;
  for (size_t i = 0; i < quadro->len; i++) {
    soma += quadro->buf[i];
  }

  float media = (float)soma / (float)quadro->len;

  esp_camera_fb_return(quadro);

  return media;
}

// =====================================================
// DEMO WOKWI
// =====================================================

float gerarPercentualConsumidoDemo() {
  if (!MODO_DEMO) {
    return 0.0;
  }

  unsigned long faseSegundos = (millis() / 1000) % 60;
  return (faseSegundos / 60.0) * 100.0;
}

// Percentual de ração CONSUMIDA: 0 = prato como a referência (cheio),
// 100 = brilho bem diferente da referência. Ver "PetCare Vision" no
// PETCARE_AI_1.pdf para a lógica completa pensada pro hardware real.
float calcularPercentualConsumido() {
  if (!cameraDisponivel) {
    return gerarPercentualConsumidoDemo();
  }

  float brilhoAtual = capturarBrilhoMedio();

  if (brilhoAtual < 0) {
    return gerarPercentualConsumidoDemo();
  }

  if (brilhoReferenciaPratoCheio < 0) {
    // Primeira leitura após ligar/reabastecer vira a referência de
    // "prato cheio".
    brilhoReferenciaPratoCheio = brilhoAtual;
  }

  float diferenca = fabs(brilhoAtual - brilhoReferenciaPratoCheio);

  float percentual = (diferenca / DIFERENCA_BRILHO_MAXIMA) * 100.0;

  if (percentual < 0) percentual = 0;
  if (percentual > 100) percentual = 100;

  float percentualFinal = percentual + gerarPercentualConsumidoDemo();

  if (percentualFinal > 100) percentualFinal = 100;

  return percentualFinal;
}

// =====================================================
// SENSORES (nível do reservatório e temperatura — sem mudança)
// =====================================================

float lerDistanciaCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duracao == 0) {
    return ALTURA_RESERVATORIO_CM;
  }

  return duracao / 58.0;
}

int calcularNivelRacao(float distanciaCm) {
  float nivel = ((ALTURA_RESERVATORIO_CM - distanciaCm) /
                (ALTURA_RESERVATORIO_CM - DISTANCIA_CHEIO_CM)) * 100.0;

  if (nivel < 0) nivel = 0;
  if (nivel > 100) nivel = 100;

  return (int)nivel;
}

// =====================================================
// SERVO (pulso manual, sem depender da biblioteca ESP32Servo)
// =====================================================

void escreverServo(int angulo) {
  int pulsoMicros = map(angulo, 0, 180, 500, 2400);
  unsigned long inicio = millis();

  while (millis() - inicio < 500) {
    digitalWrite(SERVO_PIN, HIGH);
    delayMicroseconds(pulsoMicros);
    digitalWrite(SERVO_PIN, LOW);
    delayMicroseconds(20000 - pulsoMicros);
  }
}

void liberarRacao() {
  escreverServo(90);
  escreverServo(0);
}

// =====================================================
// STATUS
// =====================================================

String definirStatus(int nivelRacao, float percentualConsumido, float temperaturaAmbiente) {
  if (nivelRacao < NIVEL_MINIMO_RACAO || percentualConsumido >= (100 - PERCENTUAL_MINIMO_RESTANTE)) {
    return "racao_baixa";
  }

  if (temperaturaAmbiente > TEMPERATURA_MAXIMA) {
    return "temperatura_elevada";
  }

  return "normal";
}

// =====================================================
// WI-FI (mesmo padrão da coleira-smart)
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
    Serial.println("O comedouro continua lendo os sensores e tentara reconectar.");
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

    String clientId = "petcarehub-comedouro-";
    clientId += String(random(0xffff), HEX);

    Serial.print("Conectando ao MQTT... ");

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("conectado.");
      mqttClient.publish(TOPICO_STATUS, "comedouro_online", true);
    } else {
      Serial.print("falhou. Codigo: ");
      Serial.println(mqttClient.state());
    }
  }
}

// =====================================================
// JSON
// =====================================================

String boolJson(bool valor) {
  return valor ? "true" : "false";
}

String montarPayload() {
  String payload = "{";

  payload += "\"modulo\":\"comedouro\",";
  payload += "\"nivelRacao\":";
  payload += String(nivelRacaoAtual);
  payload += ",";

  payload += "\"percentualConsumido\":";
  payload += String(percentualConsumidoAtual, 1);
  payload += ",";

  payload += "\"pesoConsumidoG\":";
  payload += String(pesoConsumidoGAtual, 1);
  payload += ",";

  payload += "\"temperaturaAmbiente\":";
  payload += String(temperaturaAtual, 1);
  payload += ",";

  payload += "\"status\":\"";
  payload += statusAtual;
  payload += "\",";

  payload += "\"alerta\":";
  payload += boolJson(alertaAtual);
  payload += ",";

  payload += "\"cameraDisponivel\":";
  payload += boolJson(cameraDisponivel);
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
  String payload = montarPayload();

  Serial.println(payload);

  if (WiFi.status() == WL_CONNECTED && mqttClient.connected()) {
    mqttClient.publish(TOPICO_TELEMETRIA, payload.c_str());
    mqttClient.publish(TOPICO_STATUS, statusAtual.c_str());
    mqttClient.publish(TOPICO_ALERTA, alertaAtual ? "alerta_ativo" : "sem_alerta");
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
  Serial.println("PetCare Hub — Comedouro Inteligente");
  Serial.println("ESP32-CAM + DHT22 + HC-SR04 + MQTT (PetCare Vision)");
  Serial.println("=========================================");

  randomSeed(micros());

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, LOW);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  cameraDisponivel = iniciarCamera();

  if (!cameraDisponivel) {
    Serial.println("Camera nao iniciada — usando percentual de demonstracao.");
  }

  conectarWiFiInicial();
  configurarMQTT();

  Serial.println("Comedouro iniciado.");
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

  unsigned long agora = millis();

  if (agora - ultimaLeitura < INTERVALO_LEITURA) {
    return;
  }
  ultimaLeitura = agora;

  float distancia = lerDistanciaCm();
  nivelRacaoAtual = calcularNivelRacao(distancia);

  percentualConsumidoAtual = calcularPercentualConsumido();
  pesoConsumidoGAtual = (percentualConsumidoAtual / 100.0) * POTE_CHEIO_G;

  TempAndHumidity leituraDht = dhtSensor.getTempAndHumidity();
  temperaturaAtual = isnan(leituraDht.temperature) ? 0.0 : leituraDht.temperature;

  statusAtual = definirStatus(nivelRacaoAtual, percentualConsumidoAtual, temperaturaAtual);
  alertaAtual = statusAtual != "normal";

  if (alertaAtual && (agora - ultimaLiberacao > INTERVALO_LIBERACAO)) {
    liberarRacao();
    ultimaLiberacao = agora;
  }

  publicarDados();
}
