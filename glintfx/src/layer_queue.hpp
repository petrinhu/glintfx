// SPDX-License-Identifier: MPL-2.0
// EN: D2D-3B -- the PURE buffered-command queue behind Draw2D's opt-in layer/draw-order mode
//     (plan docs/superpowers/plans/2026-07-23-onda5-draw2d-primitives.md, decision D27; the D28
//     scissor-grouping rule this file's own `drain_grouped()` implements). ZERO GL dependency,
//     same discipline as src/sprite_batch.hpp/src/primitives2d.hpp: this header takes
//     already-validated, already-projected draw commands and hands back stable-sorted,
//     scissor-grouped batches for draw2d.cpp (D2D-3B's own GL half) to replay through the
//     UNTOUCHED `SpriteBatch::draw_quad()`. Includes src/transform2d.hpp for `SpriteCorners`
//     (the same TL,TR,BR,BL 4-corner type the camera/primitive math already produces) and
//     `<glintfx/draw2d.hpp>` transitively for `RectF`/`ColorF` -- reusing them here avoids a
//     duplicate POD family, same rationale primitives2d.hpp/sprite_batch.hpp already give for
//     their own includes.
//
//     WHAT THIS QUEUE DOES NOT DO (deliberately, single-source discipline): it does NOT resolve
//     `src_px`'s `{0,0,0,0}` "whole texture" sentinel, does NOT clamp `tint` to [0,1], and does
//     NOT re-validate finiteness of anything it is handed -- `LayerCommand::src_px`/`tint` are
//     stored RAW and resolved/clamped at REPLAY time by `SpriteBatch::draw_quad()`'s own
//     `resolve_src()`/`clampf()` (sprite_batch.hpp, untouched, D31), exactly the same as the
//     streaming path already does. Duplicating either computation here would be a second place
//     the two formulas could drift apart -- draw2d.cpp (the caller) is responsible for handing
//     this queue only ALREADY-validated-finite corners/src_px/tint (D10's guard order, checked
//     BEFORE `push()` is ever called), same split `primitives2d.hpp`'s own header comment
//     documents for `compute_line_corners()`/`compute_outline_strips()`.
//
//     D27 STABLE SORT, literal: `drain_grouped()` stable-sorts by `LayerCommand::layer` ascending
//     -- `std::stable_sort` is the mechanism, not an implementation detail: two commands pushed
//     with the SAME layer value keep their PUSH order in the output (the mutation the adversarial
//     reviewer is briefed to try is swapping `std::stable_sort` for a plain `std::sort`, which
//     offers no such guarantee and can silently reorder same-layer commands run-to-run).
//
//     D28 SCISSOR GROUPING, literal: AFTER the stable sort (grouping happens in the SORTED order,
//     not the original push order -- sorting by layer can interleave commands that had different
//     scissor snapshots at call time), a NEW `LayerGroup` starts whenever a command's scissor
//     snapshot differs from the group's own (`scissor_equal()` below) -- so every group is a
//     maximal RUN of consecutive (in sorted order) commands sharing one scissor state. This is
//     the exact grouping draw2d.cpp's replay loop needs to know precisely when to force a GL
//     flush boundary (one `glScissor` call per group, not per command).
// PT: D2D-3B -- a fila PURA de comandos bufferizados por trás do modo opt-in de
//     camada/ordem-de-desenho do Draw2D (plano docs/superpowers/plans/2026-07-23-onda5-draw2d-
//     primitives.md, decisão D27; a regra de agrupamento por scissor do D28 que o próprio
//     `drain_grouped()` deste arquivo implementa). ZERO dependência de GL, mesma disciplina de
//     src/sprite_batch.hpp/src/primitives2d.hpp: este header recebe comandos de desenho JÁ
//     validados, JÁ projetados, e devolve lotes ordenados-de-forma-estável e agrupados-por-scissor
//     pro draw2d.cpp (a própria metade GL do D2D-3B) reproduzir através do
//     `SpriteBatch::draw_quad()` INTOCADO. Inclui src/transform2d.hpp pro `SpriteCorners` (o
//     MESMO tipo de 4 cantos TL,TR,BR,BL que a matemática de câmera/primitiva já produz) e
//     `<glintfx/draw2d.hpp>` transitivamente pro `RectF`/`ColorF` -- reusá-los aqui evita uma
//     família de POD duplicada, mesma racional que primitives2d.hpp/sprite_batch.hpp já dão pros
//     próprios includes.
//
//     O QUE ESTA FILA NÃO FAZ (de propósito, disciplina de fonte única): NÃO resolve a sentinela
//     `{0,0,0,0}` de "textura inteira" do `src_px`, NÃO clampa `tint` a [0,1], e NÃO revalida a
//     finitude de nada que recebe -- `LayerCommand::src_px`/`tint` são guardados CRUS e
//     resolvidos/clampados no momento do REPLAY pelo próprio `resolve_src()`/`clampf()` de
//     `SpriteBatch::draw_quad()` (sprite_batch.hpp, intocado, D31), exatamente como o caminho
//     streaming já faz. Duplicar qualquer uma das duas contas aqui seria um segundo lugar onde as
//     duas fórmulas poderiam divergir -- o draw2d.cpp (o chamador) é responsável por entregar a
//     esta fila só cantos/src_px/tint JÁ validados como finitos (a ordem de guarda do D10, checada
//     ANTES do `push()` ser sequer chamado), o mesmo split que o próprio comentário de cabeçalho
//     de `primitives2d.hpp` documenta pra `compute_line_corners()`/`compute_outline_strips()`.
//
//     ORDENAÇÃO ESTÁVEL D27, literal: `drain_grouped()` ordena de forma estável por
//     `LayerCommand::layer` ascendente -- `std::stable_sort` é o mecanismo, não um detalhe de
//     implementação: dois comandos empurrados com o MESMO valor de camada mantêm a própria ordem
//     de PUSH na saída (a mutação que o reviewer adversarial está orientado a tentar é trocar
//     `std::stable_sort` por um `std::sort` simples, que não oferece essa garantia e pode
//     reordenar comandos de mesma camada em silêncio, execução a execução).
//
//     AGRUPAMENTO POR SCISSOR D28, literal: DEPOIS da ordenação estável (o agrupamento acontece na
//     ordem ORDENADA, não na ordem original de push -- ordenar por camada pode intercalar comandos
//     que tinham snapshots de scissor diferentes no momento da chamada), um NOVO `LayerGroup`
//     começa sempre que o snapshot de scissor de um comando difere do próprio grupo
//     (`scissor_equal()` abaixo) -- então todo grupo é uma CORRIDA maximal de comandos
//     consecutivos (na ordem ordenada) compartilhando um estado de scissor. Este é exatamente o
//     agrupamento de que o laço de replay do draw2d.cpp precisa pra saber com precisão quando
//     forçar uma fronteira de flush GL (uma chamada `glScissor` por grupo, não por comando).
// Copyright (c) 2026 Petrus Silva Costa
#pragma once

