#pragma once

// 1: teste no Wokwi. 0: placa fisica ESP32-S3 N16R8.
#ifndef PETCARE_SIMULATION
#define PETCARE_SIMULATION 1
#endif

// Confirmar o mapa da placa OEM antes de habilitar a camera e o motor fisicos.
// A compilacao fisica permite primeiro testar DHT11 e diagnosticar a camera.
#define CAMERA_PINOUT_CONFIRMADO false
#define MOTOR_FISICO_HABILITADO false

constexpr int PIN_DHT = 42;
constexpr int PIN_FIM_CURSO = 41;
constexpr int PIN_AIN1 = 1;
constexpr int PIN_AIN2 = 2;
constexpr int PIN_PWMA = 38;
constexpr int PIN_STBY = 40;
constexpr bool FIM_CURSO_ATIVO_BAIXO = true; // COM -> GND, NO -> GPIO41
constexpr unsigned long TEMPO_MAXIMO_PORCAO_MS = 8000; // ajustar na bancada
constexpr unsigned long TEMPO_FREIO_MS = 150; // ajustar na bancada

// Apenas botoes de teste no Wokwi; no fisico use o Serial Monitor.
constexpr int PIN_TESTE_DISPENSAR = 21;
constexpr int PIN_TESTE_REARMAR = 47;

#if PETCARE_SIMULATION
constexpr const char* WIFI_SSID = "Wokwi-GUEST";
constexpr const char* WIFI_PASSWORD = "";
#else
constexpr const char* WIFI_SSID = ""; // preencher localmente
constexpr const char* WIFI_PASSWORD = "";
#endif

// Somente telemetria no broker publico. Comandos do motor sao locais.
constexpr const char* MQTT_BROKER = "broker.hivemq.com";
constexpr int MQTT_PORT = 1883;
constexpr const char* TOPICO_TELEMETRIA = "petcarehub/fiap/comedouro01/telemetria";

// Referencia candidata: ESP32-S3-EYE / Freenove S3 WROOM.
// NAO e uma identificacao confirmada da placa OEM SmartKits.
// https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board
constexpr int CAM_XCLK = 15, CAM_SDA = 4, CAM_SCL = 5;
constexpr int CAM_D0 = 11, CAM_D1 = 9, CAM_D2 = 8, CAM_D3 = 10;
constexpr int CAM_D4 = 12, CAM_D5 = 18, CAM_D6 = 17, CAM_D7 = 16;
constexpr int CAM_VSYNC = 6, CAM_HREF = 7, CAM_PCLK = 13;
