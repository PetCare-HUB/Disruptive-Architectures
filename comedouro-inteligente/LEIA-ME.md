# Comedouro ESP32-S3: testar antes da montagem

Hardware informado: ESP32-S3 CAM OEM N16R8 com OV5640, DHT11, TB6612 e motor DC com redutor. Um novo acionamento do microinterruptor corresponde a uma porção mecânica. Não há HC-SR04.

## O que o Wokwi testa

O mesmo `controle_porcao.h` é usado nos perfis Wokwi e físico. O simulador executa o código ESP32-S3, mas não reproduz todo o equipamento:

| Equipamento | Representação neste teste |
| --- | --- |
| ESP32-S3 CAM N16R8 | ESP32-S3 DevKitC, flash 16 MB / PSRAM 8 MB octal |
| DHT11 | DHT22 virtual; perfil físico troca o protocolo para DHT11 |
| TB6612 e motor | Quatro LEDs mostram AIN1, AIN2, PWMA e STBY; não simulam potência nem rotação |
| Microinterruptor | Botão azul, contato aberto em repouso |
| OV5640 | Sem emulação; `cameraDisponivel=false` no Wokwi |

O teste não valida corrente, torque, entupimento, peso por porção, imagem ou pinagem da câmera OEM. Não é ainda “gravar e conectar sem ajustes”. A [lista oficial de hardware](https://docs.wokwi.com/getting-started/supported-hardware) não inclui DHT11, TB6612 ou OV5640 nativos.

## Abrir no Wokwi

Os binários gerados estão em `build/wokwi/`, ignorados pelo Git. O `wokwi.toml` aponta para eles. No VS Code com extensão Wokwi, abra esta pasta e execute **Wokwi: Start Simulator**. Recompile após editar o código.

No navegador:

1. Abra https://wokwi.com/projects/new/esp32-s3.
2. Substitua o conteúdo de `diagram.json` pelo arquivo desta pasta.
3. No editor, pressione F1 e escolha **Upload Firmware and Start Simulation…**.
4. Selecione `build/wokwi/comedouro-inteligente.ino.merged.bin` (o arquivo inclui bootloader e partições).

O upload do binário executa a versão compilada; o `sketch.ino` padrão do site não é o código deste projeto. Para compilar no site, copie também o conteúdo do `.ino`, `config.h`, `controle_porcao.h` e `libraries.txt`. Não grave o binário de simulação na montagem real. [Carregar firmware próprio no Wokwi](https://docs.wokwi.com/guides/esp32#custom-application-firmware).

## Roteiro interativo

1. Após iniciar, o Serial mostra `MOTOR: parado`. Todos os sinais ficam desligados.
2. Pressione o botão verde **PEDIR 1 PORCAO**, ou envie `d` no Serial. AIN1, PWMA e STBY acendem; AIN2 permanece apagado.
3. Antes de oito segundos simulados, pressione o botão azul **MICROINTERRUPTOR** por mais de 30 ms. Aparece `PORCAO_CONCLUIDA`, o contador aumenta uma vez, o motor freia por 150 ms e para.
4. Segure o azul antes de pedir outra porção: a chave inicial não conta. Solte o azul e pressione novamente para concluir.
5. Peça uma porção e não pressione o azul: após oito segundos, o motor freia e entra em `falha_motor`. Novas porções são rejeitadas.
6. Pressione o vermelho **REARMAR FALHA** ou envie `r`: volta ao repouso, sem alimentar sozinho. `p` durante o movimento cancela e exige rearme.
7. Clique no DHT22 e altere temperatura/umidade. Acima de 32 °C, aparece alerta; isso **não** aciona o motor.

Botões verdes/vermelhos são controles do teste, não peças que você precisa comprar. No físico, os comandos podem ser enviados pelo Serial Monitor. O contador reinicia ao reiniciar o ESP32. Ele confirma o ciclo da chave, não que ração efetivamente caiu.

## Dashboard / MQTT

A cada dois segundos, publica em `petcarehub/fiap/comedouro01/telemetria`, no broker `broker.hivemq.com:1883`. Abra o dashboard e selecione **Wokwi / dispositivo via MQTT**. Temperatura, umidade, estado do motor e ciclos são exibidos. `origem=wokwi` identifica o teste.

Nível, consumo e gramas são `null`: não há sensores/calibração para medi-los. Não são preenchidos com números aleatórios. O MQTT público recebe somente telemetria; não aceita comandos para movimentar um motor real. O teste funciona pelo Serial e pelos botões mesmo sem MQTT.

## Compilar nesta máquina

Arduino ESP32 3.3.11, DHT sensor library for ESPx 1.19 e PubSubClient 2.8 estavam instalados. Execute, a partir desta pasta:

```powershell
./compilar.ps1 -Perfil wokwi
./compilar.ps1 -Perfil fisico
```

O script compila sete cenários do controlador usando `static_assert`: os cenários são executados pelo avaliador de constantes C++, e qualquer falha impede a compilação. Não são testes elétricos nem execução no Wokwi.

## Preparação do perfil físico

`config.h` usa `PETCARE_SIMULATION=1` por padrão. O script passa `0` ao compilar o perfil físico. Pela Arduino IDE, altere para `0`, selecione **ESP32S3 Dev Module**, flash **16 MB**, **OPI PSRAM**, partição **16M Flash (3MB APP/9.9MB FATFS)** e USB CDC desativado para Serial pela porta USB-UART. Preencha o Wi-Fi localmente; não publique a senha.

O perfil físico usa DHT11. O motor vem desabilitado (`MOTOR_FISICO_HABILITADO=false`), e a câmera também (`CAMERA_PINOUT_CONFIRMADO=false`): ainda faltam tensão/corrente do motor, sentido correto, contato COM/NO ou COM/NC e mapa da câmera OEM. O mapa de câmera em `config.h` é uma **referência candidata S3-EYE/Freenove, não confirmação do fabricante**. Não habilite somente porque a compilação passou.

Pinagem proposta para a bancada, sem cartão microSD (confirmar contra o mapa da câmera e a serigrafia da placa):

| Sinal | GPIO proposto |
| --- | --- |
| DHT11 DATA | 42 |
| Microinterruptor NO | 41 (COM em GND) |
| TB6612 AIN1 / AIN2 | 1 / 2 |
| TB6612 PWMA / STBY | 38 / 40 |

VCC lógico do TB6612 em 3,3 V, GND comum; motor em AO1/AO2. VM depende da alimentação confirmada do motor, não é saída de GPIO. Coloque pull-down externo de 10 kΩ em STBY para manter o motor desabilitado durante reset/gravação; não fixe STBY em 3,3 V. Mantenha BIN1/BIN2/PWMB do canal não usado em GND. Confira a placa do seu módulo e o desacoplamento recomendado no [datasheet Toshiba](https://toshiba-semicon-storage.com/info/TB6612FNG_datasheet_en_20141001.pdf?did=10660&prodName=TB6612FNG).

Após confirmar o mapa, o perfil físico permite uma foto JPEG real em `http://IP_DO_ESP/foto.jpg` na rede local (IP impresso no Serial). Não há streaming nem cálculo de consumo; isso exige validação/calibração posterior. Não coloque credenciais do Wi-Fi no perfil Wokwi.

Primeiro teste a câmera e o DHT11 com o motor sem alimentação; depois valide chave, sentido de rotação e parada com o mecanismo vazio. Ajuste timeout/freio na bancada antes de usar ração. A fonte ajustável e a medição do circuito são etapas de bancada, não algo que o Wokwi consegue substituir.

Os binários antigos em `esp32.esp32.esp32cam/` são da versão AI Thinker e não são utilizados pelo novo projeto.

## Validação realizada

- Compilação para ESP32-S3 dos perfis Wokwi e físico concluída.
- Sete testes do controlador aprovados por avaliação constante C++.
- Diagrama carregado no editor web do Wokwi; upload do firmware falhou na automação do navegador. Execução interativa no Wokwi e MQTT ponta a ponta ainda não confirmados.
- Nenhum firmware gravado na placa e nenhum teste elétrico realizado.