#include "transform2d.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace glintfx::draw2d_detail {

// EN: D28 -- one scissor snapshot, captured by value at push() time (D27: "the scissor snapshot
//     travels with each command"). `active == false` means "no scissor" (the same meaning as
//     `Draw2d::Impl::scissor_active == false` on the streaming side) -- `rect` is meaningless in
//     that case, left at its default-constructed `RectF{}`.
// PT: D28 -- um snapshot de scissor, capturado por valor no momento do push() (D27: "o snapshot
//     do scissor viaja com cada comando"). `active == false` significa "sem scissor" (o mesmo
//     significado de `Draw2d::Impl::scissor_active == false` do lado streaming) -- `rect` é sem
//     significado nesse caso, fica no próprio `RectF{}` default-construído.
struct ScissorSnapshot {
  bool active = false;
  RectF rect{};
};

// EN: Value-equality for two scissor snapshots -- two INACTIVE snapshots are always equal
//     (`rect` is meaningless when `active == false`, so its bits must not affect grouping); two
//     ACTIVE snapshots are equal only when every `rect` field matches exactly (no tolerance --
//     this is state-identity for GL flush-boundary purposes, not a geometric "close enough").
// PT: Igualdade-por-valor pra dois snapshots de scissor -- dois snapshots INATIVOS são sempre
//     iguais (`rect` é sem significado quando `active == false`, então os bits dele não podem
//     afetar o agrupamento); dois snapshots ATIVOS são iguais só quando todo campo de `rect` bate
//     exatamente (sem tolerância -- isto é identidade-de-estado pra fins de fronteira de flush GL,
//     não um "perto o suficiente" geométrico).
inline bool scissor_equal(const ScissorSnapshot& a, const ScissorSnapshot& b) {
  if (a.active != b.active) return false;
  if (!a.active) return true; // both inactive -- rect is don't-care.
  return a.rect.x == b.rect.x && a.rect.y == b.rect.y && a.rect.w == b.rect.w && a.rect.h == b.rect.h;
}

// EN: One buffered draw, in the POST-VALIDATION, POST-PROJECTION state D27 documents: final
//     screen-space corners (already run through world_to_screen() when a camera was active AT
//     CALL TIME, D13's per-call semantics preserved even under buffering -- see this file's own
//     header comment), the resolved texture identity + its own pixel dimensions (captured now,
//     not re-looked-up at replay, so a destroy_texture() call later in the SAME bracket cannot
//     retroactively corrupt an already-queued command), and RAW `src_px`/`tint` (resolved/clamped
//     at replay, single source, see this file's own header comment).
// PT: Um desenho bufferizado, no estado PÓS-VALIDAÇÃO, PÓS-PROJEÇÃO que o D27 documenta: cantos
//     finais em espaço de tela (já passados por world_to_screen() quando uma câmera estava ativa
//     NO MOMENTO DA CHAMADA, a semântica por-chamada do D13 preservada mesmo sob buffering -- ver
//     o próprio comentário de cabeçalho deste arquivo), a identidade de textura resolvida + as
//     próprias dimensões em pixel dela (capturadas agora, não re-consultadas no replay, pra que
//     uma chamada destroy_texture() mais tarde no MESMO bracket não possa corromper
//     retroativamente um comando já enfileirado), e `src_px`/`tint` CRUS (resolvidos/clampados no
//     replay, fonte única, ver o próprio comentário de cabeçalho deste arquivo).
struct LayerCommand {
  SpriteCorners corners;
  std::uint32_t texture_id = 0;
  int tex_w = 0;
  int tex_h = 0;
  RectF src_px{};
  ColorF tint{};
  ScissorSnapshot scissor{};
  int layer = 0;
};

