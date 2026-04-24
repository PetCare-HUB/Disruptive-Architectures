# 🐾 PetCare Hub — Sistema Integrado de Monitoramento de Pets
 
> Projeto desenvolvido para a disciplina **Disruptive Architectures: IoT, IoB & Generative IA**  
> FIAP — 1º Sprint
 
---
 
## 📋 Sumário
 
- [Sobre o Projeto](#sobre-o-projeto)
- [Problema Real](#problema-real)
- [Solução Proposta](#solução-proposta)
- [Arquitetura do Sistema](#arquitetura-do-sistema)
- [Módulos do Projeto](#módulos-do-projeto)
- [Tecnologias Utilizadas](#tecnologias-utilizadas)
- [Componentes e Sensores](#componentes-e-sensores)
- [Protocolo de Comunicação](#protocolo-de-comunicação)
- [Dashboard](#dashboard)
- [Simulação no Wokwi](#simulação-no-wokwi)
- [Como Executar](#como-executar)
- [Estrutura do Repositório](#estrutura-do-repositório)
- [Resultados Parciais](#resultados-parciais)
- [Próximos Passos](#próximos-passos)
- [Integrantes](#integrantes)
---
 
## 📌 Sobre o Projeto
 
O **PetCare Hub** é um sistema de monitoramento inteligente para pets que integra três módulos de IoT:
 
1. **Comedouro Inteligente** — controle e histórico de alimentação
2. **Coleira Smart** — monitoramento de temperatura corporal e atividade física
3. **Monitor de Ambiente** — temperatura, umidade, qualidade do ar e presença do pet
Todos os módulos enviam dados via **MQTT** para um broker na nuvem e alimentam um **Dashboard em tempo real** com alertas automáticos.
 
---
 
## ❗ Problema Real
 
Donos de pets enfrentam dificuldades para monitorar a saúde e o bem-estar dos animais quando estão fora de casa:
 
- Não sabem se o pet comeu ou bebeu água no dia
- Não conseguem detectar sinais precoces de febre ou doença
- Não têm controle sobre as condições do ambiente em que o animal fica
- Não recebem alertas imediatos em situações de risco
---
 
## 💡 Solução Proposta
 
Um sistema de IoT de baixo custo baseado em **ESP32**, sensores específicos e comunicação **MQTT**, que coleta dados em tempo real e os exibe em um **dashboard centralizado**, enviando alertas automáticos ao tutor.
 
---
 
## 🏗️ Arquitetura do Sistema
 
```
┌──────────────────────────────────────────────────────────┐
│                   ESP32 (Cérebro Central)                │
│                                                          │
│   [Módulo 1]         [Módulo 2]        [Módulo 3]        │
│   Comedouro          Coleira           Ambiente          │
│   HC-SR04            DS18B20           DHT22             │
│   HX711              MPU6050           MQ-135            │
│   Servo Motor        --------          PIR HC-SR501      │
│   LCD I2C                                                │
└─────────────────────────┬────────────────────────────────┘
                          │ WiFi
                   ┌──────▼──────┐
                   │  Broker     │
                   │  MQTT       │
                   │ (HiveMQ)    │
                   └──────┬──────┘
                          │
                   ┌──────▼──────┐
                   │  Dashboard  │
                   │ (Node-RED)  │
                   └─────────────┘
```
 
---
 
## 📦 Módulos do Projeto
 
### 🍽️ Módulo 1 — Comedouro Inteligente
 
Monitora o nível de ração/água e o consumo do pet ao longo do dia.
 
| Componente | Função |
|---|---|
| HC-SR04 | Mede o nível de ração no pote (cm) |
| Célula de carga + HX711 | Calcula o peso consumido (gramas) |
| Servo Motor | Libera porção automática |
| Display LCD I2C 16x2 | Exibe status localmente |
 
**Lógica:**
- Nível de ração < 20% → aciona servo → envia alerta `ALERTA: Ração baixa!`
- Registra horário e quantidade de cada refeição
---
 
### 🐾 Módulo 2 — Coleira Smart
 
Monitora a saúde e o comportamento do pet em tempo real.
 
| Componente | Função |
|---|---|
| DS18B20 | Temperatura corporal do animal (°C) |
| MPU6050 | Acelerômetro + Giroscópio — detecta movimento |
 
**Estados detectados:**
 
| Aceleração | Temperatura | Status |
|---|---|---|
| Baixa | Normal | 😴 Dormindo |
| Alta | Normal | 🏃 Brincando/Ativo |
| Qualquer | > 39.5°C | 🚨 Alerta: possível febre |
 
---
 
### 🌡️ Módulo 3 — Monitor de Ambiente
 
Garante que o ambiente onde o pet fica seja seguro e confortável.
 
| Componente | Função |
|---|---|
| DHT22 | Temperatura e umidade do cômodo |
| MQ-135 | Qualidade do ar (ppm) |
| PIR HC-SR501 | Detecta presença do pet no local |
 
**Alertas automáticos:**
- Temperatura ambiente > 30°C → `ALERTA: Ambiente muito quente!`
- Umidade < 30% ou > 80% → `ALERTA: Umidade inadequada`
- Qualidade do ar ruim → `ALERTA: Ar comprometido`
---
 
## 🛠️ Tecnologias Utilizadas
 
| Camada | Ferramenta | Justificativa |
|---|---|---|
| Microcontrolador | **ESP32** | WiFi integrado, múltiplos GPIO, suporte MQTT nativo |
| Simulação | **Wokwi** | Simulação de circuitos com ESP32 online e gratuito |
| Firmware | **C++ / Arduino IDE** | Amplamente suportado para ESP32 |
| Protocolo IoT | **MQTT** | Leve, eficiente, ideal para dispositivos embarcados |
| Broker | **HiveMQ Cloud** | Broker MQTT gratuito e estável na nuvem |
| Dashboard | **Node-RED** | Fluxo visual de dados, integração MQTT nativa |
| Visualização | **Node-RED UI** | Gráficos e alertas em tempo real |
 
---
 
## 🔌 Componentes e Sensores
 
| Sensor/Atuador | Módulo | Protocolo |
|---|---|---|
| HC-SR04 (Ultrassônico) | Comedouro | Digital (Trigger/Echo) |
| HX711 + Célula de Carga | Comedouro | SPI |
| Servo Motor SG90 | Comedouro | PWM |
| LCD I2C 16x2 | Comedouro | I2C |
| DS18B20 | Coleira | OneWire |
| MPU6050 | Coleira | I2C |
| DHT22 | Ambiente | Digital |
| MQ-135 | Ambiente | Analógico (ADC) |
| PIR HC-SR501 | Ambiente | Digital |
 
---
 
## 📡 Protocolo de Comunicação
 
O sistema utiliza **MQTT** com os seguintes tópicos:
 
```
petcare/comedouro/nivel       → Nível de ração (%)
petcare/comedouro/peso        → Peso no pote (g)
petcare/comedouro/alerta      → Alerta de ração baixa
 
petcare/coleira/temperatura   → Temp. corporal (°C)
petcare/coleira/atividade     → Status (dormindo/ativo/brincando)
petcare/coleira/alerta        → Alerta de febre
 
petcare/ambiente/temperatura  → Temp. ambiente (°C)
petcare/ambiente/umidade      → Umidade (%)
petcare/ambiente/ar           → Qualidade do ar (ppm)
petcare/ambiente/presenca     → Pet presente (true/false)
petcare/ambiente/alerta       → Alertas de ambiente
```
 
---
 
## 📊 Dashboard
 
O dashboard foi construído no **Node-RED UI** e apresenta:
 
- 📈 Gráfico de nível de ração ao longo do dia
- ⚖️ Histórico de peso consumido por refeição
- 🌡️ Temperatura corporal do pet em tempo real
- 🏃 Status de atividade com ícones visuais
- 🌬️ Condições do ambiente (temp, umidade, ar)
- 🔔 Painel de alertas com timestamp
---
 
## 🧪 Simulação no Wokwi
 
O protótipo foi simulado na plataforma **[Wokwi](https://wokwi.com)**.
 
🔗 **Link da simulação:** `[inserir link do projeto Wokwi aqui]`
 
**Como acessar:**
1. Acesse o link da simulação
2. Clique em ▶️ **Play** para iniciar
3. Observe os dados sendo publicados no Serial Monitor
4. Os dados são enviados ao broker MQTT e refletidos no dashboard
---
 
## 🚀 Como Executar
 
### Pré-requisitos
 
- [Arduino IDE](https://www.arduino.cc/en/software) com suporte ao ESP32
- Conta gratuita no [HiveMQ Cloud](https://www.hivemq.com/mqtt-cloud-broker/)
- [Node-RED](https://nodered.org/) instalado localmente ou via Docker
### 1. Configurar o Firmware
 
```bash
# Clone o repositório
git clone https://github.com/seu-usuario/petcare-hub.git
cd petcare-hub/firmware
```
 
Edite o arquivo `config.h` com suas credenciais:
 
```cpp
#define WIFI_SSID     "sua_rede_wifi"
#define WIFI_PASSWORD "sua_senha"
#define MQTT_BROKER   "seu_broker.hivemq.cloud"
#define MQTT_PORT     8883
#define MQTT_USER     "seu_usuario"
#define MQTT_PASSWORD "sua_senha_mqtt"
```
 
### 2. Instalar Bibliotecas (Arduino IDE)
 
```
- PubSubClient (MQTT)
- DHT sensor library (Adafruit)
- HX711 (bogde)
- MPU6050 (Electronic Cats)
- OneWire + DallasTemperature (DS18B20)
- LiquidCrystal_I2C
```
 
### 3. Configurar o Node-RED
 
```bash
# Instalar Node-RED
npm install -g --unsafe-perm node-red
 
# Iniciar
node-red
```
 
1. Acesse `http://localhost:1880`
2. Importe o arquivo `dashboard/flows.json`
3. Configure o nó MQTT com as credenciais do broker
4. Acesse o dashboard em `http://localhost:1880/ui`
---
 
## 📁 Estrutura do Repositório
 
```
petcare-hub/
│
├── firmware/
│   ├── petcare_hub.ino        # Código principal ESP32
│   ├── config.h               # Credenciais WiFi e MQTT
│   ├── modulo_comedouro.h     # Lógica do comedouro
│   ├── modulo_coleira.h       # Lógica da coleira
│   └── modulo_ambiente.h      # Lógica do ambiente
│
├── dashboard/
│   ├── flows.json             # Fluxo Node-RED exportado
│   └── screenshots/           # Capturas do dashboard funcionando
│
├── wokwi/
│   ├── diagram.json           # Diagrama do circuito Wokwi
│   └── sketch.ino             # Código para simulação
│
├── docs/
│   ├── arquitetura.png        # Diagrama de arquitetura
│   ├── circuito.png           # Foto/print do circuito
│   └── apresentacao.pdf       # Slides do pitch
│
├── video/
│   └── link.txt               # Link do vídeo no YouTube
│
└── README.md
```
 
---
 
## 📈 Resultados Parciais
 
### ✅ Implementado no Sprint 1
 
- [x] Simulação do ESP32 com sensor DHT22 no Wokwi
- [x] Simulação do sensor HC-SR04 (nível de ração)
- [x] Publicação de dados via MQTT no HiveMQ Cloud
- [x] Dashboard básico no Node-RED com gráficos em tempo real
- [x] Alerta de nível crítico de ração funcionando
- [x] Display LCD exibindo status local
### 🔄 Em desenvolvimento
 
- [ ] Integração do MPU6050 (acelerômetro — coleira)
- [ ] Módulo de qualidade do ar (MQ-135)
- [ ] Histórico de alimentação com persistência de dados
- [ ] Notificação push via Telegram Bot
---
 
## 🔭 Próximos Passos
 
| Sprint | Objetivo |
|---|---|
| Sprint 2 | Integrar Visão Computacional (detecção do pet por câmera) |
| Sprint 3 | App mobile para visualização do dashboard |
| Sprint 4 | IA generativa para análise de comportamento e saúde |
 
---
 
## 👥 Integrantes da Equipe

| Nome                           | RM      | Turma   | GitHub                                        | LinkedIn                                                                |
| ------------------------------ | ------- | ------- | --------------------------------------------- | ----------------------------------------------------------------------- |
| Alexander Dennis Isidro Mamani | 565554  | 2TDSPG  | [alex-isidro](https://github.com/alex-isidro) | [LinkedIn](https://www.linkedin.com/in/alexander-dennis-a3b48824b/)     |
| Kelson Zhang                   | 563748  | 2TDSPG  | [KelsonZh0](https://github.com/KelsonZh0)     | [LinkedIn](https://www.linkedin.com/in/kelson-zhang-211456323/)         |
---
 
## 📄 Licença
 
Este projeto foi desenvolvido para fins acadêmicos na **FIAP**.
 
---
 
> 🐶 *"Tecnologia a serviço do bem-estar animal"*
