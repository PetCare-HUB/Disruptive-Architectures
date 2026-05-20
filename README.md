# 🐾 PetCare Hub — Coleira Smart IoT

Protótipo IoT desenvolvido para o **Challenge FIAP 2026 — CLYVO VET**.

A proposta da **Coleira Smart** é monitorar a movimentação do pet em tempo real usando um **ESP32** com sensor **MPU6050**, classificando o comportamento do animal como:

- `repouso`
- `ativo`
- `muito_ativo`

Os dados são enviados via **MQTT** para um dashboard web, permitindo acompanhar a atividade do pet em tempo real.

---

## 🎯 Objetivo do Projeto

O objetivo deste protótipo é demonstrar como tecnologias de IoT podem apoiar o cuidado contínuo e preventivo de pets.

A coleira coleta dados de movimentação e envia essas informações para um dashboard, permitindo que tutores e clínicas acompanhem padrões de atividade do animal.

Esse monitoramento pode ajudar na identificação de comportamentos relevantes, como:

- baixa movimentação por muito tempo;
- aumento incomum de atividade;
- períodos de repouso;
- comportamento compatível com passeio ou corrida;
- alertas de inatividade.

---

## 🧠 Problema que o Projeto Resolve

Atualmente, o cuidado veterinário ainda é muito reativo. Na maioria dos casos, o tutor procura a clínica apenas quando percebe sintomas claros ou em situações de emergência.

A **Coleira Smart** propõe uma forma de acompanhamento contínuo, permitindo gerar dados sobre o comportamento do pet antes que um possível problema se agrave.

---

## 🔌 Tecnologias Utilizadas

| Tecnologia | Uso no projeto |
|---|---|
| ESP32 | Microcontrolador principal |
| MPU6050 | Sensor de aceleração/movimento |
| Wokwi | Simulação do circuito |
| C++ / Arduino | Programação do firmware |
| MQTT | Comunicação entre ESP32 e dashboard |
| HiveMQ Broker | Broker MQTT público utilizado nos testes |
| HTML | Estrutura do dashboard |
| CSS | Estilização do dashboard |
| JavaScript | Recebimento MQTT e atualização da interface |

---

## 🧩 Arquitetura da Solução

```text
MPU6050
   ↓
ESP32
   ↓ Wi-Fi
MQTT Broker HiveMQ
   ↓
Dashboard Web
```

Fluxo completo:

```text
Sensor lê o movimento
        ↓
ESP32 calcula a velocidade de movimento
        ↓
ESP32 classifica o pet
        ↓
ESP32 publica os dados via MQTT
        ↓
Dashboard assina o tópico MQTT
        ↓
Dashboard exibe os dados em tempo real
```

---

## 📡 Comunicação MQTT

O projeto utiliza **MQTT** para enviar os dados da coleira para o dashboard.

MQTT é um protocolo leve muito usado em IoT. Ele permite que um dispositivo pequeno, como o ESP32, publique mensagens em um canal chamado **tópico**, enquanto outro sistema, como o dashboard, assina esse mesmo tópico para receber os dados em tempo real.

### Broker utilizado

```text
broker.hivemq.com
```

### Porta MQTT usada pelo ESP32

```text
1883
```

### WebSocket MQTT usado pelo dashboard

```text
wss://broker.hivemq.com:8884/mqtt
```

### Tópicos MQTT

```text
petcarehub/fiap/coleira01/telemetria
petcarehub/fiap/coleira01/status
petcarehub/fiap/coleira01/alerta
```

### Tópico principal

```text
petcarehub/fiap/coleira01/telemetria
```

Esse tópico recebe os dados completos da coleira em formato JSON.

---

## 📊 Exemplo de JSON Enviado

```json
{
  "modulo": "coleira_smart",
  "petId": "rex-001",
  "sensor": "MPU6050",
  "ax": 0.012,
  "ay": -0.031,
  "az": 1.002,
  "variacaoAceleracao": 0.145,
  "velocidadeMovimento": 1.230,
  "velocidadeMovimentoFiltrada": 1.050,
  "velocidadeMediaJanela": 0.980,
  "statusBruto": "ativo",
  "status": "ativo",
  "bateria": 98,
  "alertaInatividade": false,
  "wifi": true,
  "mqtt": true,
  "modoDemo": true,
  "uptimeMs": 12000
}
```

---

## 🐕 Classificação de Atividade

A coleira não calcula velocidade real em km/h, pois isso exigiria GPS.

Em vez disso, o projeto calcula a **velocidade de movimento**, baseada na variação da aceleração medida pelo MPU6050.

### Regras utilizadas

| Situação | Classificação |
|---|---|
| Pouco ou nenhum movimento | `repouso` |
| Movimento moderado, semelhante a passeio | `ativo` |
| Movimento forte por tempo contínuo | `muito_ativo` |
| Pequeno pico isolado | Não muda imediatamente para `muito_ativo` |

O status `muito_ativo` só é confirmado quando o movimento intenso permanece por vários segundos. Isso evita falsos positivos causados por pequenas mexidas na coleira.

---

## 🧪 Modo Demo e Modo Físico

