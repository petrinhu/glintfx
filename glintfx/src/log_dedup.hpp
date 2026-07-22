// SPDX-License-Identifier: MPL-2.0
// EN: LOGTHR-1 (Onda 2, framework-2D, docs/superpowers/plans/2026-07-22-onda2-input-host.md,
//     decision D8) -- pure, headless-testable dedup/throttle policy for repeated RmlUi log
//     messages. ZERO RmlUi/GLFW dependency in this header (testable in isolation, same
//     discipline as src/input_state.hpp / src/glfw_event_translate.hpp): callers translate
//     their own log-severity type into a plain `int` at the call site (see src/system_clock.hpp
//     and src/system_glfw_dedup.hpp, which DO include RmlUi and perform that translation).
//
//     Origin of the flood this fixes: RmlUi core (Source/Core/ElementText.cpp:86, upstream,
//     PINNED -- not patched) logs "No font face defined. Missing 'font-family' property. On
//     element ..." via Rml::SystemInterface::LogMessage once per text element per layout pass
//     when no font face resolves -- the consumer (GusWorld) measured thousands of lines in a
//     60 s headless session. Neither of glintfx's two SystemInterface implementations
//     (SystemClock for embed, SystemInterface_GLFW for App) overrode LogMessage before this
//     slice, so every one of those calls fell through to RmlUi's own LogDefault (unconditional
//     fprintf(stderr, ...)).
//
//     Policy (deterministic, D8):
//       - 1st occurrence of an identical (type, message) pair: always prints, verbatim.
//       - repeats of that same pair: suppressed (no output), EXCEPT a summary line re-emitted
//         at counts 10, 100, 1000, ... (powers of ten) --
//         "<message> (repeated Nx, suppressed)".
//       - is_error == true (the caller's LT_ERROR/LT_ASSERT) BYPASSES dedup entirely: always
//         prints, is never tracked, is never suppressed -- a flood of genuine errors must stay
//         fully visible, never folded into a summary line.
//       - the (type, message) table is capped at `capacity` distinct entries (default
//         kDefaultCapacity, LRU-evicted) so a hostile/degenerate stream that never repeats the
//         same message cannot grow the table without bound. Eviction just means "this exact
//         message is treated as new again" the next time it recurs -- never a crash, never
//         unbounded memory.
//
//     Dedup key is the FULL formatted message (not a prefix): the flood text above still
//     contains a per-element address (Element::GetAddress()), so keying on the exact string
//     still collapses the actual flood (same element, same frame-to-frame repeat) without
//     masking genuinely distinct warnings that merely happen to share a prefix.
// PT: LOGTHR-1 (Onda 2, framework-2D, docs/superpowers/plans/2026-07-22-onda2-input-host.md,
//     decisão D8) -- política de dedup/throttle pura, testável headless, para mensagens de log
//     repetidas do RmlUi. ZERO dependência de RmlUi/GLFW neste header (testável isolado, mesma
//     disciplina de src/input_state.hpp / src/glfw_event_translate.hpp): os chamadores traduzem
//     o próprio tipo de severidade de log para um `int` simples no ponto de chamada (ver
//     src/system_clock.hpp e src/system_glfw_dedup.hpp, que INCLUEM RmlUi e fazem essa
//     tradução).
//
//     Origem do flood corrigido aqui: o núcleo do RmlUi (Source/Core/ElementText.cpp:86,
//     upstream, PINADO -- não remendado) loga "No font face defined. Missing 'font-family'
//     property. On element ..." via Rml::SystemInterface::LogMessage uma vez por elemento de
//     texto por passe de layout quando nenhuma face de fonte resolve -- o consumidor (GusWorld)
//     mediu milhares de linhas numa sessão headless de 60 s. Nenhuma das duas implementações de
//     SystemInterface do glintfx (SystemClock no embed, SystemInterface_GLFW no App)
//     sobrescrevia LogMessage antes desta fatia, então toda chamada caía no LogDefault do
//     próprio RmlUi (fprintf(stderr, ...) incondicional).
//
//     Política (determinística, D8):
//       - 1ª ocorrência de um par (type, message) idêntico: sempre imprime, verbatim.
//       - repetições desse mesmo par: suprimidas (sem saída), EXCETO uma linha de sumário
//         reemitida nas contagens 10, 100, 1000, ... (potências de dez) --
//         "<message> (repeated Nx, suppressed)".
//       - is_error == true (o LT_ERROR/LT_ASSERT do chamador) IGNORA o dedup inteiramente:
//         sempre imprime, nunca é rastreado, nunca é suprimido -- uma enxurrada de erros de
//         verdade precisa continuar totalmente visível, nunca dobrada numa linha de sumário.
//       - a tabela (type, message) tem teto de `capacity` entradas distintas (padrão
//         kDefaultCapacity, evicção LRU) então um stream hostil/degenerado que nunca repete a
//         mesma mensagem não pode crescer a tabela sem limite. Evicção só significa "esta
//         mensagem exata é tratada como nova de novo" na próxima recorrência -- nunca um crash,
//         nunca memória sem limite.
//
//     Chave de dedup é a mensagem formatada INTEIRA (não um prefixo): o texto do flood acima
//     ainda contém o endereço por-elemento (Element::GetAddress()), então chavear na string
//     exata ainda colapsa o flood de fato (mesmo elemento, repetição frame-a-frame) sem
//     mascarar avisos genuinamente distintos que só compartilham um prefixo.
// Copyright (c) 2026 Petrus Silva Costa
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <string>
#include <unordered_map>

