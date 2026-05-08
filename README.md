# 🐾 PetCare Hub — Sistema Integrado de Monitoramento de Pets

> Projeto desenvolvido para a disciplina **Disruptive Architectures: IoT, IoB & Generative IA**  
> FIAP — Challenge 2026

---

## 📌 Sobre o Projeto

O **PetCare Hub** é um sistema IoT para auxiliar tutores no acompanhamento da rotina dos pets por meio de sensores, comunicação MQTT e dashboard em tempo real.

O projeto é composto por **dois módulos inteligentes**:

1. **Coleira Smart** — monitora a movimentação do animal.
2. **Comedouro Inteligente** — monitora alimentação, nível de ração, peso disponível e temperatura ambiente.

Os dados coletados pelos módulos são enviados via **MQTT** para um broker na nuvem e exibidos em um **dashboard em tempo real**, permitindo o acompanhamento do comportamento e do ambiente do pet.

---

## ⚠️ Ajuste de Escopo Técnico

Inicialmente, o projeto previa a medição de temperatura corporal pela coleira. Porém, essa abordagem foi removida por não representar uma medição confiável.

A temperatura corporal de cães e gatos normalmente exige contato direto adequado, sendo comumente aferida por via retal em contexto veterinário. Como a coleira fica posicionada no pescoço do animal, ela não seria adequada para representar a temperatura corporal real do pet.

Dessa forma, o projeto foi ajustado para:

- Monitorar **movimentação do animal** pela coleira;
- Monitorar **alimentação e temperatura ambiente** pelo comedouro.

Esse ajuste torna o projeto mais realista, viável e coerente com uma solução IoT acadêmica.

---

## ❗ Problema Real

Donos de pets enfrentam dificuldades para acompanhar a rotina dos animais quando estão fora de casa.

Entre os principais problemas estão:

- Não saber se o pet se movimentou durante o dia;
- Não conseguir identificar longos períodos de inatividade;
- Não saber se ainda há ração disponível;
- Não acompanhar a quantidade aproximada de alimento no comedouro;
- Não monitorar a temperatura do ambiente onde o pet se alimenta;
- Não receber alertas automáticos em situações de risco.

---

## 💡 Solução Proposta

A solução proposta é um sistema IoT de baixo custo baseado em **ESP32**, sensores e comunicação **MQTT**.

O sistema coleta dados em tempo real por meio de dois módulos:

- **Coleira Smart**, responsável pela leitura da movimentação do animal;
- **Comedouro Inteligente**, responsável pela leitura de ração, peso disponível e temperatura ambiente.

Essas informações são enviadas para um broker MQTT e exibidas em um dashboard centralizado, permitindo que o tutor acompanhe o status do pet de forma simples e rápida.

---

## 🏗️ Arquitetura do Sistema

```text
┌──────────────────────────────┐
│        Coleira Smart         │
│                              │
│ ESP32                        │
│ MPU6050                      │
│                              │
│ Captura movimentação         │
│ do animal                    │
└──────────────┬───────────────┘
               │ Wi-Fi / MQTT
               ▼
        ┌─────────────┐
        │ Broker MQTT │
        │ HiveMQ      │
        └──────┬──────┘
               ▼
        ┌─────────────┐
        │ Dashboard   │
        │ Node-RED    │
        └─────────────┘


┌──────────────────────────────┐
│    Comedouro Inteligente     │
│                              │
│ ESP32                        │
│ HC-SR04                      │
│ HX711 + Célula de carga      │
│ Servo Motor                  │
│ DHT22                        │
│                              │
│ Captura alimentação e        │
│ temperatura ambiente         │
└──────────────┬───────────────┘
               │ Wi-Fi / MQTT
               ▼
        ┌─────────────┐
        │ Broker MQTT │
        │ HiveMQ      │
        └──────┬──────┘
               ▼
        ┌─────────────┐
        │ Dashboard   │
        │ Node-RED    │
        └─────────────┘
```

Cada módulo possui seu próprio **ESP32**, pois a coleira e o comedouro são dispositivos fisicamente separados.

---

## 📦 Módulos do Projeto

---

## 🐾 Módulo 1 — Coleira Smart

A **Coleira Smart** tem como objetivo monitorar a movimentação do animal durante o dia.

