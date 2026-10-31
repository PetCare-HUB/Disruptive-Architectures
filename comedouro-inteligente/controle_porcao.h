#pragma once
#include <stdint.h>

// Logica independente do Arduino, compartilhada pelos dois perfis.
// A contagem confirma um ciclo mecanico, nao a passagem/peso de racao.
class ControlePorcao {
public:
  enum class Estado { Parado, SoltandoChave, AguardandoChave, Freando, Falha };
  enum class Saida { Desligado, Girar, Frear };
  enum class Falha { Nenhuma, TempoEsgotado, Cancelado };

  constexpr ControlePorcao(uint32_t limite = 8000, uint32_t freio = 150)
      : limite_(limite), freio_(freio) {}

  constexpr bool iniciar(uint32_t agora, bool pressionado) {
    if (estado != Estado::Parado) return false;
    inicio_ = ultimaMudanca_ = agora;
    bruto_ = pressionado;
    // Exigir liberacao estavel mesmo quando a leitura inicial e aberta.
    estado = Estado::SoltandoChave;
    falha = Falha::Nenhuma;
    return true;
  }

  constexpr void atualizar(uint32_t agora, bool pressionado) {
    if (estado == Estado::Parado || estado == Estado::Falha) return;
    if (estado == Estado::Freando) {
      if (uint32_t(agora - inicioFreio_) >= freio_)
        estado = falha == Falha::Nenhuma ? Estado::Parado : Estado::Falha;
      return;
    }
    if (uint32_t(agora - inicio_) >= limite_) {
      pararComFalha(agora, Falha::TempoEsgotado);
      return;
    }
    if (pressionado != bruto_) {
      bruto_ = pressionado;
      ultimaMudanca_ = agora;
    }
    if (uint32_t(agora - ultimaMudanca_) < 30) return;
    if (estado == Estado::SoltandoChave && !bruto_) {
      estado = Estado::AguardandoChave;
    } else if (estado == Estado::AguardandoChave && bruto_) {
      ++porcoes;
      inicioFreio_ = agora;
      estado = Estado::Freando;
    }
  }

  constexpr void cancelar(uint32_t agora) {
    if (estado == Estado::SoltandoChave || estado == Estado::AguardandoChave)
      pararComFalha(agora, Falha::Cancelado);
  }
  constexpr bool rearmar() {
    if (estado != Estado::Falha) return false;
    estado = Estado::Parado;
    falha = Falha::Nenhuma;
    return true;
  }
  constexpr Saida saida() const {
    return estado == Estado::Freando ? Saida::Frear
      : (estado == Estado::SoltandoChave || estado == Estado::AguardandoChave)
        ? Saida::Girar : Saida::Desligado;
  }
  Estado estado = Estado::Parado;
  Falha falha = Falha::Nenhuma;
  uint32_t porcoes = 0;

private:
  constexpr void pararComFalha(uint32_t agora, Falha motivo) {
    falha = motivo;
    inicioFreio_ = agora;
    estado = Estado::Freando;
  }
  uint32_t limite_, freio_, inicio_ = 0, inicioFreio_ = 0, ultimaMudanca_ = 0;
  bool bruto_ = false;
};
