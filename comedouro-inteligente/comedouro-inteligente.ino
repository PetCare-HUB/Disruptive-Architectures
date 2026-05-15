/*
  PetCare Hub — Comedouro Inteligente

  Objetivo:
  - Simular um comedouro inteligente no Wokwi
  - Medir nivel de racao com HC-SR04
  - Simular peso de racao com base em uma variavel controlada
  - Medir temperatura ambiente com DHT22
  - Simular liberacao de racao com Servo Motor
  - Mostrar os dados em JSON no Serial Monitor

  Importante:
  - A temperatura medida aqui é temperatura AMBIENTE.
  - O projeto NAO mede temperatura corporal do pet.

  Biblioteca necessária:
  - DHT sensor library for ESPx
*/

#include <DHTesp.h>

#define DHT_PIN 15

#define TRIG_PIN 5
#define ECHO_PIN 18

#define HX711_DT 32
#define HX711_SCK 33

#define SERVO_PIN 13

const float ALTURA_RESERVATORIO_CM = 30.0;
const float DISTANCIA_CHEIO_CM = 5.0;

const int NIVEL_MINIMO_RACAO = 20;
const float PESO_MINIMO_RACAO = 150.0;
const float TEMPERATURA_MAXIMA = 32.0;

DHTesp dhtSensor;

unsigned long ultimaLiberacao = 0;
const unsigned long INTERVALO_LIBERACAO = 15000; // 15 segundos para teste no Wokwi

float pesoSimulado = 500.0;

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

  float distancia = duracao / 58.0;
  return distancia;
}

int calcularNivelRacao(float distanciaCm) {
  float nivel = ((ALTURA_RESERVATORIO_CM - distanciaCm) /
                (ALTURA_RESERVATORIO_CM - DISTANCIA_CHEIO_CM)) * 100.0;

  if (nivel < 0) {
    nivel = 0;
  }

  if (nivel > 100) {
    nivel = 100;
  }

  return (int)nivel;
}

float lerPesoRacao() {
  /*
    Nesta primeira versao, o peso é simulado para facilitar o teste.
    O HX711 fica no diagrama para representar o sensor real.

    Em uma fase seguinte, podemos trocar esta funcao pela leitura real da celula de carga.
  */

  pesoSimulado -= 20.0;

  if (pesoSimulado < 80.0) {
    pesoSimulado = 500.0;
  }

  return pesoSimulado;
}

void escreverServo(int angulo) {
  /*
    Gera pulso manual para o servo sem depender da biblioteca ESP32Servo.
    Isso evita erro de biblioteca no Arduino IDE.
  */

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

String definirStatus(int nivelRacao, float pesoRacao, float temperaturaAmbiente) {
  if (nivelRacao < NIVEL_MINIMO_RACAO || pesoRacao < PESO_MINIMO_RACAO) {
    return "racao_baixa";
  }

  if (temperaturaAmbiente > TEMPERATURA_MAXIMA) {
    return "temperatura_elevada";
  }

  return "normal";
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Iniciando Comedouro Inteligente...");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(HX711_DT, INPUT);
  pinMode(HX711_SCK, OUTPUT);

  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, LOW);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  Serial.println("Comedouro Inteligente iniciado com sucesso.");
}

void loop() {
  float distancia = lerDistanciaCm();
  int nivelRacao = calcularNivelRacao(distancia);

  float pesoRacao = lerPesoRacao();

  TempAndHumidity leituraDht = dhtSensor.getTempAndHumidity();
  float temperaturaAmbiente = leituraDht.temperature;

  if (isnan(temperaturaAmbiente)) {
    temperaturaAmbiente = 0.0;
  }

  String status = definirStatus(nivelRacao, pesoRacao, temperaturaAmbiente);
  bool alerta = status != "normal";

  if (pesoRacao < PESO_MINIMO_RACAO && millis() - ultimaLiberacao > INTERVALO_LIBERACAO) {
    liberarRacao();
    ultimaLiberacao = millis();
  }

  Serial.print("{");
  Serial.print("\"modulo\":\"comedouro\",");
  Serial.print("\"nivel_racao\":");
  Serial.print(nivelRacao);
  Serial.print(",\"peso_racao\":");
  Serial.print(pesoRacao, 1);
  Serial.print(",\"temperatura_ambiente\":");
  Serial.print(temperaturaAmbiente, 1);
  Serial.print(",\"status\":\"");
  Serial.print(status);
  Serial.print("\",");
  Serial.print("\"alerta\":");
  Serial.print(alerta ? "true" : "false");
  Serial.println("}");

  delay(2000);
}
