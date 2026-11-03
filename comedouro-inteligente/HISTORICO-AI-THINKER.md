# Comedouro — geração anterior (ESP32-CAM AI Thinker)

Este arquivo guarda a documentação do comedouro **antes da migração para o ESP32-S3**, feita em
18/09/2026. Nada aqui descreve o equipamento atual: o guia válido é
[LEIA-ME.md](LEIA-ME.md).

Está versionado porque a abordagem de medir consumo por brilho (chamada **PetCare Vision**) foi
apresentada na Sprint 3 e aparece no vídeo daquela entrega. Apagá-la deixaria o vídeo antigo sem
contexto; mantê-la no README principal fazia o leitor achar que ainda é o circuito montado.

---

## Hardware daquela geração

| Componente | Uso |
|---|---|
| ESP32-CAM (AI Thinker) | Microcontrolador + câmera do comedouro |
| Câmera OV2640 | Estimativa de ração consumida |
| HC-SR04 | Nível do reservatório de ração |
| DHT22 | Temperatura ambiente |
| Servo motor | Liberação de ração |

No ESP32-S3 isso virou OV5640, DHT11, motor DC com driver TB6612 e microinterruptor de fim de
curso. **Não há HC-SR04**, e a porção deixou de ser um ângulo de servo para ser um ciclo
mecânico completo, confirmado pelo microinterruptor.

---

## PetCare Vision — consumo por brilho

Visão computacional clássica, sem LLM. Não confundir com a **PetCare AI**, que é a camada de
LLM + RAG do produto e continua ativa.

1. Ao ligar, ou depois de reabastecer, o ESP32-CAM fotografava o prato e guardava o **brilho
   médio** como referência de "prato cheio".
2. Periodicamente tirava nova foto e comparava o brilho atual contra essa referência.
3. Quanto maior a diferença, maior o `percentualConsumido`.
4. O peso em gramas vinha desse percentual e de uma constante de calibração do pote
   (pote cheio = 40 g = 100%), não de sensor.

**Limitação no Wokwi:** a câmera simulada devolvia sempre o mesmo quadro, então o percentual
calculado a partir da imagem ficava travado perto de 0%. O firmware tinha um `MODO_DEMO` que
sobrepunha uma curva de consumo simulada só para a demonstração.

**Por que saiu:** a medição por brilho depende de iluminação estável e de o prato estar sempre
no mesmo enquadramento — duas condições que a bancada não garantiu. O firmware S3 publica
`percentualConsumido` e `nivelRacao` como `null`, e sinaliza isso em `consumoDisponivel` e
`nivelDisponivel`, em vez de informar um número que não tem como sustentar.

---

## Telemetria daquela geração

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

Os binários antigos continuam em `esp32.esp32.esp32cam/`. São legado AI Thinker e **não devem
ser gravados no ESP32-S3**.
