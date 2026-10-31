/* PetCare Hub - ESP32-S3: Wokwi e hardware compartilham controle_porcao.h.
   Leia LEIA-ME.md. A simulacao NAO valida a OV5640, potencia ou mecanica.
   Serial: d = uma porcao, p = parar, r = rearmar falha.
*/
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include "config.h"
#include "controle_porcao.h"
#if !PETCARE_SIMULATION
#include "esp_camera.h"
#include "esp_http_server.h"
#endif

WiFiClient rede;
PubSubClient mqtt(rede);
DHTesp dht;
QueueHandle_t comandos;
portMUX_TYPE trava = portMUX_INITIALIZER_UNLOCKED;
ControlePorcao controle(TEMPO_MAXIMO_PORCAO_MS, TEMPO_FREIO_MS);
ControlePorcao retrato;
bool cameraDisponivel = false;
float temperatura = NAN, umidade = NAN;
uint32_t ultimaLeitura = 0, ultimaRede = 0, ultimaPublicacao = 0;
uint32_t ultimoTotal = 0;
int ultimoEstado = -1;
constexpr bool motorPermitido = PETCARE_SIMULATION || MOTOR_FISICO_HABILITADO;

bool chavePressionada() {
  return digitalRead(PIN_FIM_CURSO) == (FIM_CURSO_ATIVO_BAIXO ? LOW : HIGH);
}

// Mesmo conjunto de sinais no Wokwi e no TB6612 real.
// STBY baixo em repouso. Girar: IN1=1,IN2=0,PWM=1. Frear: PWM=0,STBY=1.
void aplicarMotor(ControlePorcao::Saida saida) {
  static int anterior = -1;
  if (!motorPermitido) return;
  if (anterior == int(saida)) return;
  anterior = int(saida);
  if (saida == ControlePorcao::Saida::Girar) {
    digitalWrite(PIN_STBY, LOW);
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, LOW);
    digitalWrite(PIN_PWMA, HIGH);
    digitalWrite(PIN_STBY, HIGH);
  } else if (saida == ControlePorcao::Saida::Frear) {
    digitalWrite(PIN_PWMA, LOW);
    digitalWrite(PIN_STBY, HIGH);
  } else {
    digitalWrite(PIN_STBY, LOW);
    digitalWrite(PIN_PWMA, LOW);
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
  }
}