namespace glintfx {

// EN: Fail-high, fixed-policy dedup table -- same "never UB, degrade to a safe default"
//     discipline as every other module in this library (InputState, Gamepads, Audio):
//     out-of-capacity/hostile input never grows memory without bound and never crashes.
// PT: Tabela de dedup fail-high, política fixa -- mesma disciplina "nunca UB, degrada a um
//     padrão seguro" de todo outro módulo desta biblioteca (InputState, Gamepads, Audio):
//     input fora do teto/hostil nunca cresce memória sem limite e nunca crasha.
class LogDedup {
public:
  // EN: Generous enough for any real document's distinct warning set; small enough that a
  //     hostile stream of always-different messages costs a bounded amount of memory.
  // PT: Generoso o bastante pro conjunto de avisos distintos de qualquer documento real;
  //     pequeno o bastante para que um stream hostil de mensagens sempre-diferentes custe uma
  //     quantidade de memória limitada.
  static constexpr std::size_t kDefaultCapacity = 256;

  explicit LogDedup(std::size_t capacity = kDefaultCapacity) noexcept
      : capacity_(capacity > 0 ? capacity : 1) {}

  // EN: Decides whether THIS occurrence of (type, message) should be printed, and if so, what
  //     line the caller should print (`out_line`).
  //       - is_error == true: ALWAYS returns true with out_line == message verbatim (never
  //         tracked, never suppressed -- see the class header comment).
  //       - the 1st occurrence of a (type, message) pair not yet seen (or evicted, and
  //         therefore "new again"): returns true with out_line == message verbatim.
  //       - the Nth occurrence for N in {10, 100, 1000, ...}: returns true with out_line == a
  //         built "<message> (repeated Nx, suppressed)" summary line.
  //       - every other repeat: returns false; `out_line` is left untouched and the caller MUST
  //         print nothing for this call.
  // PT: Decide se ESTA ocorrência de (type, message) deve ser impressa e, se sim, qual linha o
  //     chamador deve imprimir (`out_line`).
  //       - is_error == true: SEMPRE retorna true com out_line == mensagem verbatim (nunca
  //         rastreado, nunca suprimido -- ver o comentário de cabeçalho da classe).
  //       - a 1ª ocorrência de um par (type, message) ainda não visto (ou evicto, e portanto
  //         "novo de novo"): retorna true com out_line == mensagem verbatim.
  //       - a N-ésima ocorrência para N em {10, 100, 1000, ...}: retorna true com out_line ==
  //         uma linha de sumário construída "<message> (repeated Nx, suppressed)".
  //       - toda outra repetição: retorna false; `out_line` fica intocado e o chamador NÃO deve
  //         imprimir nada nesta chamada.
  bool should_print(int type, const std::string& message, bool is_error, std::string& out_line) {
    if (is_error) {
      out_line = message;
      return true;
    }

    const Key key{type, message};
    auto it = index_.find(key);
    if (it == index_.end()) {
      // EN: New key -- evict the least-recently-used entry first if already at capacity, then
      //     insert this one at the front (most-recently-used end) with count 1.
      // PT: Chave nova -- evicta a entrada menos-recentemente-usada primeiro se já no teto,
      //     depois insere esta na frente (extremidade mais-recentemente-usada) com contagem 1.
      if (order_.size() >= capacity_) {
        index_.erase(order_.back().key);
        order_.pop_back();
      }
      order_.push_front(Entry{key, 1, 10});
      index_.emplace(key, order_.begin());
      out_line = message;
      return true;
    }

    // EN: Existing key -- touch (move to the MRU front, std::list::splice never invalidates
    //     the moved element's iterator) and bump the occurrence count.
    // PT: Chave existente -- toca (move pra frente MRU; std::list::splice nunca invalida o
    //     iterador do elemento movido) e incrementa a contagem de ocorrência.
    order_.splice(order_.begin(), order_, it->second);
    Entry& entry = *it->second;
    ++entry.count;
    if (entry.count == entry.next_report) {
      out_line = message + " (repeated " + std::to_string(entry.count) + "x, suppressed)";
      entry.next_report *= 10;
      return true;
    }
    return false;
  }

private:
  struct Key {
    int type;
    std::string message;
    bool operator==(const Key& other) const noexcept {
      return type == other.type && message == other.message;
    }
  };
  struct KeyHash {
    std::size_t operator()(const Key& k) const noexcept {
      // EN: Simple combine -- collisions only cost a hash bucket, never correctness (equality
      //     is still checked by unordered_map via operator==).
      // PT: Combinação simples -- colisões só custam um bucket de hash, nunca corretude
      //     (igualdade ainda é checada pelo unordered_map via operator==).
      return std::hash<int>{}(k.type) ^ (std::hash<std::string>{}(k.message) << 1);
    }
  };
  struct Entry {
    Key key;
    std::uint64_t count;
    // EN: Next occurrence count at which a summary line is due (10, then *10 each time it fires).
    // PT: Próxima contagem de ocorrência na qual uma linha de sumário é devida (10, depois *10
    //     a cada disparo).
    std::uint64_t next_report;
  };

  // EN: front = most-recently-used, back = least-recently-used (LRU eviction victim).
  // PT: frente = mais-recentemente-usado, fundo = menos-recentemente-usado (vítima de evicção LRU).
  std::list<Entry> order_;
  std::unordered_map<Key, std::list<Entry>::iterator, KeyHash> index_;
  std::size_t capacity_;
};

} // namespace glintfx
