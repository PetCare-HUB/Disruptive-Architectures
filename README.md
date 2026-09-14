# 🐾 PetCare Hub — IoT (Coleira Smart + Comedouro Inteligente)

Protótipo IoT desenvolvido para o **Challenge FIAP 2026 — CLYVO VET**, disciplina **Disruptive Architectures**.

Este repositório reúne os dois dispositivos IoT do PetCare Hub:

- **Coleira Smart** — monitora a movimentação do pet em tempo real usando um **ESP32** com sensor **MPU6050**, classificando o comportamento do animal como `SEDENTARIO`, `MODERADO` ou `ATIVO`.
- **Comedouro Inteligente** — mede nível de ração (HC-SR04) e temperatura ambiente (DHT22), e usa uma **câmera** (ESP32-CAM, abordagem **PetCare Vision**) para estimar o percentual de ração consumida comparando o brilho do prato ao longo do tempo.

Os dois enviam dados via **MQTT** — a coleira já tem um dashboard web dedicado; ver a seção [🧠 PetCare AI — Sprint 3](#-petcare-ai--sprint-3-disruptive-architectures) pra como esses dados (mais os do resto do PetCare Hub) alimentam a camada de IA definida para esta sprint.

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
| ESP32 | Microcontrolador da coleira |
| ESP32-CAM (AI-Thinker) | Microcontrolador + câmera do comedouro |
| MPU6050 | Sensor de aceleração/movimento (coleira) |
| Câmera OV2640 | Estimativa de ração consumida — PetCare Vision (comedouro) |
| HC-SR04 | Nível do reservatório de ração (comedouro) |
| DHT22 | Temperatura ambiente (comedouro) |
| Servo motor | Liberação de ração (comedouro) |
| Wokwi | Simulação dos dois circuitos |
| C++ / Arduino | Programação dos dois firmwares |
| MQTT | Comunicação entre os ESP32 e o dashboard |
| HiveMQ Broker | Broker MQTT público utilizado nos testes |
| HTML | Estrutura do dashboard |
| CSS | Estilização do dashboard |
| JavaScript | Recebimento MQTT e atualização da interface |

---

## 🧩 Arquitetura da Solução

```text
MPU6050                          HC-SR04 + DHT22 + Câmera OV2640
   ↓                                        ↓
ESP32 (coleira)                  ESP32-CAM (comedouro)
   ↓ Wi-Fi                                  ↓ Wi-Fi
        MQTT Broker HiveMQ (tópicos separados)
                    ↓
   Dashboard Web (assina a coleira) · API Java (persiste os dois)
```

Fluxo completo (coleira):

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

Fluxo completo (comedouro):

```text
HC-SR04 lê a distância até a ração → nível do reservatório
Câmera captura o brilho médio do prato → percentual consumido
DHT22 lê a temperatura ambiente
        ↓
ESP32-CAM calcula status (normal / racao_baixa / temperatura_elevada)
        ↓
ESP32-CAM publica os dados via MQTT
        ↓
(ainda sem dashboard dedicado — ver "Resultados Parciais" abaixo)
```

---

## 📡 Comunicação MQTT

O projeto utiliza **MQTT** para enviar os dados da coleira e do comedouro.

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
# Coleira
petcarehub/fiap/coleira01/telemetria
petcarehub/fiap/coleira01/status
petcarehub/fiap/coleira01/alerta

# Comedouro
petcarehub/fiap/comedouro01/telemetria
petcarehub/fiap/comedouro01/status
petcarehub/fiap/comedouro01/alerta
```

### Tópicos principais

```text
petcarehub/fiap/coleira01/telemetria
petcarehub/fiap/comedouro01/telemetria
```

Esses tópicos recebem os dados completos de cada dispositivo em formato JSON. O dashboard web atual assina só o da coleira — ver [Resultados Parciais](#-resultados-parciais-sprint-3).

---

## 📊 Exemplo de JSON Enviado

### Coleira

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
  "statusBruto": "MODERADO",
  "status": "MODERADO",
  "bateria": 98,
  "alertaInatividade": false,
  "wifi": true,
  "mqtt": true,
  "modoDemo": true,
  "uptimeMs": 12000
}
```

### Comedouro

```json
{
  "modulo": "comedouro",
  "nivelRacao": 78,
  "percentualConsumido": 32.5,
  "pesoConsumidoG": 13.0,
  "temperaturaAmbiente": 27.2,
  "status": "normal",
  "alerta": false,
  "cameraDisponivel": true,
  "wifi": true,
  "mqtt": true,
  "modoDemo": true,
  "uptimeMs": 12000
}
```

`percentualConsumido` e `pesoConsumidoG` vêm da câmera (PetCare Vision — ver [🧠 PetCare AI — Sprint 3](#-petcare-ai--sprint-3-disruptive-architectures)): o ESP32-CAM compara o brilho médio do prato agora contra uma referência de "prato cheio" e converte esse percentual em gramas usando uma constante de calibração do pote (pote cheio = 40g = 100%).

---

## 🐕 Classificação de Atividade

A coleira não calcula velocidade real em km/h, pois isso exigiria GPS.

Em vez disso, o projeto calcula a **velocidade de movimento**, baseada na variação da aceleração medida pelo MPU6050.

> Os três valores (`SEDENTARIO`, `MODERADO`, `ATIVO`) não são uma escolha livre — são exatamente
> os aceitos pelo `CHECK` de `LEITURA_COLEIRA.status_atividade` no banco Oracle. O ESP32 já
> publica essas strings prontas via MQTT, sem precisar de nenhuma tradução no Java antes de
> persistir. `SEDENTARIO` também é o valor que já dispara automaticamente um alerta
> `ATIVIDADE_BAIXA` do lado do banco (`PRC_INS_LEITURA_COLEIRA`).

### Regras utilizadas

| Situação | Classificação |
|---|---|
| Pouco ou nenhum movimento | `SEDENTARIO` |
| Movimento moderado, semelhante a passeio | `MODERADO` |
| Movimento forte por tempo contínuo | `ATIVO` |
| Pequeno pico isolado | Não muda imediatamente para `ATIVO` |

O status `ATIVO` só é confirmado quando o movimento intenso permanece por vários segundos. Isso evita falsos positivos causados por pequenas mexidas na coleira.

---

## 🍽️ Cálculo de Consumo do Comedouro (PetCare Vision)

O comedouro **não usa mais célula de carga (HX711)** para medir quanto o pet comeu — essa leitura foi substituída por uma **câmera** (ESP32-CAM), abordagem chamada de **PetCare Vision** (visão computacional clássica, sem LLM — ver [🧠 PetCare AI — Sprint 3](#-petcare-ai--sprint-3-disruptive-architectures) pra entender a diferença entre as duas capacidades de IA do produto).

### Como funciona

1. Ao ligar (ou depois de reabastecer), o ESP32-CAM tira uma foto do prato e guarda o **brilho médio** dela como referência de "prato cheio".
2. Periodicamente, tira uma nova foto e compara o brilho atual contra essa referência.
3. Quanto maior a diferença de brilho, maior o `percentualConsumido`.
4. O peso em gramas (`pesoConsumidoG`) é calculado a partir desse percentual e de uma constante de calibração do pote (pote cheio = 40g = 100%) — não vem mais de sensor nenhum.

### Limitação conhecida no Wokwi

A câmera simulada do Wokwi sempre devolve o mesmo quadro de teste (não muda com o "conteúdo do prato"), então o percentual calculado a partir da imagem real fica travado perto de 0%. Por isso o firmware tem um `MODO_DEMO` (igual à coleira) que sobrepõe uma curva de consumo simulada só para efeito de demonstração — em hardware físico, com `MODO_DEMO = false`, o valor vem inteiramente do brilho capturado pela câmera de verdade.

---

## 🧪 Modo Demo e Modo Físico

Os dois firmwares (coleira e comedouro) têm o mesmo padrão de dois modos.

### Modo Demo

Usado no Wokwi para demonstrar automaticamente os estados do dispositivo, já que os sensores simulados (MPU6050 parado, câmera com quadro fixo) sozinhos não gerariam variação nenhuma pra mostrar.

```cpp
const bool MODO_DEMO = true;
```

Na coleira, alterna entre `SEDENTARIO`, `MODERADO` e `ATIVO`. No comedouro, sobrepõe uma curva de consumo ao percentual calculado pela câmera.

### Modo Físico

Usado no hardware real (ESP32 com MPU6050 físico, ou ESP32-CAM físico).

```cpp
const bool MODO_DEMO = false;
```

Nesse modo, o status muda apenas conforme o que o sensor (ou a câmera) realmente detectar.

---

## 🛠️ Estrutura do Projeto

```text
Disruptive-Architectures/
├── coleira-smart/
│   ├── coleira-smart.ino
│   ├── diagram.json
│   ├── libraries.txt
│   └── wokwi.toml
│
├── comedouro-inteligente/
│   ├── comedouro-inteligente.ino
│   ├── diagram.json
│   ├── libraries.txt
│   └── wokwi.toml
│
├── dashboard/
│   ├── index.html
│   ├── css/
│   │   └── styles.css
│   └── js/
│       └── app.js
│
├── entrega/
│   ├── link-github.txt
│   └── link-video-youtube.txt
│
├── PETCARE_AI_ 1.pdf            ← especificação da PetCare AI (Sprint 3) — ver seção abaixo
└── README.md
```

---

## ▶️ Como Executar no Wokwi

### Coleira Smart

Abra a pasta `coleira-smart` no Wokwi. Ela deve conter:

```text
coleira-smart.ino
diagram.json
libraries.txt
wokwi.toml
```

O `libraries.txt` precisa conter `PubSubClient` (comunicação MQTT).

Clique em **Start Simulation**. No Serial Monitor, devem aparecer mensagens parecidas com:

```json
{
  "modulo": "coleira_smart",
  "status": "MODERADO",
  "velocidadeMovimentoFiltrada": 1.05,
  "wifi": true,
  "mqtt": true
}
```

### Comedouro Inteligente

Abra a pasta `comedouro-inteligente` no Wokwi. Ela deve conter:

```text
comedouro-inteligente.ino
diagram.json
libraries.txt
wokwi.toml
```

O `libraries.txt` precisa conter `DHT sensor library for ESPx` e `PubSubClient`.

> **Se o `wokwi.toml` reclamar de firmware/elf ausente**: os binários pré-compilados
> (`esp32.esp32.esp32cam/comedouro-inteligente.ino.{bin,elf}`) precisam ser gerados uma vez
> via Arduino IDE/`arduino-cli` (placa **AI Thinker ESP32-CAM**) e commitados nessa pasta —
> ver [Como Executar no ESP32-CAM Físico](#-como-executar-no-esp32-cam-físico-comedouro).
> Sem isso, abra o `.ino` direto em [wokwi.com](https://wokwi.com) e deixe o compilador online
> do próprio Wokwi compilar (mais lento, mas não depende de binário commitado).

Clique em **Start Simulation**. No Serial Monitor, devem aparecer mensagens parecidas com:

```json
{
  "modulo": "comedouro",
  "status": "normal",
  "percentualConsumido": 12.5,
  "wifi": true,
  "mqtt": true
}
```

---

## 🖥️ Como Abrir o Dashboard

Abra o arquivo:

```text
dashboard/index.html
```

O dashboard irá se conectar ao broker MQTT e assinar o tópico:

```text
petcarehub/fiap/coleira01/telemetria
```

Quando o ESP32 publicar os dados, o dashboard será atualizado automaticamente.

---

## 🔧 Como Executar no ESP32 Físico (Coleira)

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

## 🔧 Como Executar no ESP32-CAM Físico (Comedouro)

### 1. Instalar bibliotecas

Na Arduino IDE, instale `PubSubClient` e `DHT sensor library for ESPx` (Sketch → Include Library → Manage Libraries). O suporte à câmera (`esp_camera.h`) já vem embutido no core ESP32 — não precisa instalar nada à parte.

### 2. Selecionar a placa certa

```text
Tools → Board → AI Thinker ESP32-CAM
```

A pinagem da câmera no código (`PWDN_GPIO_NUM`, `XCLK_GPIO_NUM`, etc.) é fixa e específica desse módulo — só funciona com essa placa selecionada.

### 3. Configurar Wi-Fi e desativar o modo demo

Mesmos passos da coleira (seções 2 e 3 acima), no arquivo `comedouro-inteligente.ino`.

### 4. Ligações dos sensores

| Componente | ESP32-CAM |
|---|---|
| HC-SR04 TRIG | GPIO 13 |
| HC-SR04 ECHO | GPIO 12 |
| DHT22 SDA | GPIO 14 |
| Servo PWM | GPIO 15 |

A câmera OV2640 já vem soldada ao módulo AI-Thinker — não tem fiação extra pra fazer.

### 5. Gravar o firmware

O ESP32-CAM não tem porta USB própria — é preciso um adaptador FTDI (USB-Serial), ligando GPIO0 ao GND **antes** de ligar a placa (modo de gravação), gravar, depois desligar essa ligação e resetar pra rodar o programa normalmente. Na Arduino IDE, **Upload** como de costume depois de entrar em modo de gravação.

### 6. Gerar os binários pré-compilados para o Wokwi

Depois de compilar com sucesso pela Arduino IDE (ou `arduino-cli compile --fqbn esp32:esp32:esp32cam`), copie a pasta de build gerada (`esp32.esp32.esp32cam/`, com os arquivos `.bin`/`.elf`) para dentro de `comedouro-inteligente/`, no mesmo padrão já usado pela coleira. Isso é necessário porque o `build/` local não é versionado (`.gitignore`), e o `wokwi.toml` do comedouro depende desses arquivos existirem no repositório para rodar sem precisar recompilar.

---

## 📺 Demonstração em Vídeo

O vídeo de apresentação deve demonstrar:

1. O problema que o projeto resolve;
2. A simulação da coleira no Wokwi;
3. O envio dos dados via MQTT;
4. O dashboard recebendo os dados em tempo real;
5. A mudança entre `SEDENTARIO`, `MODERADO` e `ATIVO`;
6. O alerta de inatividade;
7. A explicação do modo demo e do modo físico.

Link do vídeo:

```text
https://youtu.be/ss0ONo7hGNA
```

---

## ⚠️ Observações Técnicas

- O projeto não mede temperatura corporal do pet.
- A bateria exibida no dashboard é simulada.
- O status `ATIVO` exige movimento intenso por tempo contínuo.
- O dashboard precisa usar o mesmo tópico MQTT configurado no ESP32.
- Em redes Wi-Fi públicas ou corporativas, a porta MQTT pode ser bloqueada.
- Para medição de velocidade real em km/h seria necessário adicionar GPS ao protótipo.
- O percentual de ração consumida do comedouro (PetCare Vision) usa brilho médio da imagem, não reconhecimento de objeto — é uma aproximação, sensível a mudança de iluminação no ambiente real.
- No Wokwi, a câmera do comedouro sempre devolve o mesmo quadro de teste — o percentual mostrado na simulação depende do `MODO_DEMO`, não da imagem em si.

---

## 🧠 PetCare AI — Sprint 3 (Disruptive Architectures)

Especificação completa: [`PETCARE_AI_ 1.pdf`](PETCARE_AI_%201.pdf). Esta seção é um resumo — **é um documento de design, não uma implementação**; nenhum código de LLM/RAG está neste (ou em nenhum outro) repositório do PetCare Hub ainda.

### O problema

O PetCare Hub já coleta continuamente dados reais da rotina do pet (movimento pela coleira, alimentação pelo comedouro, condições ambientais) e já calcula um Score de Saúde (0-100) e alertas. O problema é que dado bruto e um número de score, sozinhos, não dizem *por que* algo mudou nem *o que fazer* a respeito. A PetCare AI existe pra preencher essa lacuna: interpreta o contexto do pet e traduz isso numa orientação simples e acionável — **nunca diagnostica, nunca substitui o veterinário**.

### Arquitetura: LLM + RAG

1. **IoT → Java → Oracle** (este repositório): coleira e comedouro enviam leituras via MQTT; o Java persiste.
2. **Cálculo determinístico** (sem IA): o Java calcula variação percentual dos indicadores e o nível de urgência, a partir das faixas do Score de Saúde já existentes (80-100 baixa, 50-79 média, 0-49 alta).
3. **RAG (Retrieval-Augmented Generation)**: o Java consulta uma base de conhecimento veterinário, filtrando por espécie/idade/porte do pet.
4. **LLM**: recebe os dados já calculados (incluindo a urgência) e as diretrizes recuperadas, e tem uma responsabilidade deliberadamente restrita — explicar em linguagem humana por que aquele padrão é relevante, e escolher uma ação sugerida dentro de uma lista fechada. A LLM nunca decide nem pode contradizer a urgência que o sistema já calculou.
5. **Validação**: se algum campo vier fora do formato esperado, o Java corrige ou usa um valor seguro padrão antes de repassar ao Mobile.

### Dados utilizados

Cadastro do pet, leituras da coleira e do comedouro, Score de Saúde (atual e histórico), alertas, histórico recente, consultas e eventos preventivos, e uma base de conhecimento veterinário. A IA nunca acessa o banco diretamente — todo o contexto é montado e controlado pelo Java.

### Personalização

Em três camadas, todas na etapa determinística (antes da LLM entrar em cena): perfil individual do pet (espécie/idade/porte filtram as diretrizes do RAG), baseline próprio do pet (não uma média populacional) e contexto clínico do próprio pet (evita repetir sugestão já tomada, evita interpretar como anomalia um efeito esperado de um cuidado recente).

### PetCare AI vs. PetCare Vision

São duas capacidades de IA distintas no produto:

- **PetCare AI** — a camada de LLM + RAG acima: interpreta contexto e explica em linguagem humana.
- **PetCare Vision** — visão computacional clássica (sem LLM), usada só no comedouro: compara o brilho do prato antes/depois da refeição pra estimar o percentual de ração consumida. Diferente da PetCare AI, essa parte **já está implementada** neste repositório — ver [Cálculo de Consumo do Comedouro](#-cálculo-de-consumo-do-comedouro-petcare-vision) acima.

---

## 👥 Integrantes da Equipe

| Nome | RM | Turma | GitHub | LinkedIn |
|---|---|---|---|---|
| Alexander Dennis Isidro Mamani | 565554 | 2TDSPG | [alex-isidro](https://github.com/alex-isidro) | [LinkedIn](https://www.linkedin.com/in/alexander-dennis-a3b48824b/) |
| Kelson Zhang | 563748 | 2TDSPG | [KelsonZh0](https://github.com/KelsonZh0) | [LinkedIn](https://www.linkedin.com/in/kelson-zhang-211456323/) |

---