// Tarefa independente: MQTT, captura e leitura DHT nao atrasam o timeout do motor.
void tarefaMotor(void*) {
  for (;;) {
    const uint32_t agora = millis();
    char comando;
    if (xQueueReceive(comandos, &comando, 0) == pdTRUE) {
      if (comando == 'd' && motorPermitido) controle.iniciar(agora, chavePressionada());
      if (comando == 'p') controle.cancelar(agora);
      if (comando == 'r') controle.rearmar();
    }
    controle.atualizar(agora, chavePressionada());
    aplicarMotor(controle.saida());
    portENTER_CRITICAL(&trava);
    retrato = controle;
    portEXIT_CRITICAL(&trava);
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

ControlePorcao estadoMotor() {
  portENTER_CRITICAL(&trava);
  ControlePorcao copia = retrato;
  portEXIT_CRITICAL(&trava);
  return copia;
}

void solicitar(char comando) {
  if (comando != 'd' && comando != 'p' && comando != 'r') return;
  const ControlePorcao c = estadoMotor();
  if (comando == 'd' && (!motorPermitido || c.estado != ControlePorcao::Estado::Parado)) {
    Serial.println("Comando rejeitado: motor ocupado, bloqueado ou com falha.");
    return;
  }
  // Caixa de correio de um item: nao acumula porcoes para executar depois.
  if (comando == 'p') xQueueOverwrite(comandos, &comando);
  else if (xQueueSend(comandos, &comando, 0) != pdTRUE)
    Serial.println("Comando rejeitado: existe uma solicitacao pendente.");
}

const char* nomeEstado(ControlePorcao::Estado e) {
  switch (e) {
    case ControlePorcao::Estado::Parado: return "parado";
    case ControlePorcao::Estado::SoltandoChave: return "liberando_chave";
    case ControlePorcao::Estado::AguardandoChave: return "dispensando";
    case ControlePorcao::Estado::Freando: return "freando";
    default: return "falha_motor";
  }
}

#if !PETCARE_SIMULATION
esp_err_t servirFoto(httpd_req_t* req) {
  // Nao disputa captura com a movimentacao; motor tem tarefa propria.
  if (estadoMotor().estado != ControlePorcao::Estado::Parado) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    return httpd_resp_sendstr(req, "Aguarde o motor parar.");
  }
  camera_fb_t* quadro = esp_camera_fb_get();
  if (!quadro) return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Falha na captura");
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  esp_err_t resultado = httpd_resp_send(req, (const char*)quadro->buf, quadro->len);
  esp_camera_fb_return(quadro);
  return resultado;
}

void iniciarCamera() {
  if (!CAMERA_PINOUT_CONFIRMADO) {
    Serial.println("Camera bloqueada: confirmar mapa da placa OEM em config.h.");
    return;
  }
  if (!psramFound()) {
    Serial.println("Camera indisponivel: PSRAM nao detectada; configurar OPI PSRAM.");
    return;
  }
  camera_config_t cfg = {};
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer = LEDC_TIMER_0;
  cfg.pin_d0 = CAM_D0; cfg.pin_d1 = CAM_D1; cfg.pin_d2 = CAM_D2; cfg.pin_d3 = CAM_D3;
  cfg.pin_d4 = CAM_D4; cfg.pin_d5 = CAM_D5; cfg.pin_d6 = CAM_D6; cfg.pin_d7 = CAM_D7;
  cfg.pin_xclk = CAM_XCLK; cfg.pin_pclk = CAM_PCLK;
  cfg.pin_vsync = CAM_VSYNC; cfg.pin_href = CAM_HREF;
  cfg.pin_sccb_sda = CAM_SDA; cfg.pin_sccb_scl = CAM_SCL;
  cfg.pin_pwdn = -1; cfg.pin_reset = -1;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;
  cfg.frame_size = FRAMESIZE_QVGA;
  cfg.jpeg_quality = 12;
  cfg.fb_count = 1;
  cfg.fb_location = CAMERA_FB_IN_PSRAM;
  cfg.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  esp_err_t erro = esp_camera_init(&cfg);
  cameraDisponivel = erro == ESP_OK;
  Serial.printf("Inicializacao da camera: 0x%x\n", erro);
}

void manterServidorCamera() {
  static bool iniciado = false;
  if (iniciado || !cameraDisponivel || WiFi.status() != WL_CONNECTED) return;
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t servidor = nullptr;
  if (httpd_start(&servidor, &cfg) != ESP_OK) return;
  httpd_uri_t rota = {};
  rota.uri = "/foto.jpg";
  rota.method = HTTP_GET;
  rota.handler = servirFoto;
  if (httpd_register_uri_handler(servidor, &rota) != ESP_OK) {
    httpd_stop(servidor);
    return;
  }
  iniciado = true;
  Serial.print("Foto real na rede local: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/foto.jpg");
}
#endif

void manterRede() {
  if (WiFi.status() == WL_CONNECTED && mqtt.connected()) {
    mqtt.loop();
    return;
  }
  if (millis() - ultimaRede < 5000 || !WIFI_SSID[0]) return;
  ultimaRede = millis();
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    return;
  }
  String id = "petcare-comedouro-s3-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  mqtt.connect(id.c_str());
}

String numeroJson(float valor) { return isnan(valor) ? "null" : String(valor, 1); }

void publicar() {
  const ControlePorcao c = estadoMotor();
  const bool falhaMotor = c.falha != ControlePorcao::Falha::Nenhuma;
  const char* status = falhaMotor ? "falha_motor" : isnan(temperatura) ? "falha_sensor"
    : temperatura > 32 ? "temperatura_elevada" : "normal";
  String json = "{\"modulo\":\"comedouro\",\"nivelRacao\":null,\"percentualConsumido\":null,\"pesoConsumidoG\":null";
  json += ",\"temperaturaAmbiente\":" + numeroJson(temperatura);
  json += ",\"umidadeAmbiente\":" + numeroJson(umidade);
  json += ",\"porcoesDispensadas\":" + String(c.porcoes);
  json += ",\"estadoMotor\":\"" + String(nomeEstado(c.estado)) + "\"";
  json += ",\"falhaMotor\":\"" + String(c.falha == ControlePorcao::Falha::TempoEsgotado
    ? "tempo_esgotado" : c.falha == ControlePorcao::Falha::Cancelado ? "cancelado" : "nenhuma") + "\"";
  json += ",\"status\":\"" + String(status) + "\",\"alerta\":" + String(strcmp(status, "normal") ? "true" : "false");
  json += ",\"cameraDisponivel\":" + String(cameraDisponivel ? "true" : "false");
  json += ",\"wifi\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
  json += ",\"mqtt\":" + String(mqtt.connected() ? "true" : "false");
  json += ",\"modoDemo\":" + String(PETCARE_SIMULATION ? "true" : "false");
  json += ",\"origem\":\"" + String(PETCARE_SIMULATION ? "wokwi" : "hardware") + "\"";
  json += ",\"consumoDisponivel\":false,\"nivelDisponivel\":false";
  json += ",\"sensorTemperatura\":\"" + String(PETCARE_SIMULATION ? "DHT22_substituto_virtual" : "DHT11") + "\"";
  json += ",\"uptimeMs\":" + String(millis()) + "}";
  Serial.println(json);
  if (mqtt.connected()) mqtt.publish(TOPICO_TELEMETRIA, json.c_str(), false);
}

