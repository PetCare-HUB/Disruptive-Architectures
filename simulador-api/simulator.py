"""Simulador virtual de dispositivos IoT para a API Java do PetCare Hub."""

from __future__ import annotations

import argparse
import json
import math
import os
import random
import sys
import time
from datetime import date, datetime, timedelta
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import Request, urlopen


# ---------------------------------------------------------------------------
# PET ID: troque o valor padrão pelo ID do pet já cadastrado no Oracle.
# Também é possível informar o ID pelo terminal com PETCARE_PET_ID.
# O token deve pertencer a um tutor autorizado a acessar esse pet.
# ---------------------------------------------------------------------------
PET_ID = int(os.getenv("PETCARE_PET_ID", "1"))

# Destino local por padrão: configure explicitamente para usar outro ambiente.
API_BASE_URL = os.getenv("PETCARE_API_URL", "http://localhost:8080").rstrip("/")
API_TOKEN = os.getenv("PETCARE_TOKEN", "")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Envia leituras sintéticas aos endpoints IoT da API Java.")
    parser.add_argument("--cycles", type=int, default=0,
                        help="Número de ciclos; 0 calcula pelo histórico de dias (padrão).")
    parser.add_argument("--interval", type=float, default=1,
                        help="Segundos reais entre ciclos (padrão: 1; 0 executa em lote).")
    parser.add_argument("--history-days", type=int, default=14,
                        help="Distribui a carga por esse número de dias passados (padrão: 14).")
    parser.add_argument("--samples-per-day", type=int, default=8,
                        help="Leituras por dia simulado (padrão: 8, a cada ~3 horas).")
    parser.add_argument("--month-to-date", action="store_true",
                        help="Preenche os horários ausentes do primeiro dia deste mês até agora, sem duplicar leituras próximas.")
    parser.add_argument("--since", type=date.fromisoformat, default=None, metavar="AAAA-MM-DD",
                        help="Como --month-to-date, mas a partir da data informada; serve para buracos que começam no mês anterior.")
    parser.add_argument("--scenario", choices=("normal", "alert-once"), default="normal",
                        help="alert-once envia bateria abaixo de 20%% apenas no primeiro ciclo.")
    parser.add_argument("--seed", type=int, default=None,
                        help="Semente opcional para repetir a sequência de valores.")
    return parser.parse_args()


def activity_for_cycle(cycle: int, seed: int) -> str:
    """Gera blocos de atividade de durações variadas, em vez de alternar a cada leitura."""
    rng = random.Random(seed)
    states = ("SEDENTARIO", "MODERADO", "ATIVO")
    state_index = 0
    block_start = 1
    while block_start <= cycle:
        duration = rng.randint(2, 6)
        block_end = block_start + duration - 1
        if cycle <= block_end:
            return states[state_index]
        state_index = (state_index + rng.choices((0, 1, 2), weights=(20, 55, 25))[0]) % len(states)
        block_start = block_end + 1
    return states[state_index]


def build_readings(
    cycle: int,
    rng: random.Random,
    alert_once: bool,
    reading_time: datetime,
    activity_seed: int,
) -> list[tuple[str, dict[str, object]]]:
    activity = activity_for_cycle(cycle, activity_seed)
    # Simula recargas periódicas para o histórico de semanas não ficar travado em bateria baixa.
    battery_phase = (cycle - 1) % 64
    battery = 100 - min(80, battery_phase * 2)
    if alert_once and cycle == 1:
        battery = 18

    # Refeições ao longo do dia e reposição periódica do reservatório.
    meal_cycle = cycle % 3 == 0
    consumed_g = round(rng.uniform(38, 55) if meal_cycle else rng.uniform(0, 4), 1)
    food_level = 90 - ((cycle - 1) % 16) * 4

    temperature = round(24 + 2 * math.sin(cycle / 6) + rng.uniform(-0.3, 0.3), 1)
    humidity = max(35, min(70, round(52 + 7 * math.sin(cycle / 9) + rng.uniform(-2, 2))))
    air_quality = max(250, min(850, int(480 + 90 * math.sin(cycle / 7) + rng.randint(-25, 25))))

    return [
        ("coleira", {
            "petId": PET_ID,
            "statusAtividade": activity,
            "nivelBateria": battery,
            "timestampLeitura": reading_time.isoformat(timespec="seconds"),
        }),
        ("comedouro", {
            "petId": PET_ID,
            "nivelRacaoPct": food_level,
            "pesoConsumidoG": consumed_g,
            "timestampLeitura": reading_time.isoformat(timespec="seconds"),
        }),
        ("ambiente", {
            "petId": PET_ID,
            "temperaturaAmbiente": temperature,
            "umidadePct": humidity,
            "qualidadeArPpm": air_quality,
            "petPresente": cycle % 5 != 0,
            "timestampLeitura": reading_time.isoformat(timespec="seconds"),
        }),
    ]