Ela utiliza um sensor de movimento para identificar se o pet está parado, ativo ou muito ativo.

### Componentes

| Componente | Função |
|---|---|
| ESP32 | Microcontrolador com conexão Wi-Fi |
| MPU6050 | Acelerômetro e giroscópio para detectar movimento |

### Dados coletados

| Dado | Descrição |
|---|---|
| Movimento | Valor calculado a partir da aceleração |
| Status | Estado do animal: repouso, ativo ou muito ativo |
| Alerta | Indica possível inatividade prolongada |

### Estados detectados

| Condição | Status |
|---|---|
| Baixo movimento | Repouso |
| Movimento médio | Ativo |
| Movimento alto | Muito ativo |
| Muito tempo sem movimento | Alerta de inatividade |

### Exemplo de JSON enviado

```json
{
  "modulo": "coleira",
  "movimento": 1.82,
  "status": "ativo",
  "alerta": false
}
```

---

## 🍽️ Módulo 2 — Comedouro Inteligente

O **Comedouro Inteligente** tem como objetivo monitorar a alimentação do pet e a temperatura ambiente próxima ao local onde ele se alimenta.

Esse módulo verifica o nível de ração, o peso disponível no pote e também pode liberar uma porção de ração por meio de um servo motor.

### Componentes

| Componente | Função |
|---|---|
| ESP32 | Microcontrolador com conexão Wi-Fi |
| HC-SR04 | Mede o nível de ração no reservatório |
| HX711 + Célula de carga | Mede o peso da ração disponível |
| Servo Motor SG90 | Libera uma porção de ração |
| DHT22 | Mede a temperatura ambiente |

### Dados coletados

| Dado | Descrição |
|---|---|
| Nível de ração | Percentual aproximado de ração no reservatório |
| Peso da ração | Quantidade aproximada de ração disponível em gramas |
| Temperatura ambiente | Temperatura próxima ao comedouro |
| Status | Situação atual do comedouro |
| Alerta | Indica ração baixa ou temperatura elevada |

### Alertas automáticos

- Ração abaixo do nível mínimo;
- Peso da ração abaixo do esperado;
- Temperatura ambiente elevada;
- Falha simulada na liberação de ração.

### Exemplo de JSON enviado

```json
{
  "modulo": "comedouro",
  "nivel_racao": 35,
  "peso_racao": 420,
  "temperatura_ambiente": 28.5,
  "status": "normal",
  "alerta": false
}
```

---

## 🛠️ Tecnologias Utilizadas

| Camada | Ferramenta | Justificativa |
|---|---|---|
| Microcontrolador | ESP32 | Possui Wi-Fi integrado e bom suporte para sensores |
| Simulação | Wokwi | Permite simular ESP32 e sensores online |
| Firmware | C++ / Arduino | Linguagem comum para projetos com ESP32 |
| Protocolo IoT | MQTT | Protocolo leve, eficiente e adequado para IoT |
| Broker MQTT | HiveMQ Cloud | Broker MQTT em nuvem para testes |
| Dashboard | Node-RED | Permite criar fluxos visuais e dashboards |
| Visualização | Node-RED Dashboard | Exibe gráficos, cards e alertas em tempo real |

---

## 🔌 Componentes e Sensores

| Sensor/Atuador | Módulo | Protocolo/Sinal |
|---|---|---|
| ESP32 | Coleira Smart | Wi-Fi / GPIO |
| MPU6050 | Coleira Smart | I2C |
| ESP32 | Comedouro Inteligente | Wi-Fi / GPIO |
| HC-SR04 | Comedouro Inteligente | Digital Trigger/Echo |
| HX711 + Célula de carga | Comedouro Inteligente | Digital |
| Servo Motor SG90 | Comedouro Inteligente | PWM |
| DHT22 | Comedouro Inteligente | Digital |

---

## 📡 Protocolo de Comunicação

O sistema utiliza **MQTT** para comunicação entre os módulos IoT e o dashboard.

### Tópicos da Coleira Smart

```text
petcare/coleira/movimento
petcare/coleira/status
petcare/coleira/alerta
```

### Tópicos do Comedouro Inteligente