// EN: One maximal run of consecutive (post layer-sort) commands sharing one scissor state (D28).
// PT: Uma corrida maximal de comandos consecutivos (pós-ordenação-por-camada) compartilhando um
//     estado de scissor (D28).
struct LayerGroup {
  ScissorSnapshot scissor{};
  std::vector<LayerCommand> commands;
};

enum class PushResult {
  Ok,
  CapExceeded, // D27: the 262144-command memory bound was reached -- the command was DROPPED.
};

// EN: Pure per-bracket command buffer (D27). No GL call anywhere in this class.
// PT: Buffer de comando puro, por-bracket (D27). Nenhuma chamada GL em nenhum lugar desta classe.
class LayerQueue {
public:
  // EN: D27, literal: "hard cap 262144 commands per bracket (~24 MiB at ~96 B/command)" -- a
  //     MEMORY bound against a hostile stream (the same class as SpriteBatch's own 4096-quad
  //     capacity flush, D4), not a performance claim (D30 measures that).
  // PT: D27, literal: "teto rígido de 262144 comandos por bracket (~24 MiB a ~96 B/comando)" --
  //     um limite de MEMÓRIA contra um stream hostil (a mesma classe da própria capacidade de
  //     4096 quads do SpriteBatch, D4), não um claim de performance (o D30 mede isso).
  static constexpr std::size_t kMaxCommands = 262144;

  // EN: Appends `cmd` unless the cap is already reached, in which case the command is DROPPED
  //     (never appended, never partially stored) and `CapExceeded` is returned -- the caller
  //     (draw2d.cpp) owns the dedup'd log line, this class cannot log (pure).
  // PT: Anexa `cmd` a menos que o teto já esteja atingido, caso em que o comando é DESCARTADO
  //     (nunca anexado, nunca parcialmente guardado) e `CapExceeded` é devolvido -- o chamador
  //     (draw2d.cpp) é dono da linha de log dedup'd, esta classe não pode logar (é pura).
  PushResult push(const LayerCommand& cmd) {
    if (commands_.size() >= kMaxCommands) return PushResult::CapExceeded;
    commands_.push_back(cmd);
    return PushResult::Ok;
  }

  bool empty() const { return commands_.empty(); }
  std::size_t size() const { return commands_.size(); }

  // EN: Discards every buffered command without returning them -- D27/D32's own "shutdown()
  //     discards any buffer" contract; also used to reset an armed-but-never-drained queue.
  // PT: Descarta todo comando bufferizado sem devolvê-los -- o próprio contrato "shutdown()
  //     descarta qualquer buffer" do D27/D32; também usado pra resetar uma fila armada mas nunca
  //     drenada.
  void clear() { commands_.clear(); }

  // EN: D27/D28, literal -- stable-sorts by `layer` ascending (equal layer keeps PUSH order,
  //     `std::stable_sort`'s own guarantee, the mutation-killer this file's header comment
  //     names), THEN groups the sorted result into maximal runs of consecutive same-scissor
  //     commands (`scissor_equal()` above). Clears the queue (D27: buffered state does not
  //     survive its own drain). Pure, zero GL -- the caller (draw2d.cpp) applies each group's
  //     scissor state and replays its commands through the batcher.
  // PT: D27/D28, literal -- ordena de forma estável por `layer` ascendente (camada igual mantém a
  //     ordem de PUSH, a própria garantia do `std::stable_sort`, o mata-mutação que o comentário
  //     de cabeçalho deste arquivo nomeia), DEPOIS agrupa o resultado ordenado em corridas
  //     maximais de comandos consecutivos de mesmo scissor (`scissor_equal()` acima). Limpa a
  //     fila (D27: estado bufferizado não sobrevive ao próprio dreno). Pura, zero GL -- o
  //     chamador (draw2d.cpp) aplica o estado de scissor de cada grupo e reproduz os comandos
  //     dele pelo batcher.
  std::vector<LayerGroup> drain_grouped() {
    std::vector<LayerCommand> sorted = std::move(commands_);
    commands_.clear();
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const LayerCommand& a, const LayerCommand& b) { return a.layer < b.layer; });

    std::vector<LayerGroup> groups;
    for (const LayerCommand& cmd : sorted) {
      if (groups.empty() || !scissor_equal(groups.back().scissor, cmd.scissor)) {
        LayerGroup group;
        group.scissor = cmd.scissor;
        groups.push_back(std::move(group));
      }
      groups.back().commands.push_back(cmd);
    }
    return groups;
  }

private:
  std::vector<LayerCommand> commands_;
};

} // namespace glintfx::draw2d_detail