void setup() {
  Serial.begin(115200);
  // Estado seguro antes de Wi-Fi, sensores ou camera.
  if (motorPermitido) {
    for (int p : {PIN_STBY, PIN_PWMA, PIN_AIN1, PIN_AIN2}) {
      digitalWrite(p, LOW);
      pinMode(p, OUTPUT);
    }
  }
  pinMode(PIN_FIM_CURSO, FIM_CURSO_ATIVO_BAIXO ? INPUT_PULLUP : INPUT_PULLDOWN);
#if PETCARE_SIMULATION
  pinMode(PIN_TESTE_DISPENSAR, INPUT_PULLUP);
  pinMode(PIN_TESTE_REARMAR, INPUT_PULLUP);
  dht.setup(PIN_DHT, DHTesp::DHT22);
#else
  dht.setup(PIN_DHT, DHTesp::DHT11);
  iniciarCamera();
#endif
  comandos = xQueueCreate(1, sizeof(char));
  if (!comandos || xTaskCreate(tarefaMotor, "motor", 3072, nullptr, 3, nullptr) != pdPASS) {
    Serial.println("Falha ao iniciar controle: motor desabilitado.");
    for (;;) delay(1000);
  }
  WiFi.mode(WIFI_STA);
  if (WIFI_SSID[0]) WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setBufferSize(1024);
  mqtt.setSocketTimeout(1);
  Serial.println(PETCARE_SIMULATION ? "PERFIL WOKWI: camera e potencia nao simuladas." : "PERFIL FISICO: ESP32-S3 / DHT11 / TB6612.");
  Serial.println("Comandos: d=1 porcao, p=parar, r=rearmar falha. Timeout=8 segundos.");
}

void loop() {
  while (Serial.available()) solicitar((char)Serial.read());
#if PETCARE_SIMULATION
  // Debounce dos botoes de teste, sem repetir ao mante-los pressionados.
  static bool bruto[2] = {false, false}, estavel[2] = {false, false};
  static uint32_t mudanca[2] = {0, 0};
  const int pinos[2] = {PIN_TESTE_DISPENSAR, PIN_TESTE_REARMAR};
  for (int i = 0; i < 2; ++i) {
    bool pressionado = digitalRead(pinos[i]) == LOW;
    if (pressionado != bruto[i]) { bruto[i] = pressionado; mudanca[i] = millis(); }
    if (millis() - mudanca[i] >= 30 && estavel[i] != bruto[i]) {
      estavel[i] = bruto[i];
      if (estavel[i]) solicitar(i == 0 ? 'd' : 'r');
    }
  }
#endif
  const ControlePorcao c = estadoMotor();
  if (ultimoTotal != c.porcoes) {
    Serial.println("PORCAO_CONCLUIDA");
    ultimoTotal = c.porcoes;
  }
  if (ultimoEstado != int(c.estado)) {
    Serial.print("MOTOR: "); Serial.println(nomeEstado(c.estado));
    ultimoEstado = int(c.estado);
  }
  if (millis() - ultimaLeitura >= 2000) {
    ultimaLeitura = millis();
    TempAndHumidity leitura = dht.getTempAndHumidity();
    temperatura = leitura.temperature;
    umidade = leitura.humidity;
  }
  manterRede();
#if !PETCARE_SIMULATION
  manterServidorCamera();
#endif
  if (millis() - ultimaPublicacao >= 2000) {
    ultimaPublicacao = millis();
    publicar();
  }
  delay(1);
}