```text
petcare/comedouro/nivel
petcare/comedouro/peso
petcare/comedouro/temperatura_ambiente
petcare/comedouro/status
petcare/comedouro/alerta
```

---

## 📊 Dashboard

O dashboard será construído no **Node-RED Dashboard** e apresentará os dados recebidos via MQTT.

### Informações exibidas

- Status de movimentação do pet;
- Nível de atividade da coleira;
- Nível de ração no comedouro;
- Peso aproximado da ração disponível;
- Temperatura ambiente próxima ao comedouro;
- Alertas automáticos com data e horário.

### Cards sugeridos

| Card | Informação |
|---|---|
| Status do Pet | Repouso, ativo ou muito ativo |
| Movimento | Valor de movimentação captado pelo MPU6050 |
| Nível de Ração | Percentual aproximado de ração |
| Peso da Ração | Peso em gramas |
| Temperatura Ambiente | Temperatura captada pelo DHT22 |
| Alertas | Ração baixa, temperatura elevada ou inatividade |

---

## 🧪 Simulação no Wokwi

O protótipo será simulado na plataforma **Wokwi** antes da montagem física.

Para facilitar o desenvolvimento e os testes, serão criadas duas simulações separadas:

1. Simulação da **Coleira Smart**
2. Simulação do **Comedouro Inteligente**

Essa separação deixa o projeto mais organizado e representa melhor a arquitetura real, já que cada módulo terá seu próprio ESP32.

---

## 🐾 Simulação 1 — Coleira Smart

### Componentes no Wokwi

```text
ESP32
MPU6050
```

### Ligações sugeridas

```text
MPU6050 VCC  → 3.3V
MPU6050 GND  → GND
MPU6050 SDA  → GPIO 21
MPU6050 SCL  → GPIO 22
```

### Saída esperada no Serial Monitor

```json
{
  "modulo": "coleira",
  "movimento": 1.45,
  "status": "ativo",
  "alerta": false
}
```

---

## 🍽️ Simulação 2 — Comedouro Inteligente

### Componentes no Wokwi

```text
ESP32
HC-SR04
HX711
Servo Motor
DHT22
```

### Ligações sugeridas

```text
DHT22 DATA     → GPIO 15

HC-SR04 TRIG   → GPIO 5
HC-SR04 ECHO   → GPIO 18

HX711 DT       → GPIO 32
HX711 SCK      → GPIO 33

Servo PWM      → GPIO 13
```

### Saída esperada no Serial Monitor

```json
{
  "modulo": "comedouro",
  "nivel_racao": 42,
  "peso_racao": 380,
  "temperatura_ambiente": 27.2,
  "status": "normal",
  "alerta": false
}
```

---

## 🚀 Como Executar

### Pré-requisitos

- Conta no Wokwi;
- Arduino IDE ou PlatformIO;
- Conta gratuita no HiveMQ Cloud;
- Node-RED instalado localmente ou via Docker;
- Bibliotecas necessárias para ESP32.

---

## 1. Clonar o Repositório

```bash
git clone https://github.com/seu-usuario/petcare-hub.git
cd petcare-hub
```

---

## 2. Simular a Coleira no Wokwi

Acesse a pasta:

```bash
cd wokwi/coleira-smart
```

Arquivos esperados:

```text
sketch.ino
diagram.json
libraries.txt
```

Execute a simulação no Wokwi e verifique os dados no Serial Monitor.

---

## 3. Simular o Comedouro no Wokwi

Acesse a pasta:

```bash
cd wokwi/comedouro-inteligente
```

Arquivos esperados:

```text
sketch.ino
diagram.json
libraries.txt
```

Execute a simulação no Wokwi e verifique os dados no Serial Monitor.

---

## 4. Configurar o MQTT

Crie um arquivo `config.h` em cada módulo com as credenciais do Wi-Fi e do broker MQTT.

Exemplo:

```cpp
#define WIFI_SSID "sua_rede_wifi"
#define WIFI_PASSWORD "sua_senha_wifi"

#define MQTT_BROKER "seu_broker.hivemq.cloud"
#define MQTT_PORT 8883
#define MQTT_USER "seu_usuario_mqtt"
#define MQTT_PASSWORD "sua_senha_mqtt"
```

---

## 5. Instalar Bibliotecas