O código possui dois modos de funcionamento.

### Modo Demo

Usado no Wokwi para demonstrar automaticamente os estados da coleira.

```cpp
const bool MODO_DEMO = true;
```

Nesse modo, a simulação alterna entre `repouso`, `ativo` e `muito_ativo` para facilitar a apresentação.

### Modo Físico

Usado no ESP32 real com MPU6050 físico.

```cpp
const bool MODO_DEMO = false;
```

Nesse modo, o status muda apenas conforme o movimento real detectado pelo sensor.

---

## 🛠️ Estrutura do Projeto

```text
petcarehub-coleira-smart/
├── coleira-smart/
│   ├── coleira-smart.ino
│   ├── diagram.json
│   └── libraries.txt
│
├── dashboard-coleira/
│   ├── index.html
│   ├── css/
│   │   └── styles.css
│   └── js/
│       └── app.js
│
└── README.md
```

---

## ▶️ Como Executar no Wokwi

### 1. Abrir o projeto no Wokwi

Abra a pasta `coleira-smart` no Wokwi.

Ela deve conter:

```text
coleira-smart.ino
diagram.json
libraries.txt
```

### 2. Verificar o arquivo `libraries.txt`

O arquivo `libraries.txt` precisa conter:

```text
PubSubClient
```

Essa biblioteca é necessária para a comunicação MQTT.

### 3. Rodar a simulação

Clique em **Start Simulation**.

No Serial Monitor, devem aparecer mensagens parecidas com:

```json
{
  "modulo": "coleira_smart",
  "status": "ativo",
  "velocidadeMovimentoFiltrada": 1.05,
  "wifi": true,
  "mqtt": true
}
```

---

## 🖥️ Como Abrir o Dashboard

Abra o arquivo:

```text
dashboard-coleira/index.html
```

O dashboard irá se conectar ao broker MQTT e assinar o tópico:

```text
petcarehub/fiap/coleira01/telemetria
```

Quando o ESP32 publicar os dados, o dashboard será atualizado automaticamente.

---

## 🔧 Como Executar no ESP32 Físico

### 1. Instalar bibliotecas

Na Arduino IDE, instale:

```text
PubSubClient
```

Caminho:

```text
Sketch → Include Library → Manage Libraries → PubSubClient
```

### 2. Configurar Wi-Fi

No código `.ino`, altere:

```cpp
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";
```

Para:

```cpp
const char* WIFI_SSID = "NOME_DO_SEU_WIFI";
const char* WIFI_PASSWORD = "SENHA_DO_SEU_WIFI";
```

### 3. Desativar o modo demo

Altere:

```cpp
const bool MODO_DEMO = true;
```

Para:

```cpp
const bool MODO_DEMO = false;
```

### 4. Ligações do MPU6050

| MPU6050 | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### 5. Fazer upload para o ESP32

Na Arduino IDE:

```text
Tools → Board → ESP32 Dev Module
```

Depois clique em **Upload**.

---

## 📺 Demonstração em Vídeo

O vídeo de apresentação deve demonstrar:

1. O problema que o projeto resolve;
2. A simulação da coleira no Wokwi;
3. O envio dos dados via MQTT;
4. O dashboard recebendo os dados em tempo real;
5. A mudança entre `repouso`, `ativo` e `muito_ativo`;
6. O alerta de inatividade;
7. A explicação do modo demo e do modo físico.

Link do vídeo:

```text
Adicionar link do YouTube não listado aqui
```

---

## ✅ Funcionalidades Implementadas

- [x] Leitura do sensor MPU6050
- [x] Cálculo de variação de aceleração
- [x] Cálculo de velocidade de movimento
- [x] Classificação entre repouso, ativo e muito ativo
- [x] Filtro contra movimentos pequenos
- [x] Confirmação por tempo para evitar falso positivo
- [x] Envio de dados via MQTT
- [x] Dashboard web em tempo real
- [x] Exibição do JSON recebido
- [x] Gráfico de movimento
- [x] Alerta de inatividade
- [x] Compatibilidade com Wokwi e ESP32 físico

---

## ⚠️ Observações Técnicas

- O projeto não mede temperatura corporal do pet.
- A bateria exibida no dashboard é simulada.
- O status `muito_ativo` exige movimento intenso por tempo contínuo.
- O dashboard precisa usar o mesmo tópico MQTT configurado no ESP32.
- Em redes Wi-Fi públicas ou corporativas, a porta MQTT pode ser bloqueada.
- Para medição de velocidade real em km/h seria necessário adicionar GPS ao protótipo.

---

## 👥 Integrantes da Equipe

| Nome | RM | Turma | GitHub | LinkedIn |
|---|---|---|---|---|
| Alexander Dennis Isidro Mamani | 565554 | 2TDSPG | [alex-isidro](https://github.com/alex-isidro) | [LinkedIn](https://www.linkedin.com/in/alexander-dennis-a3b48824b/) |
| Kelson Zhang | 563748 | 2TDSPG | [KelsonZh0](https://github.com/KelsonZh0) | [LinkedIn](https://www.linkedin.com/in/kelson-zhang-211456323/) |

---
