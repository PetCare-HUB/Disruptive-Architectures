#include "../controle_porcao.h"
using C = ControlePorcao;

constexpr bool ciclo() {
  C c;
  if (!c.iniciar(0, false) || c.iniciar(1, false)) return false;
  c.atualizar(30, false);
  c.atualizar(100, true);
  c.atualizar(129, true);
  if (c.porcoes != 0) return false;
  c.atualizar(130, true);
  if (c.porcoes != 1 || c.saida() != C::Saida::Frear) return false;
  c.atualizar(280, true);
  c.atualizar(1000, true);
  return c.estado == C::Estado::Parado && c.porcoes == 1 && c.saida() == C::Saida::Desligado;
}
constexpr bool chaveInicialPressionada() {
  C c;
  c.iniciar(0, true);
  c.atualizar(100, true);
  if (c.porcoes != 0) return false;
  c.atualizar(200, false);
  c.atualizar(230, false);
  c.atualizar(300, true);
  c.atualizar(330, true);
  return c.porcoes == 1;
}
constexpr bool ruido() {
  C c;
  c.iniciar(0, false);
  c.atualizar(30, false);
  for (uint32_t t = 40; t < 100; t += 10) c.atualizar(t, (t / 10) % 2 == 0);
  if (c.porcoes != 0) return false;
  c.atualizar(100, true);
  c.atualizar(130, true);
  return c.porcoes == 1;
}
constexpr bool timeout(bool preso) {
  C c;
  c.iniciar(0, preso);
  c.atualizar(30, preso);
  c.atualizar(8000, preso);
  if (c.saida() != C::Saida::Frear || c.iniciar(8001, false)) return false;
  c.atualizar(8150, preso);
  if (c.estado != C::Estado::Falha || c.porcoes != 0 || c.iniciar(9000, false)) return false;
  return c.rearmar() && c.saida() == C::Saida::Desligado && c.iniciar(10000, false);
}
constexpr bool cancelamento() {
  C c;
  c.iniciar(0, false);
  c.cancelar(10);
  c.atualizar(160, false);
  return c.estado == C::Estado::Falha && c.porcoes == 0 && c.falha == C::Falha::Cancelado;
}
constexpr bool estouroRelogio() {
  C c;
  c.iniciar(UINT32_MAX - 20, false);
  c.atualizar(10, false);
  c.atualizar(20, true);
  c.atualizar(50, true);
  return c.porcoes == 1;
}
static_assert(ciclo(), "Um comando deve completar exatamente uma porcao");
static_assert(chaveInicialPressionada(), "Chave inicial nao conta porcao");
static_assert(ruido(), "Debounce deve rejeitar pulsos curtos");
static_assert(timeout(false), "Fio aberto deve parar por timeout");
static_assert(timeout(true), "Chave presa deve parar por timeout");
static_assert(cancelamento(), "Cancelamento deve parar e bloquear reinicio");
static_assert(estouroRelogio(), "Contagem deve tolerar wrap do millis");
