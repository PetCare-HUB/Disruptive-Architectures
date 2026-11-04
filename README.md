# 🐾 PetCare Hub — IoT (Coleira Smart + Comedouro Inteligente)

Protótipo IoT desenvolvido para o **Challenge FIAP 2026 — CLYVO VET**, disciplina **Disruptive Architectures**.

Este repositório reúne os dois dispositivos IoT do PetCare Hub:

- **Coleira Smart** — monitora a movimentação do pet em tempo real usando um **ESP32** com sensor **MPU6050**, classificando o comportamento do animal como `SEDENTARIO`, `MODERADO` ou `ATIVO`.
- **Comedouro Inteligente** — em migração para ESP32-S3 N16R8 + OV5640, DHT11, motor DC com TB6612 e microinterruptor. O teste Wokwi valida a lógica de uma porção e as falhas com componentes substitutos. [Guia atual e limites da simulação](comedouro-inteligente/LEIA-ME.md).

Os dois têm páginas no **dashboard web** para acompanhar dados via **MQTT**. O comedouro também tem uma **simulação interativa no navegador**, com dispenser, pet comendo e câmera virtual, que funciona sem equipamento e sem internet. Ver a seção [🧠 PetCare AI](#-petcare-ai--a-camada-de-ia-do-produto) pra como os dados dos dispositivos (mais os do resto do PetCare Hub) alimentam a camada de IA do produto, implementada na Sprint 4.

> **A camada de IA (PetCare AI) foi implementada em Java**, dentro da API do PetCare Hub (Spring
> Boot + Spring AI + Google Gemini, com RAG):
> **[github.com/PetCare-HUB/java](https://github.com/PetCare-HUB/java)**. Este repositório guarda
> os dispositivos, o dashboard e o simulador que gera o histórico que a IA lê.

| Repositório | O que tem |
|---|---|
| **Este** (Disruptive-Architectures) | Firmware da coleira e do comedouro, dashboard MQTT, simulador de leituras |
| [java](https://github.com/PetCare-HUB/java) | API Java: persistência no Oracle, Score de Saúde, alertas e **a PetCare AI** |
| [Mobile](https://github.com/PetCare-HUB/Mobile) | App do tutor, com a aba **PetCare AI** |

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
| MPU6050 | Sensor de aceleração/movimento (coleira) |
| ESP32-S3 CAM OEM N16R8 | Microcontrolador do comedouro (16 MB flash / 8 MB PSRAM) |
| Câmera OV5640 | Prevista no comedouro; pinagem da câmera OEM ainda por confirmar na bancada |
| DHT11 | Temperatura e umidade (comedouro) |
| TB6612 + motor DC com redutor | Acionamento da porção (comedouro) |
| Microinterruptor de fim de curso | Confirma o ciclo mecânico de uma porção (comedouro) |
| Wokwi | Simulação dos dois circuitos |
| C++ / Arduino | Programação dos dois firmwares |
| MQTT | Comunicação entre os ESP32 e o dashboard |
| HiveMQ Broker | Broker MQTT público utilizado nos testes |
| HTML | Estrutura do dashboard |
| CSS | Estilização do dashboard |
| JavaScript | Recebimento MQTT e atualização da interface |
| Python | Simulador que envia leituras à API Java (`simulador-api/`) |
| Java + Spring Boot | API que persiste as leituras no Oracle ([repositório Java](https://github.com/PetCare-HUB/java)) |
| Spring AI + Google Gemini | PetCare AI: `gemini-2.5-flash` para as respostas e `gemini-embedding-001` para o RAG ([repositório Java](https://github.com/PetCare-HUB/java)) |

---

## 🧩 Arquitetura da Solução

```text
MPU6050                          DHT11 + TB6612/motor + microinterruptor
   ↓                                        ↓
ESP32 (coleira)                  ESP32-S3 CAM N16R8 (comedouro)
   ↓ Wi-Fi                                  ↓ Wi-Fi
        MQTT Broker HiveMQ (tópicos separados)
                    ↓
   Dashboard Web (páginas da coleira e do comedouro)

Simulador (simulador-api/) ── HTTP ──> API Java ──> Oracle ──> PetCare AI ──> App mobile
```

Hoje o MQTT termina no dashboard: a API Java não assina o broker. O histórico que chega ao Oracle
— e que a PetCare AI lê — entra pelos endpoints HTTP `POST /leituras/coleira`, `/comedouro` e
`/ambiente`, alimentados pelo [simulador](simulador-api/README.md) com o mesmo formato de dado
que os dispositivos publicam.

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
Pedido de porção chega (botão de teste ou HTTP)
        ↓
Motor gira; o microinterruptor confirma o ciclo → porção contada
DHT lê temperatura e umidade do ambiente
        ↓
ESP32-S3 calcula status (normal / falha_motor / falha_sensor / temperatura_elevada)
        ↓
ESP32-S3 publica os dados via MQTT
        ↓
Dashboard do comedouro exibe as leituras no modo MQTT
```

Na simulação local, `dashboard/comedouro.html` gera os dados no próprio navegador e atualiza a animação, a visão virtual do prato e o gráfico. Esses dados não são enviados ao broker nem à API.

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

Esses tópicos recebem os dados completos de cada dispositivo em formato JSON. A página da coleira assina seu tópico; a página do comedouro assina o tópico correspondente quando o modo **Wokwi / dispositivo via MQTT** é selecionado.

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
  "nivelRacao": null,
  "percentualConsumido": null,
  "pesoConsumidoG": null,
  "temperaturaAmbiente": 27.2,
  "umidadeAmbiente": 58.0,
  "porcoesDispensadas": 3,
  "estadoMotor": "parado",
  "falhaMotor": "nenhuma",
  "status": "normal",
  "alerta": false,
  "cameraDisponivel": false,
  "wifi": true,
  "mqtt": true,
  "modoDemo": true,
  "origem": "wokwi",
  "consumoDisponivel": false,
  "nivelDisponivel": false,
  "sensorTemperatura": "DHT22_substituto_virtual",
  "uptimeMs": 12000
}
```

Os três campos em `null` são deliberados: o firmware S3 **não mede nível nem ingestão**, e
`consumoDisponivel` e `nivelDisponivel` dizem isso a quem consome, em vez de deixar adivinhar.
Declarar a ausência é melhor do que publicar um número sem como sustentar — a geração anterior
estimava consumo pelo brilho da imagem, e o motivo de ter saído está no
[histórico do AI Thinker](comedouro-inteligente/HISTORICO-AI-THINKER.md).

O que o comedouro informa de fato hoje: temperatura e umidade pelo DHT, quantas porções foram
dispensadas (`porcoesDispensadas`), em que ponto do ciclo o motor está (`estadoMotor`) e se
alguma porção falhou (`falhaMotor`). `origem` separa o que veio do Wokwi do que veio de
hardware, e `sensorTemperatura` registra que no simulador o DHT11 é substituído por um DHT22
virtual.

---

## 🐕 Classificação de Atividade

A coleira não calcula velocidade real em km/h, pois isso exigiria GPS.

Em vez disso, o projeto calcula a **velocidade de movimento**, baseada na variação da aceleração medida pelo MPU6050.

> Os três valores (`SEDENTARIO`, `MODERADO`, `ATIVO`) não são uma escolha livre — são exatamente
> os aceitos pelo `CHECK` de `LEITURA_COLEIRA.status_atividade` no banco Oracle. O ESP32 já
> publica essas strings prontas via MQTT, no mesmo formato que o `POST /leituras/coleira` da API
> Java grava sem tradução. `SEDENTARIO` também é o valor que já dispara automaticamente um alerta
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

## 🍽️ A porção do comedouro

Uma porção é **um ciclo mecânico completo**, não um tempo de motor ligado nem um ângulo de
servo. O microinterruptor de fim de curso confirma o ciclo: o `controle_porcao.h` solta a
chave, espera ela fechar de novo e então freia. Só aí a porção é contada.

A máquina de estados é pequena de propósito, e é o que o teste no Wokwi exercita:

| Estado | O que está acontecendo |
|---|---|
| `parado` | Em repouso, pronto para receber um pedido |
| `liberando_chave` | Motor girando até o microinterruptor abrir |
| `dispensando` | Girando até o microinterruptor fechar de novo — o ciclo da porção |
| `freando` | Freio aplicado por `TEMPO_FREIO_MS` antes de voltar a `parado` |
| `falha_motor` | O ciclo não completou, e o comedouro precisa de rearme |

Duas falhas são tratadas: `tempo_esgotado`, quando o ciclo passa de `TEMPO_MAXIMO_PORCAO_MS` —
o caso do entupimento — e `cancelado`. Enquanto o estado for `falha_motor`, novas porções são
recusadas até alguém rearmar. A alternativa, tentar de novo sozinho, transformaria um
entupimento em motor queimado.

O pedido de porção é uma **caixa de correio de um item**: pedir duas vezes seguidas não
enfileira duas porções. Isso evita que uma falha de rede vire pet alimentado em dobro.

> **O comedouro não mede quanto o pet comeu.** A geração anterior estimava isso pelo brilho da
> imagem do prato, abordagem chamada *PetCare Vision*. Aquela documentação, e o motivo de ter
> saído, estão no [histórico do AI Thinker](comedouro-inteligente/HISTORICO-AI-THINKER.md).

---

## 🧪 Modo Demo e Modo Físico

A descrição de `MODO_DEMO` abaixo é histórica para o comedouro. O comedouro S3 atual usa `PETCARE_SIMULATION` em `config.h`; consulte o guia específico. A coleira mantém seus modos.

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
│   ├── config.h                  ← pinagem, Wi-Fi e tempos do ciclo
│   ├── controle_porcao.h         ← maquina de estados da porcao
│   ├── tests/                    ← testes do controle_porcao, rodam sem hardware
│   ├── build/                    ← binarios dos perfis wokwi e fisico
│   ├── LEIA-ME.md                ← guia atual do ESP32-S3
│   ├── HISTORICO-AI-THINKER.md   ← geracao anterior, so para referencia
│   ├── diagram.json
│   ├── libraries.txt
│   └── wokwi.toml
│
├── dashboard/
│   ├── index.html
│   ├── comedouro.html
│   ├── README.md
│   ├── css/
│   │   └── styles.css
│   └── js/
│       ├── app.js
│       ├── comedouro.js
│       └── simulacao.js
│
├── simulador-api/
│   ├── simulator.py              ← gera historico de leituras pela API Java
│   └── README.md
│
├── entrega/
│   ├── link-github.txt
│   └── link-video-youtube.txt
│
├── PETCARE_AI_ 1.pdf            ← desenho da PetCare AI; a implementação está na seção abaixo
└── README.md
```

---

## 🤖 Simulador virtual integrado à API Java

Para gerar histórico sem depender dos dispositivos físicos, use o [simulador da API](simulador-api/README.md). Ele envia leituras sintéticas da coleira, do comedouro e do ambiente pelos endpoints Java; o backend persiste no Oracle e calcula score/alertas.

**ID do pet:** abra `simulador-api/simulator.py` e altere `PET_ID` perto do início, ou configure `PETCARE_PET_ID` no terminal. Use um ID existente e acessível ao tutor do JWT. O simulador aponta para `localhost:8080` por padrão e exige que endereço e token sejam configurados no terminal.

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

Use o [roteiro atual de teste Wokwi](comedouro-inteligente/LEIA-ME.md).
O `wokwi.toml` carrega o novo binário S3 em `comedouro-inteligente/build/wokwi/`.
DHT22 representa apenas o sensor de ambiente no simulador; na placa física o perfil usa DHT11.
LEDs representam sinais lógicos do TB6612 e um botão representa o microinterruptor.
A câmera, corrente, torque e quantidade de ração não são validados no Wokwi.

### Comedouro: simulação sem equipamento

Abra [dashboard/comedouro.html](dashboard/comedouro.html) no navegador. O modo **Simulação no navegador** já vem selecionado e funciona sem internet.

1. Escolha uma porção e clique em **Dispensar ração** para ver a comida cair no prato.
2. Clique em **Pet comer** para acompanhar a refeição na animação, na câmera virtual e no gráfico. O mesmo botão pausa a refeição.
3. Use **Reabastecer reservatório**, altere a temperatura para testar alertas ou clique em **Reiniciar** para começar de novo.

O reservatório tem capacidade simulada de 200 g e o prato, de 40 g. A porção é limitada à ração disponível e ao espaço no prato. A câmera virtual é uma ilustração da simulação; não captura vídeo nem reconhece imagens. O histórico é mantido apenas enquanto a página está aberta.

Para acompanhar o firmware, selecione **Wokwi / dispositivo via MQTT** e inicie o projeto do comedouro no Wokwi. Esse modo precisa de internet e acompanha a telemetria; os comandos locais `d`, `p` e `r` no Serial Monitor controlam o teste do motor; o dashboard recebe telemetria.

### Coleira: telemetria MQTT

Abra o arquivo:

```text
dashboard/index.html
```

O dashboard irá se conectar ao broker MQTT e assinar o tópico:

```text
petcarehub/fiap/coleira01/telemetria
```

Quando o ESP32 publicar os dados, o dashboard será atualizado automaticamente.

Use os links no topo das páginas para alternar entre **Coleira Smart** e **Comedouro Inteligente**. Mais detalhes em [dashboard/README.md](dashboard/README.md).

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

## 🔧 Como Executar no ESP32-S3 Físico (Comedouro)

O firmware atual possui perfis Wokwi e físico, com a mesma lógica do motor.
Consulte [comedouro-inteligente/LEIA-ME.md](comedouro-inteligente/LEIA-ME.md) para compilar, testar e identificar as ligações propostas.
A câmera OEM e a alimentação do motor ainda precisam de confirmação na bancada.
Os antigos binários `esp32.esp32.esp32cam/` são legados AI Thinker e não devem ser gravados no ESP32-S3.

---

## 📺 Demonstração em Vídeo

### Sprint 4 — pitch de ~5 minutos

A nota da Sprint 4 concentra-se na camada de IA, então o roteiro muda de peso:

1. O problema que o projeto resolve, em uma frase;
2. A simulação da coleira no Wokwi e o envio via MQTT;
3. O dashboard recebendo os dados em tempo real, com a troca entre `SEDENTARIO`, `MODERADO` e `ATIVO`;
4. O [simulador da API](simulador-api/README.md) gerando histórico — é o dado que a IA lê;
5. **A PetCare AI respondendo sobre um pet real**, pela aba do app ou pelo endpoint, mostrando que a resposta cita o contexto daquele pet e não uma generalidade;
6. **A base de conhecimento**: `GET /admin/ia/conhecimento/status` e uma busca, para mostrar que o RAG recupera trecho de verdade;
7. Benefícios para o tutor e para a clínica.

Link do vídeo (Sprint 4):

```text
https://youtu.be/4CAk0jqLUBs
```

### Sprint 3 — demonstração do IoT

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
- O comedouro não mede nível de ração nem quanto o pet comeu: publica `null` nesses campos e sinaliza a ausência em `nivelDisponivel` e `consumoDisponivel`.
- A porção é contada por ciclo mecânico, não por peso — o microinterruptor confirma que o ciclo completou, não quantos gramas caíram.
- A câmera OV5640 não é emulada no Wokwi (`cameraDisponivel=false`), e a pinagem da câmera OEM ainda depende de confirmação na bancada.
- O teste no Wokwi não valida corrente, torque, entupimento real nem peso por porção. Ver os limites em [LEIA-ME.md](comedouro-inteligente/LEIA-ME.md).

---

## 🧠 PetCare AI — a camada de IA do produto

Desenhada na Sprint 3 e **implementada na Sprint 4**. O desenho original está em
[`PETCARE_AI_ 1.pdf`](PETCARE_AI_%201.pdf); esta seção descreve o que foi construído e onde.

O código vive no [repositório Java](https://github.com/PetCare-HUB/java), porque é lá que o
contexto do pet já está montado — leituras dos dispositivos, Score de Saúde, alertas, consultas
e eventos preventivos. Duplicar isso num serviço separado só para hospedar a IA criaria duas
versões da mesma verdade. Os dispositivos deste repositório são a origem dos dados que ela lê.

### O problema

O PetCare Hub já coleta continuamente dados reais da rotina do pet (movimento pela coleira, alimentação pelo comedouro, condições ambientais) e já calcula um Score de Saúde (0-100) e alertas. O problema é que dado bruto e um número de score, sozinhos, não dizem *por que* algo mudou nem *o que fazer* a respeito. A PetCare AI existe pra preencher essa lacuna: interpreta o contexto do pet e traduz isso numa orientação simples e acionável — **nunca diagnostica, nunca substitui o veterinário**.

### Arquitetura: LLM + RAG

1. **IoT → Java → Oracle** (este repositório): coleira e comedouro publicam via MQTT para o dashboard; o histórico chega à API Java pelos endpoints HTTP de leitura, alimentados pelo simulador, e o Java persiste no Oracle.
2. **Cálculo determinístico** (sem IA): o Java calcula variação percentual dos indicadores e o nível de urgência, a partir das faixas do Score de Saúde já existentes (80-100 baixa, 50-79 média, 0-49 alta).
3. **RAG (Retrieval-Augmented Generation)**: o Java consulta uma base de conhecimento veterinário, filtrando por espécie/idade/porte do pet.
4. **LLM**: recebe os dados já calculados (incluindo a urgência) e as diretrizes recuperadas, e tem uma responsabilidade deliberadamente restrita — explicar em linguagem humana por que aquele padrão é relevante, e escolher uma ação sugerida dentro de uma lista fechada. A LLM nunca decide nem pode contradizer a urgência que o sistema já calculou.
5. **Validação**: se algum campo vier fora do formato esperado, o Java corrige ou usa um valor seguro padrão antes de repassar ao Mobile.

### Onde está, e como conferir

| Peça | Onde |
|---|---|
| Endpoints do assistente | `POST /pets/{petId}/ia/analise`, `POST /pets/{petId}/ia/perguntar`, `POST /pets/{petId}/ia/alertas/{alertaId}/explicar`, `GET /pets/{petId}/ia/analises` |
| Administração da base | `POST /admin/ia/reindexar`, `GET /admin/ia/conhecimento/status`, `GET /admin/ia/conhecimento` |
| Chat e embeddings | Spring AI 1.1.8 + Google Gemini — `gemini-2.5-flash` para chat, `gemini-embedding-001` (3072 dimensões) para os vetores |
| Base vetorial | `ConhecimentoVectorStore`, implementação própria de `VectorStore` |
| Persistência | Tabelas `DOCUMENTO_CONHECIMENTO` e `ANALISE_IA`, criadas pela migração Flyway `V14` |
| No app | A aba **PetCare AI** do [app mobile](https://github.com/PetCare-HUB/Mobile) consome `POST /pets/{petId}/ia/perguntar` |

**Por que uma `VectorStore` própria:** o Oracle 19c não tem tipo `VECTOR`, então o
`spring-ai-starter-vector-store-oracle` não serve. O embedding de cada trecho é gravado como
JSON num CLOB e a similaridade por cosseno é calculada em Java. Cada trecho guarda também o
nome do modelo e o número de dimensões que o geraram — se o modelo de embedding mudar, os
vetores antigos ficam incomparáveis, e é por isso que existe o `POST /admin/ia/reindexar`.

### Dados utilizados

Cadastro do pet, leituras da coleira e do comedouro, Score de Saúde (atual e histórico), alertas, histórico recente, consultas e eventos preventivos, e uma base de conhecimento veterinário. A IA nunca acessa o banco diretamente — todo o contexto é montado e controlado pelo Java.

### Personalização

Em três camadas, todas na etapa determinística (antes da LLM entrar em cena): perfil individual do pet (espécie/idade/porte filtram as diretrizes do RAG), baseline próprio do pet (não uma média populacional) e contexto clínico do próprio pet (evita repetir sugestão já tomada, evita interpretar como anomalia um efeito esperado de um cuidado recente).

### PetCare AI vs. PetCare Vision

São duas capacidades de IA distintas no produto:

- **PetCare AI** — a camada de LLM + RAG acima: interpreta contexto e explica em linguagem humana.
- **PetCare Vision** — visão computacional clássica (sem LLM) que estimava o consumo pelo brilho da imagem do prato. **Não está no firmware atual**: ficou na geração ESP32-CAM e saiu na migração para o S3, pelos motivos registrados no [histórico do AI Thinker](comedouro-inteligente/HISTORICO-AI-THINKER.md). Aparece aqui porque está no vídeo da Sprint 3 e costuma ser confundida com a PetCare AI, que é outra coisa e continua ativa.

---

## 👥 Integrantes da Equipe

| Nome | RM | Turma | GitHub | LinkedIn |
|---|---|---|---|---|
| Alexander Dennis Isidro Mamani | 565554 | 2TDSPG | [alex-isidro](https://github.com/alex-isidro) | [LinkedIn](https://www.linkedin.com/in/alexander-dennis-a3b48824b/) |
| Kelson Zhang | 563748 | 2TDSPG | [KelsonZh0](https://github.com/KelsonZh0) | [LinkedIn](https://www.linkedin.com/in/kelson-zhang-211456323/) |

---