### Bibliotecas da Coleira Smart

```text
WiFi
PubSubClient
ArduinoJson
Wire
Adafruit MPU6050
Adafruit Unified Sensor
```

### Bibliotecas do Comedouro Inteligente

```text
WiFi
PubSubClient
ArduinoJson
DHT sensor library
HX711
ESP32Servo
```

---

## 6. Configurar o Node-RED

Instale o Node-RED:

```bash
npm install -g --unsafe-perm node-red
```

Inicie o Node-RED:

```bash
node-red
```

Acesse no navegador:

```text
http://localhost:1880
```

Depois:

1. Importe o arquivo `dashboard/flows.json`;
2. Configure o nó MQTT com as credenciais do broker;
3. Acesse o dashboard em:

```text
http://localhost:1880/ui
```

---

## 📁 Estrutura do Repositório

```text
petcare-hub/
│
├── wokwi/
│   ├── coleira-smart/
│   │   ├── sketch.ino
│   │   ├── diagram.json
│   │   └── libraries.txt
│   │
│   └── comedouro-inteligente/
│       ├── sketch.ino
│       ├── diagram.json
│       └── libraries.txt
│
├── firmware/
│   ├── coleira-smart/
│   │   ├── coleira-smart.ino
│   │   └── config.h.example
│   │
│   └── comedouro-inteligente/
│       ├── comedouro-inteligente.ino
│       └── config.h.example
│
├── dashboard/
│   ├── flows.json
│   └── screenshots/
│
├── docs/
│   ├── arquitetura.png
│   ├── circuito-coleira.png
│   ├── circuito-comedouro.png
│   └── apresentacao.pdf
│
├── video/
│   └── link.txt
│
└── README.md
```

---

## 📈 Resultados Parciais

### ✅ Implementado no Sprint 1

- [x] Definição do problema e da solução IoT;
- [x] Separação do projeto em dois módulos;
- [x] Definição dos sensores da Coleira Smart;
- [x] Definição dos sensores do Comedouro Inteligente;
- [x] Remoção da medição de temperatura corporal da coleira;
- [x] Definição da arquitetura com ESP32, MQTT e Node-RED;
- [x] Planejamento da simulação no Wokwi.

### 🔄 Em desenvolvimento

- [ ] Simulação da Coleira Smart no Wokwi;
- [ ] Simulação do Comedouro Inteligente no Wokwi;
- [ ] Publicação dos dados via MQTT;
- [ ] Criação do dashboard no Node-RED;
- [ ] Registro dos prints da simulação;
- [ ] Vídeo demonstrativo do funcionamento.

---

## 🔭 Próximos Passos

| Etapa | Objetivo |
|---|---|
| 1 | Criar simulação da Coleira Smart no Wokwi |
| 2 | Criar simulação do Comedouro Inteligente no Wokwi |
| 3 | Testar leitura dos sensores no Serial Monitor |
| 4 | Enviar dados via MQTT |
| 5 | Criar dashboard no Node-RED |
| 6 | Documentar prints e vídeo de demonstração |
| 7 | Planejar montagem física dos protótipos |

---

## 🧠 Possíveis Evoluções Futuras

Como evolução do projeto, podem ser adicionados futuramente:

- Aplicativo mobile para o tutor;
- Histórico de alimentação;
- Banco de dados para armazenar leituras;
- Alertas por Telegram ou e-mail;
- IA para identificar padrões de comportamento;
- Integração com prontuário digital do pet.

---

## 👥 Integrantes da Equipe

| Nome | RM | Turma | GitHub | LinkedIn |
|---|---|---|---|---|
| Alexander Dennis Isidro Mamani | 565554 | 2TDSPG | [alex-isidro](https://github.com/alex-isidro) | [LinkedIn](https://www.linkedin.com/in/alexander-dennis-a3b48824b/) |
| Kelson Zhang | 563748 | 2TDSPG | [KelsonZh0](https://github.com/KelsonZh0) | [LinkedIn](https://www.linkedin.com/in/kelson-zhang-211456323/) |

---

## 📄 Licença

Este projeto foi desenvolvido para fins acadêmicos na **FIAP**.

---

> 🐶 Tecnologia a serviço do bem-estar animal.
