# Simulador virtual de dispositivos

Simula uma coleira, um comedouro e um sensor ambiental enviando leituras HTTP para a API Java. O backend valida o tutor e o pet, persiste no Oracle, recalcula o score e pode criar alertas.

## Onde alterar o ID do pet

No início de `simulator.py`, altere o valor padrão nesta linha:

```python
PET_ID = int(os.getenv("PETCARE_PET_ID", "1"))
```

Troque `1` pelo **ID numérico do pet já cadastrado no Oracle**. Como alternativa, configure `PETCARE_PET_ID` no terminal. O JWT também precisa pertencer a um tutor autorizado a acessar esse pet; caso contrário, a API recusará a leitura.

## Configurar API, token e pet

O destino padrão é `http://localhost:8080` para evitar enviar dados sem querer à produção. No PowerShell:

```powershell
$env:PETCARE_API_URL = "http://localhost:8080"
$env:PETCARE_TOKEN = "<JWT_DO_TUTOR>"
$env:PETCARE_PET_ID = "1"
```

Não salve o token no script, no README ou no Git. Para outro ambiente, informe explicitamente o endereço correto em `PETCARE_API_URL`.

## Executar

Requer Python 3.10 ou superior e não precisa de bibliotecas externas.

```powershell
python .\simulador-api\simulator.py --history-days 14 --samples-per-day 8 --interval 1
```

Cada ciclo envia três leituras. Por padrão, o simulador gera 14 dias de histórico, com 8 amostras por dia (aproximadamente uma a cada 3 horas), e espera 1 segundo real entre os ciclos. Os timestamps são distribuídos do início do período passado até agora, para acelerar a criação do histórico. O estado da coleira permanece em blocos de 2 a 6 leituras, variando entre sedentário, moderado e ativo.

Para gerar 30 dias com 12 amostras por dia:

```powershell
python .\simulador-api\simulator.py --history-days 30 --samples-per-day 12 --interval 1
```

Para completar o mês atual até agora sem repetir horários já cobertos, use:

```powershell
python .\simulador-api\simulator.py --month-to-date --samples-per-day 8 --interval 1
```

Esse modo consulta os históricos existentes do pet na API e cria leituras em intervalos de aproximadamente 3 horas apenas quando não encontra uma amostra próxima (margem de 1h30) em cada tipo de sensor. Se uma execução parar, rode o mesmo comando novamente: ele consulta os dados outra vez e continua preenchendo os horários faltantes.

`--history-days` define quantos dias o histórico cobre, `--samples-per-day` define a densidade e `--interval` controla a espera real entre ciclos. São três requisições por ciclo (coleira, comedouro e ambiente), então 30 dias × 12 amostras resultam em 1.080 requisições. Use um `--interval` maior se o Render limitar a taxa de requisições.

Cada ciclo envia três leituras:

- `POST /leituras/coleira`
- `POST /leituras/comedouro`
- `POST /leituras/ambiente`

`--seed 42` repete a mesma sequência de variações e estados de atividade.

### Demonstrar alerta de bateria baixa

```powershell
python .\simulador-api\simulator.py --cycles 3 --interval 5 --scenario alert-once
```

O primeiro ciclo envia bateria abaixo de 20% uma única vez; os ciclos seguintes retornam a valores normais, evitando abrir alertas repetidos.

## Limites desta etapa

- O simulador chama diretamente a API Java; não publica MQTT nem escreve diretamente no MongoDB.
- A API atual recebe `petId`. Migrar para `deviceId` exige uma mudança posterior no backend para resolver o dispositivo associado ao pet.
- O histórico é gravado pelo Java no Oracle e pode depois ser exportado para JSON e importado no MongoDB.
- Os valores são sintéticos para demonstração, não são dados clínicos reais.