def post_reading(kind: str, payload: dict[str, object]) -> None:
    url = f"{API_BASE_URL}/leituras/{kind}"
    request = Request(
        url,
        data=json.dumps(payload).encode("utf-8"),
        headers={"Content-Type": "application/json", "Authorization": f"Bearer {API_TOKEN}"},
        method="POST",
    )
    try:
        with urlopen(request, timeout=20) as response:
            body = response.read().decode("utf-8", errors="replace")
            print(f"[{datetime.now().isoformat(timespec='seconds')}] {kind}: HTTP {response.status} — {body}")
    except HTTPError as error:
        detail = error.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"{kind}: HTTP {error.code} — {detail}") from error
    except URLError as error:
        raise RuntimeError(f"{kind}: falha ao acessar {url}: {error.reason}") from error


def existing_readings(kind: str, start: datetime, end: datetime) -> list[datetime]:
    query = urlencode({
        "de": start.isoformat(timespec="seconds"),
        "ate": end.isoformat(timespec="seconds"),
    })
    url = f"{API_BASE_URL}/pets/{PET_ID}/leituras/{kind}?{query}"
    request = Request(url, headers={"Authorization": f"Bearer {API_TOKEN}"}, method="GET")
    try:
        with urlopen(request, timeout=30) as response:
            records = json.loads(response.read().decode("utf-8"))
        return [datetime.fromisoformat(item["timestampLeitura"]) for item in records]
    except HTTPError as error:
        detail = error.read().decode("utf-8", errors="replace")
        raise RuntimeError(f"Falha ao consultar {kind}: HTTP {error.code} — {detail}") from error
    except (URLError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
        raise RuntimeError(f"Falha ao consultar leituras existentes de {kind}: {error}") from error


def main() -> int:
    args = parse_args()
    if args.cycles < 0 or args.interval < 0 or args.history_days < 0 or args.samples_per_day <= 0:
        print("--cycles, --interval e --history-days não podem ser negativos; --samples-per-day deve ser positivo.", file=sys.stderr)
        return 2
    if args.month_to_date and args.since:
        print("Use --month-to-date ou --since, não os dois.", file=sys.stderr)
        return 2
    if not API_TOKEN:
        print("Defina PETCARE_TOKEN com o JWT de um tutor autorizado ao PET_ID.", file=sys.stderr)
        return 2

    rng = random.Random(args.seed)
    activity_seed = args.seed if args.seed is not None else random.SystemRandom().randrange(2**32)
    now = datetime.now()
    fill_gaps = args.month_to_date or args.since is not None
    if args.since is not None and args.since > now.date():
        print("--since não pode ser uma data futura.", file=sys.stderr)
        return 2
    if fill_gaps:
        if args.since is not None:
            history_start = datetime.combine(args.since, datetime.min.time())
            period = f"de {args.since.isoformat()} até agora"
        else:
            history_start = now.replace(day=1, hour=0, minute=0, second=0, microsecond=0)
            period = "mês atual até hoje"
        history_end = now
        slot_step = timedelta(hours=24 / args.samples_per_day)
        slots: list[datetime] = []
        slot = history_start
        while slot <= history_end:
            slots.append(slot)
            slot += slot_step
        kinds = ("coleira", "comedouro", "ambiente")
        existing = {kind: existing_readings(kind, history_start, history_end) for kind in kinds}
        # Se já existe uma leitura a até metade do espaçamento nominal, considera o horário coberto.
        tolerance = slot_step / 2
        pending_slots: list[tuple[datetime, list[str]]] = []
        for slot in slots:
            missing = [
                kind for kind in kinds
                if not any(abs((timestamp - slot).total_seconds()) <= tolerance.total_seconds()
                           for timestamp in existing[kind])
            ]
            if missing:
                pending_slots.append((slot, missing))
        total_cycles = len(pending_slots)
        print(f"Leituras já existentes consultadas. Horários a preencher: {total_cycles} de {len(slots)}.")
    else:
        history_start = now - timedelta(days=args.history_days)
        total_cycles = args.cycles or max(1, args.history_days * args.samples_per_day)
        pending_slots = []
        period = f"{args.history_days} dias"
    print(f"API: {API_BASE_URL}")
    print(f"PET_ID: {PET_ID}  (alterável no início do arquivo ou via PETCARE_PET_ID)")
    print(
        f"Cenário: {args.scenario}; intervalo real: {args.interval}s; "
        f"histórico: {period}; ciclos: {total_cycles}"
    )
    print("Pare com Ctrl+C. As leituras passam pelo Java e são persistidas no Oracle.")

    cycle = 0
    try:
        while cycle < total_cycles:
            cycle += 1
            print(f"\nCiclo {cycle}")
            if fill_gaps:
                reading_time, missing_kinds = pending_slots[cycle - 1]
            else:
                reading_time = history_start + timedelta(
                    days=args.history_days * (cycle - 1) / max(total_cycles - 1, 1)
                )
                missing_kinds = ["coleira", "comedouro", "ambiente"]
            readings = build_readings(cycle, rng, args.scenario == "alert-once", reading_time, activity_seed)
            for kind, payload in readings:
                if kind in missing_kinds:
                    post_reading(kind, payload)
            if cycle >= total_cycles:
                break
            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("\nSimulador encerrado pelo usuário.")
    except RuntimeError as error:
        print(f"Erro: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
