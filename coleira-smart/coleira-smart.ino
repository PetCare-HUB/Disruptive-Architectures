/*
  PetCare Hub — Coleira Smart

  Objetivo:
  - Simular uma coleira inteligente no Wokwi
  - Ler movimentação pelo MPU6050
  - Classificar o pet como repouso, ativo ou muito ativo
  - Mostrar os dados em JSON no Serial Monitor

  Importante:
  - Este módulo NAO mede temperatura corporal.
  - A temperatura corporal foi removida porque a aferição confiável em pets
    normalmente exige contato adequado, geralmente por via retal em contexto veterinário.
  - A coleira mede apenas movimentação.

  Bibliotecas necessárias:
  - Nenhuma biblioteca externa.
  - Usa apenas Wire, que já vem com Arduino/ESP32.
*/

#include <Wire.h>

#define MPU6050_ADDR 0x68

const float LIMITE_REPOUSO = 0.25;
const float LIMITE_ATIVO = 1.20;

unsigned long ultimoMovimento = 0;
const unsigned long TEMPO_INATIVIDADE = 15000; // 15 segundos para teste no Wokwi

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
  Wire.beginTransmission(MPU6050_ADDR);

  if (Wire.endTransmission() != 0) {
    return false;
  }

  // Tira o MPU6050 do modo sleep
  escreverRegistroMPU(0x6B, 0x00);

  // Configura acelerometro para faixa de +/- 2g
  escreverRegistroMPU(0x1C, 0x00);

  return true;
}

void lerAceleracao(float &ax, float &ay, float &az) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x3B); // Registrador inicial da aceleracao
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 6, true);

  int16_t rawX = lerInt16MPU();
  int16_t rawY = lerInt16MPU();
  int16_t rawZ = lerInt16MPU();

  // Em +/- 2g, a sensibilidade aproximada é 16384 LSB/g
  ax = rawX / 16384.0;
  ay = rawY / 16384.0;
  az = rawZ / 16384.0;
}

String definirStatus(float movimento) {
  if (movimento < LIMITE_REPOUSO) {
    return "repouso";
  }

  if (movimento < LIMITE_ATIVO) {
    return "ativo";
  }

  return "muito_ativo";
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Iniciando Coleira Smart...");

  Wire.begin(21, 22); // SDA = GPIO 21, SCL = GPIO 22

  if (!iniciarMPU6050()) {
    Serial.println("Erro: MPU6050 nao encontrado. Verifique o diagram.json.");
    while (true) {
      delay(1000);
    }
  }

  ultimoMovimento = millis();

  Serial.println("Coleira Smart iniciada com sucesso.");
}

void loop() {
  float ax, ay, az;
  lerAceleracao(ax, ay, az);

  /*
    O eixo Z fica perto de 1g quando parado por causa da gravidade.
    Por isso usamos abs(az - 1.0) para remover a gravidade aproximada.
  */
  float movimento = abs(ax) + abs(ay) + abs(az - 1.0);

  String status = definirStatus(movimento);

  if (status != "repouso") {
    ultimoMovimento = millis();
  }

  bool alertaInatividade = (millis() - ultimoMovimento) > TEMPO_INATIVIDADE;

  Serial.print("{");
  Serial.print("\"modulo\":\"coleira\",");
  Serial.print("\"movimento\":");
  Serial.print(movimento, 2);
  Serial.print(",\"status\":\"");
  Serial.print(status);
  Serial.print("\",");
  Serial.print("\"alerta\":");
  Serial.print(alertaInatividade ? "true" : "false");
  Serial.println("}");

  delay(2000);
}
