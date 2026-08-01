// SPDX-License-Identifier: Apache-2.0
// EN: draw2d_perf_budget (PERF-D2D3, D30) -- declared performance numbers for glintfx's Draw2D
//     atom, a DIFFERENT concern from the adversarial `qa-engineer`'s correctness suite (this
//     file's job is measurement, not fail-high/hostile-input coverage -- see
//     `performance-engineer`'s own mandate vs `qa-engineer`'s). Plan
//     docs/superpowers/plans/2026-07-23-onda5-draw2d-primitives.md section 5 item 5, D30. Three
//     metrics, printed as machine-readable `GLINTFX_PERF <key>=<value>` lines every run (visible
//     in the CI log regardless of gate outcome):
//       (a) pure_batcher_quads_per_s  -- 50k quads through SpriteBatch alone (CPU, no display,
//           no LayerQueue involved) -- the batcher's own ceiling, reported from the FASTEST
//           wall-clock repeat (see the METHOD paragraph below for why a best-of-N and not a
//           median).
//       (b) layered_streaming_ratio   -- the SAME 50k-command stream through the buffered path
//           (LayerQueue::push -> drain_grouped() -> SpriteBatch::draw_quad() replay, the EXACT
//           sequence draw2d.cpp's own replay loop runs at end()), reported as
//           time_layered / time_streaming (>= 1.0 -- the layered path can only ADD work, it
//           reuses the SAME batcher underneath, D31's zero-diff set), each arm taken as its own
//           fastest THREAD-CPU-TIME repeat (METHOD paragraph below).
//       (c) e2e_10k_sprite_1k_prim_median_ms -- a real Draw2d bracket (10k sprites, one texture)
//           plus a real Draw2d bracket (1k draw_filled_rect primitives), both under a live GL
//           3.3 context on Xvfb/llvmpipe, glFinish()'d before the clock stops (same discipline
//           fonteng_perf_bench.cpp's own --axis=frame uses) -- median over 30 frames (first 2
//           discarded as warm-up, same "discard cold-cache frames" convention as every timed
//           loop in that file).
//
//     DOWNGRADES, DECLARED (not hidden -- this house's own discipline, D30's own text): llvmpipe
//     rasterizes on the CPU -- metric (c) is a CPU-RASTER REGRESSION GUARD, not a GPU throughput/
//     bandwidth/vsync-pacing truth (same caveat golden_test's own comment in tests/CMakeLists.txt
//     states for pixel-exact tests). Metrics (a)/(b) ARE real CPU numbers (no GL involved at
//     all -- SpriteBatch/LayerQueue are pure headers). Metric (c) uses a SINGLE texture for the
//     10k-sprite bracket (no texture-switch stream) -- the worst-case multi-texture-interleave
//     cost is a DIFFERENT, un-measured axis (seed SEED-PERF-TEXSWITCH if it ever needs pricing).
//
//     METHOD OF THE RATIO GATE, REWRITTEN 2026-07-25 AFTER A REAL CI FAILURE (PERF-D2D3, the
//     leader's call was "fix the method", not "raise the ceiling"): the gate read 4.482 on the
//     self-hosted runner and reddened main, while 12 clean runs of the SAME code in the SAME
//     container measured 3.592 to 3.806 (median 3.721, stdev 0.064). The runner shares a physical
//     machine with its owner's interactive work, so contention is STRUCTURAL there and will
//     recur. Note that the version that failed ALREADY took a median of 8 paired per-repeat
//     ratios: a median filters isolated SPIKES, and this was not a spike, so a bigger N alone
//     fixes nothing. Measured here (probe replicating both arms verbatim, 30 process launches
//     across three contention conditions -- idle, 6 CPU burners inside the same 4-CPU cgroup, and
//     16 CPU burners on the host outside it):
//       - median of paired ratios on the WALL clock (the version that failed): 2.23 to 7.80. The
//         pairing itself is the flaw. Each repeat times the two arms in SEQUENCE, so a burst that
//         lands on ONE arm distorts that repeat's ratio in whichever direction it hit (pooled
//         per-repeat samples under load ranged 0.32 to 23.08).
//       - min per arm on the THREAD CPU clock (what this file now does): 2.965 to 3.558 across
//         all 30 launches, and its IDLE and LOADED distributions overlap.
//     Two independent changes buy that, and both are load-bearing:
//       1. CLOCK_THREAD_CPUTIME_ID instead of steady_clock. These two arms are pure single-
//          threaded CPU with no I/O and no sleeping, so descheduling is pure measurement error;
//          the CPU clock simply does not count it.
//       2. The MINIMUM of each arm across repeats, taken per arm INDEPENDENTLY, instead of a
//          median of paired ratios. Contention can only ADD time, never remove it, so the
//          fastest repeat is the best estimate of the uncontended cost -- the standard best-of-N
//          discipline for microbenchmarks under noise. Per arm and not per pair, because the two
//          arms cannot be hit by the same burst at the same time.
//     kAbRepeats = 64 (first discarded as warm-up, 63 samples per arm) is measured, not guessed:
//     subsampling those same launches, the estimator needs about 48 repeats to converge (worst
//     reading 5.31 at N=16, 4.98 at N=32, then 3.56 at both N=48 and N=63), because a contention
//     burst can cover a full second of repeats and the minimum needs at least one clean window.
//     64 is the next power of two past the convergence knee. kAbCommands = 50000 rather than the
//     original 100000 for the same robustness reason, also measured: at 100k the layered arm
//     holds ~9.6 MiB of LayerCommand and a co-runner evicts it from a shared L3, which costs real
//     CPU cycles that the CPU clock cannot filter (min-per-arm at 100k still drifted to 4.115
//     under load, versus 3.520 at 50k). Metric (a) is NOT distorted by the smaller size: the
//     streaming arm measures 9.36 M quads/s at 50k against 9.32 M at 100k, inside the run-to-run
//     spread. Metric (b) IS size-dependent by construction (the sort is n log n while the rest of
//     both arms is linear) so the ceiling below is only meaningful for kAbCommands as it stands;
//     changing that constant requires re-deriving the ceiling.
//     COST, DECLARED: the (a)+(b) block goes from ~0.4 s to ~1.6 s of wall time, so the whole
//     test goes from ~3.8 s to ~5.0 s in the CI container. That is the price of the gate not
//     flaking, paid once per CI run.
//
//     THE GATE, TWO-TIER (D30, literal): with GLINTFX_PERF_STRICT=1 in the environment, metrics
//     (a) and (c) FAIL if they cross their own BASELINE constant below by more than 1.5x (worse
//     direction: lower quads/s for (a), higher ms for (c)). Without the env var, only a blunt 10x
//     SANITY ceiling fails the test -- the number is still printed either way.
//     !! SINCE 2026-07-25, NOTHING SETS GLINTFX_PERF_STRICT=1: the only job that armed it ran on
//     the self-hosted runner, retired together with the whole .forgejo/ directory. The strict
//     tier is DORMANT, not removed -- set the variable by hand on a machine whose baselines you
//     trust. See AGENTS.md's CI-policy section for the pending item that gives it a home again. The RATIO gate (b, layered <= 3.9x streaming, re-derived 2026-07-25
//     -- see the CEILING paragraph below) is MACHINE-RELATIVE by construction (both arms run on
//     the SAME machine, SAME process, SAME invocation) so it is STRICT EVERYWHERE, unconditional
//     on GLINTFX_PERF_STRICT -- it is the direct, always-on answer to this wave's risk 2 ("layers
//     vs batching is where performance can degrade", plan section 0).
//
//     BASELINE PROVENANCE (honesty over invention -- the house's own named rule: "baseline
//     medida, nunca inventada"): the three constants below were MEASURED by this agent run on
//     2026-07-23, on the sandboxed dev container this session had shell access to (Fedora 44,
//     12th Gen Intel Core i5-12500H, 16 logical cores, 31 GiB RAM) -- NOT the self-hosted runner
//     named in the brief (a different physical machine this agent process cannot reach). This is
//     a REAL measurement, never a guess, but it is a STAND-IN baseline: per D30 the
//     leader/orchestrator should swap these three constants for the numbers a dedicated, stable
//     machine prints (a one-line diff each, this comment's date/host updated). That machine was
//     the self-hosted runner retired on 2026-07-25, so the swap is BLOCKED until the heavy job
//     gets a home again -- the strict gate stays meaningless until its baseline comes from a
//     machine worth trusting. Flagged explicitly, not silently left as if it were the real
//     thing.
//
//     A REAL FINDING FROM THIS MEASUREMENT, A REVERTED OPTIMIZATION, AND THE CEILING REVISED BY
//     THE LEADER (reported, not hidden -- "recomendações de tuning com evidência" is this role's
//     own deliverable): the ratio gate (b) measures ~3.4-3.6x on this machine, consistently,
//     across repeated process launches -- above the 2.0x ceiling D30 first set. Root cause
//     isolated with a standalone probe (this wave's own report has the breakdown):
//     `LayerQueue::drain_grouped()`'s `std::stable_sort` moves whole ~96-byte `LayerCommand`
//     structs (D27's own size estimate) to reorder by a single `int layer` key, dominating the
//     layered path's own cost (roughly 60% of it in the probe) -- `draw2d.cpp`'s own
//     `Impl::replay_layer_queue()` runs the EXACT same `drain_grouped()` call this benchmark does,
//     so this is not a benchmark artifact. A LIGHTWEIGHT-KEY SORT WAS TRIED AND REVERTED
//     (PERF-D2D3B, 2026-07-23): a version of `drain_grouped()` that sorted a `{layer, push_index}`
//     key (8 B) instead of the full struct measured ~2.9x on THIS machine -- but that measurement
//     used the system's default g++, a METHOD ERROR: every CI workflow that builds this repo
//     (`.github/workflows/ci.yml`) pins
//     `-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++`, never the default compiler. Rebuilt
//     and re-measured with clang (the only comparison that matters): the lightweight-key sort
//     measured ~3.69x, WORSE than the original `std::stable_sort`'s ~3.39x under clang -O0's own
//     codegen for this comparator shape -- the "cheaper key" theory held under gcc, not clang. The
//     leader's call (D30 owner, 2026-07-23): REVERT to the plain `std::stable_sort` (this file's
//     current form) rather than ship a change that regresses on the compiler CI actually runs, and
//     REVISE THE CEILING instead -- layered mode is OPT-IN by construction (buffer + sort +
//     scissor-regroup + replay is inherently costlier than the untouched streaming path, which
//     this gate leaves completely alone), so ~3.4x measured WITH CLANG is the honest price of that
//     feature, not a regression to chase further. The ceiling is REVISED to 4.0x -- the measured
//     ~3.4x (clang) plus margin against flaking -- while still catching a GRAVE regression (e.g. a
//     further-degraded sort, north of 4x) without flaking at a borderline ~3.5x-3.7x reading. This
//     gate stays unconditional, machine-relative, and always-on exactly as D30 originally
//     specified -- only the numeric ceiling moved, and it moved based on a clang measurement, not
//     a gcc one.
//
//     THE CEILING, RE-DERIVED FROM MEASUREMENT 2026-07-25 (kRatioGateMax = 3.9, superseding the
//     4.0 of the paragraph above, which belonged to the old wall-clock median-of-8 statistic and
//     does not transfer): with the METHOD above, the WORST reading over 30 process launches x 63
//     samples spanning idle, in-cgroup contention and host contention was 3.558, and the idle
//     median was 3.351. The ceiling is that worst reading plus a declared 10% margin, rounded
//     down to 3.9. What the margin covers: the cross-launch spread that no amount of repeating
//     removes, because a whole process can land on a slow core (the measurement host is a hybrid
//     P-core/E-core part) and both arms are then measured there. What the gate therefore trips
//     on: any change that makes the layered path more than about 16% slower relative to
//     streaming (3.9 / 3.351). Proven to bite, not assumed (mutation testing, same build, three
//     runs each): a deliberate regression in LayerQueue::drain_grouped() that runs the SAME
//     std::stable_sort a second, redundant time took the gate from 3.342/3.365/3.495 PASS to
//     4.185/4.199/4.357 FAIL, three failures out of three, and restoring the header put it back
//     at 3.412/3.456/3.468 PASS. The declared sensitivity LIMIT, measured in the same session: a
//     milder regression (copying the command vector instead of moving it, about +6%) reads
//     3.459/3.571/3.729 and does NOT fail. This gate is a guard against a GRAVE regression, never
//     a microbenchmark of single-digit drift, exactly as D30 framed it. Proven not to flake, not
//     assumed: with 16 CPU burners saturating the host outside the container the gate read 2.723
//     to 3.102 over four runs, and with 6 further burners inside the container's own 4-CPU quota
//     it read 2.978 to 3.336 over three, all PASS -- while the wall-clock diagnostic printed
//     alongside it, which is the OLD gated statistic, read 4.397 to 4.820 in those very same
//     three runs. The old gate would have failed all three. Every number here comes from this
//     file as it now stands, not from the probe.
//
// PT: draw2d_perf_budget (PERF-D2D3, D30) -- números de performance declarados para o átomo
//     Draw2D do glintfx, uma preocupação DIFERENTE da suíte de correção adversarial do
//     `qa-engineer` (o trabalho deste arquivo é medição, não cobertura fail-high/input hostil --
//     ver o mandato do `performance-engineer` vs o do `qa-engineer`). Plano
//     docs/superpowers/plans/2026-07-23-onda5-draw2d-primitives.md seção 5 item 5, D30. Três
//     métricas, impressas como linhas `GLINTFX_PERF <chave>=<valor>` machine-readable toda
//     execução (visíveis no log de CI independente do resultado do gate):
//       (a) pure_batcher_quads_per_s  -- 50k quads pelo SpriteBatch sozinho (CPU, sem display,
//           sem LayerQueue envolvida) -- o teto do próprio batcher, reportado a partir da
//           repetição MAIS RÁPIDA de relógio de parede (ver o parágrafo METHOD abaixo pra
//           entender por que um melhor-de-N e não uma mediana).
//       (b) layered_streaming_ratio   -- o MESMO stream de 50k comandos pelo caminho
//           bufferizado (LayerQueue::push -> drain_grouped() -> replay via
//           SpriteBatch::draw_quad(), a MESMA sequência que o próprio laço de replay de
//           draw2d.cpp roda no end()), reportado como time_layered / time_streaming (>= 1.0 --
//           o caminho bufferizado só pode ADICIONAR trabalho, reusa o MESMO batcher por baixo, o
//           conjunto zero-diff do D31), cada braço tomado na própria repetição mais rápida de
//           TEMPO DE CPU DA THREAD (parágrafo METHOD abaixo).
//       (c) e2e_10k_sprite_1k_prim_median_ms -- um bracket real de Draw2d (10k sprites, uma
//           textura) mais um bracket real de Draw2d (1k primitivas draw_filled_rect), os dois sob
//           um contexto GL 3.3 vivo no Xvfb/llvmpipe, com glFinish() antes do cronômetro parar
//           (mesma disciplina do próprio --axis=frame de fonteng_perf_bench.cpp) -- mediana sobre
//           30 frames (2 primeiros descartados como aquecimento, mesma convenção "descarte
//           frames de cache frio" de todo laço cronometrado daquele arquivo).
//
//     DOWNGRADES, DECLARADOS (não escondidos -- a própria disciplina desta casa, o próprio texto
//     do D30): o llvmpipe rasteriza na CPU -- a métrica (c) é uma GUARDA DE REGRESSÃO DE RASTER
//     DE CPU, não uma verdade de throughput/banda/pacing-de-vsync de GPU (mesma ressalva que o
//     próprio comentário do golden_test em tests/CMakeLists.txt declara pros testes
//     pixel-exatos). As métricas (a)/(b) SÃO números reais de CPU (nenhum GL envolvido --
//     SpriteBatch/LayerQueue são headers puros). A métrica (c) usa UMA textura só pro bracket de
//     10k sprites (sem stream de troca-de-textura) -- o custo de pior-caso com intercalação
//     multi-textura é um eixo DIFERENTE, não medido aqui (semente SEED-PERF-TEXSWITCH se algum
//     dia precisar de preço).
//
//     MÉTODO DO GATE DE RAZÃO, REESCRITO EM 2026-07-25 DEPOIS DE UMA FALHA REAL DE CI
//     (PERF-D2D3; a decisão do líder foi "consertar o método", não "subir o teto"): o gate leu
//     4,482 no runner self-hosted e deixou a main vermelha, enquanto 12 execuções limpas do MESMO
//     código no MESMO container mediram de 3,592 a 3,806 (mediana 3,721, desvio 0,064). O runner
//     divide uma máquina física com o trabalho interativo do dono, então a disputa por CPU é
//     ESTRUTURAL ali e vai se repetir. Repare que a versão que falhou JÁ tirava a mediana de 8
//     razões pareadas por repetição: mediana filtra PICOS isolados, e isto não era um pico, então
//     só aumentar o N não resolve nada. Medido aqui (sonda replicando os dois braços verbatim, 30
//     lançamentos de processo em três condições de disputa -- máquina ociosa, 6 queimadores de
//     CPU dentro do mesmo cgroup de 4 CPUs, e 16 queimadores de CPU no host fora dele):
//       - mediana das razões pareadas no relógio de PAREDE (a versão que falhou): de 2,23 a 7,80.
//         O pareamento em si é o defeito. Cada repetição cronometra os dois braços em SEQUÊNCIA,
//         então uma rajada que cai em UM dos braços distorce a razão daquela repetição na direção
//         em que bateu (as amostras por repetição sob carga, agrupadas, foram de 0,32 a 23,08).
//       - mínimo por braço no relógio de CPU DA THREAD (o que este arquivo faz agora): de 2,965 a
//         3,558 nos 30 lançamentos, com as distribuições OCIOSA e SOB CARGA se sobrepondo.
//     Duas mudanças independentes compram isso, e as duas são estruturais:
//       1. CLOCK_THREAD_CPUTIME_ID no lugar de steady_clock. Os dois braços são CPU pura de uma
//          thread só, sem I/O e sem dormir, então ser tirado do processador é puro erro de
//          medição; o relógio de CPU simplesmente não conta isso.
//       2. O MÍNIMO de cada braço ao longo das repetições, tomado por braço de forma
//          INDEPENDENTE, no lugar de uma mediana de razões pareadas. Disputa só pode ADICIONAR
//          tempo, nunca tirar, então a repetição mais rápida é a melhor estimativa do custo sem
//          disputa -- a disciplina padrão de melhor-de-N para microbenchmark sob ruído. Por braço
//          e não por par, porque os dois braços não podem ser atingidos pela mesma rajada ao
//          mesmo tempo.
//     kAbRepeats = 64 (a primeira descartada como aquecimento, 63 amostras por braço) é medido,
//     não chutado: subamostrando esses mesmos lançamentos, o estimador precisa de cerca de 48
//     repetições pra convergir (pior leitura 5,31 em N=16, 4,98 em N=32, e então 3,56 tanto em
//     N=48 quanto em N=63), porque uma rajada de disputa pode cobrir um segundo inteiro de
//     repetições e o mínimo precisa de pelo menos uma janela limpa. 64 é a potência de dois
//     seguinte ao joelho da convergência. kAbCommands = 50000 em vez dos 100000 originais pela
//     mesma razão de robustez, também medida: em 100k o braço em camadas segura ~9,6 MiB de
//     LayerCommand e um processo vizinho o expulsa de um L3 compartilhado, o que custa ciclos de
//     CPU reais que o relógio de CPU não tem como filtrar (o mínimo por braço em 100k ainda
//     derivou pra 4,115 sob carga, contra 3,520 em 50k). A métrica (a) NÃO é distorcida pelo
//     tamanho menor: o braço streaming mede 9,36 M quads/s em 50k contra 9,32 M em 100k, dentro
//     da dispersão de execução pra execução. A métrica (b) DEPENDE do tamanho por construção (a
//     ordenação é n log n enquanto o resto dos dois braços é linear), então o teto abaixo só faz
//     sentido pro kAbCommands como está; mudar essa constante exige rederivar o teto.
//     CUSTO, DECLARADO: o bloco (a)+(b) sai de ~0,4 s para ~1,6 s de relógio de parede, então o
//     teste inteiro sai de ~3,8 s para ~5,0 s no container de CI. É o preço de o gate não flakear,
//     pago uma vez por execução de CI.
//
//     O GATE, DOIS NÍVEIS (D30, literal): com GLINTFX_PERF_STRICT=1 no ambiente, as métricas (a)
//     e (c) FALHAM se cruzarem a própria constante BASELINE abaixo em mais de 1,5x (direção
//     pior: quads/s mais baixo pra (a), ms mais alto pra (c)). Sem a env, só um teto de SANIDADE
//     grosseiro de 10x falha o teste -- o número é sempre impresso dos dois jeitos.
//     !! DESDE 2026-07-25, NADA SETA GLINTFX_PERF_STRICT=1: o único job que a armava rodava no
//     runner self-hosted, aposentado junto com o diretório .forgejo/ inteiro. O nível estrito
//     está DORMENTE, não removido -- sete a variável à mão numa máquina cujas baselines você
//     confia. Ver a seção de política de CI do AGENTS.md pra pendência que lhe dá casa de novo. O gate de RATIO (b, camadas <= 3,9x streaming, rederivado
//     em 2026-07-25 -- ver o parágrafo CEILING abaixo) é RELATIVO-À-MÁQUINA por construção (os
//     dois braços rodam na MESMA máquina, MESMO processo, MESMA invocação) então é ESTRITO EM
//     TODO LUGAR, incondicional a GLINTFX_PERF_STRICT -- é a resposta direta e sempre-ligada ao
//     risco 2 desta onda ("layers vs batching é onde performance pode degradar", plano seção 0).
//
//     PROVENIÊNCIA DA BASELINE (honestidade sobre invenção -- a própria regra nomeada da casa:
//     "baseline medida, nunca inventada"): as três constantes abaixo foram MEDIDAS por este
//     agente em 2026-07-23, no container de dev sandboxed ao qual esta sessão teve acesso de
//     shell (Fedora 44, Intel Core i5-12500H de 12ª geração, 16 núcleos lógicos, 31 GiB RAM) --
//     NÃO o runner self-hosted nomeado no brief (uma máquina física diferente que este processo
//     de agente não alcança). É uma medição REAL, nunca um chute, mas é uma baseline PROVISÓRIA:
//     pelo D30 o líder/orquestrador deveria trocar estas três constantes pelos números que uma
//     máquina dedicada e estável imprimir (um diff de uma linha cada, data/host deste comentário
//     atualizados). Essa máquina era o runner self-hosted aposentado em 2026-07-25, então a troca
//     está BLOQUEADA até o job pesado ganhar casa de novo -- o gate estrito segue não
//     significando nada até a baseline vir de uma máquina confiável. Flagrado explicitamente,
//     não deixado em silêncio como se fosse a coisa real.
//
//     UM ACHADO REAL DESTA MEDIÇÃO, UMA OTIMIZAÇÃO REVERTIDA, E O TETO REVISADO PELO LÍDER
//     (reportado, não escondido -- "recomendações de tuning com evidência" é entrega própria deste
//     papel): o gate de razão (b) mede ~3,4-3,6x nesta máquina, de forma consistente, em vários
//     lançamentos de processo -- acima do teto de 2,0x que o D30 fixava a princípio. Causa-raiz
//     isolada com uma sonda avulsa (o próprio relatório desta onda tem a quebra): o
//     `std::stable_sort` do `LayerQueue::drain_grouped()` move structs `LayerCommand` inteiras de
//     ~96 bytes (a própria estimativa de tamanho do D27) pra reordenar por uma única chave
//     `int layer`, dominando o custo do próprio caminho em camadas (cerca de 60% dele na sonda) --
//     o próprio `Impl::replay_layer_queue()` de `draw2d.cpp` roda a MESMA chamada
//     `drain_grouped()` que este benchmark roda, então isto não é um artefato de benchmark. UMA
//     ORDENAÇÃO DE CHAVE LEVE FOI TENTADA E REVERTIDA (PERF-D2D3B, 2026-07-23): uma versão do
//     `drain_grouped()` que ordenava uma chave `{layer, push_index}` (8 B) em vez da struct
//     inteira mediu ~2,9x NESTA máquina -- mas essa medição usou o g++ padrão do sistema, um ERRO
//     DE MÉTODO: todo workflow de CI que builda este repo (`.github/workflows/ci.yml`,
//     ) fixa `-DCMAKE_C_COMPILER=clang
//     -DCMAKE_CXX_COMPILER=clang++`, nunca o compilador padrão. Rebuildado e re-medido com clang
//     (a única comparação que importa): a ordenação de chave leve mediu ~3,69x, PIOR que o
//     `std::stable_sort` original (~3,39x sob clang) -- a teoria da "chave mais barata" se
//     sustentava sob gcc, não sob clang, pro próprio codegen -O0 dessa forma de comparador. A
//     decisão do líder (dono do D30, 2026-07-23): REVERTER pro `std::stable_sort` simples (a forma
//     atual deste arquivo) em vez de subir uma mudança que regride no compilador que o CI de fato
//     roda, e REVISAR O TETO em vez disso -- o modo em camadas é OPT-IN por construção
//     (bufferizar + ordenar + reagrupar-por-scissor + reproduzir é inerentemente mais custoso que
//     o caminho streaming intocado, que este gate deixa completamente de lado), então ~3,4x medido
//     COM CLANG é o preço honesto dessa feature, não uma regressão a caçar mais. O teto é REVISADO
//     para 4,0x -- o ~3,4x medido (clang) mais margem contra flakiness -- ainda capturando uma
//     regressão GRAVE (ex.: uma ordenação ainda mais degradada, acima de 4x) sem flakear numa
//     leitura limítrofe perto de ~3,5x-3,7x. Este gate segue incondicional, relativo-à-máquina e
//     sempre-ligado exatamente como o D30 especificava originalmente -- só o teto numérico mudou,
//     e mudou com base numa medição clang, não gcc.
//
//     O TETO, REDERIVADO DA MEDIÇÃO EM 2026-07-25 (kRatioGateMax = 3,9, sucedendo o 4,0 do
//     parágrafo acima, que pertencia à antiga estatística de mediana-de-8 no relógio de parede e
//     não se transfere): com o MÉTODO acima, a PIOR leitura em 30 lançamentos de processo x 63
//     amostras cobrindo máquina ociosa, disputa dentro do cgroup e disputa no host foi 3,558, e a
//     mediana ociosa foi 3,351. O teto é essa pior leitura mais uma margem declarada de 10%,
//     arredondada pra baixo em 3,9. O que a margem cobre: a dispersão entre lançamentos que
//     repetição nenhuma remove, porque um processo inteiro pode cair num núcleo lento (a máquina
//     de medição é um chip híbrido de núcleos P e E) e aí os dois braços são medidos ali. No que
//     o gate portanto morde: qualquer mudança que deixe o caminho em camadas mais de uns 16% mais
//     lento em relação ao streaming (3,9 / 3,351). Provado que morde, não presumido (mutation
//     testing, mesmo build, três execuções cada): uma regressão deliberada no
//     LayerQueue::drain_grouped() que roda o MESMO std::stable_sort uma segunda vez, redundante,
//     levou o gate de 3,342/3,365/3,495 PASSA para 4,185/4,199/4,357 FALHA, três falhas em três,
//     e restaurar o header o devolveu a 3,412/3,456/3,468 PASSA. O LIMITE de sensibilidade
//     declarado, medido na mesma sessão: uma regressão mais branda (copiar o vetor de comandos em
//     vez de movê-lo, cerca de +6%) lê 3,459/3,571/3,729 e NÃO falha. Este gate é guarda contra
//     regressão GRAVE, nunca microbenchmark de deriva de um dígito, exatamente como o D30 o
//     enquadrou. Provado que não flakeia, não presumido: com 16 queimadores de CPU saturando o
//     host fora do container o gate leu de 2,723 a 3,102 em quatro execuções, e com mais 6
//     queimadores dentro da própria cota de 4 CPUs do container leu de 2,978 a 3,336 em três,
//     todas PASSA -- enquanto o diagnóstico de relógio de parede impresso ao lado, que é a
//     estatística ANTIGA do gate, leu de 4,397 a 4,820 nessas mesmas três execuções. O gate
//     antigo teria falhado as três. Todo número aqui vem deste arquivo como ele está agora, não
//     da sonda.
// Copyright (c) 2026 Petrus Silva Costa
#include "../src/layer_queue.hpp"
#include "../src/sprite_batch.hpp"
#include "../src/window_glfw.hpp"
#include <glintfx/glintfx.hpp>
#include "gl_loader.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>

namespace fs = std::filesystem;
using glintfx::ColorF;
using glintfx::Draw2d;
using glintfx::RectF;
using glintfx::Texture2d;
using glintfx::Vec2F;
using glintfx::draw2d_detail::Flush;
using glintfx::draw2d_detail::LayerCommand;
using glintfx::draw2d_detail::LayerGroup;
using glintfx::draw2d_detail::LayerQueue;
using glintfx::draw2d_detail::ScissorSnapshot;
using glintfx::draw2d_detail::SpriteBatch;
using glintfx::draw2d_detail::SpriteCorners;

namespace {

using Clock = std::chrono::steady_clock;

double median_of(std::vector<double> v) {
  if (v.empty()) return 0.0;
  std::sort(v.begin(), v.end());
  const std::size_t n = v.size();
  return (n % 2) ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

// EN: Seconds of CPU time burned by THIS thread, the clock the ratio gate is measured on (see the
//     METHOD paragraph in this file's header comment: descheduling under a contended machine is
//     measurement error for a workload that is pure single-threaded CPU with no I/O and no
//     sleeping, and this clock does not count it). POSIX-only, like this file already is
//     elsewhere (mkdtemp, /var/tmp) -- no test target here is ever built for Windows.
// PT: Segundos de tempo de CPU queimados por ESTA thread, o relógio em que o gate de razão é
//     medido (ver o parágrafo METHOD no comentário de cabeçalho deste arquivo: ser tirado do
//     processador numa máquina disputada é erro de medição para uma carga que é CPU pura de uma
//     thread só, sem I/O e sem dormir, e este relógio não o contabiliza). Só POSIX, como este
//     arquivo já é em outros pontos (mkdtemp, /var/tmp) -- nenhum alvo de teste daqui é buildado
//     para Windows.
double cpu_seconds_now() {
  timespec ts{};
  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0) return 0.0;
  return static_cast<double>(ts.tv_sec) + 1e-9 * static_cast<double>(ts.tv_nsec);
}

// EN: One timed repeat of one arm, on both clocks. The gate reads `cpu_s`; `wall_s` feeds the
//     printed throughput of metric (a) (quads per second is a wall-time quantity a consumer
//     cares about) and the printed wall-clock diagnostic.
// PT: Uma repetição cronometrada de um braço, nos dois relógios. O gate lê `cpu_s`; `wall_s`
//     alimenta o throughput impresso da métrica (a) (quads por segundo é uma grandeza de tempo
//     de relógio que interessa a quem consome) e o diagnóstico impresso de relógio de parede.
struct Timing {
  double wall_s = 0.0;
  double cpu_s = 0.0;
};

// EN: (a) -- `n` quads (kAbCommands, 50k) through SpriteBatch ALONE (no LayerQueue), draining
//     `take_ready()` after EVERY draw -- the SAME discipline `sprite_batch_sanity.cpp`'s own
//     hostile-100k test and `draw2d.cpp` itself follow (sprite_batch.hpp's own header comment:
//     the ready queue is unbounded unless the caller drains it). Returns ELAPSED TIME on both
//     clocks, not quads/s, so this and bench_layered() below can be divided directly into a time
//     RATIO (metric b) -- the caller derives quads/s for the printed metric (a) from the wall
//     half of this same number.
// PT: (a) -- `n` quads (kAbCommands, 50k) pelo SpriteBatch SOZINHO (sem LayerQueue), drenando
//     `take_ready()` após TODO desenho -- a MESMA disciplina que o próprio teste hostil-100k de
//     `sprite_batch_sanity.cpp` e o próprio `draw2d.cpp` seguem (comentário de cabeçalho do
//     sprite_batch.hpp: a fila pronta é ilimitada a menos que o chamador a drene). Devolve TEMPO
//     decorrido nos dois relógios, não quads/s, para que isto e o bench_layered() abaixo possam
//     ser divididos direto numa RAZÃO de tempo (métrica b) -- o chamador deriva quads/s pra
//     métrica impressa (a) a partir da metade de relógio de parede deste mesmo número.
Timing bench_pure_batcher(std::size_t n) {
  SpriteBatch batch;
  const RectF dst{0.f, 0.f, 1.f, 1.f};
  const double c0 = cpu_seconds_now();
  const auto t0 = Clock::now();
  batch.begin(1920, 1080);
  for (std::size_t i = 0; i < n; ++i) {
    batch.draw_sprite(1, 4, 4, dst, RectF{}, ColorF{});
    for (const Flush& f : batch.take_ready()) (void)f;
  }
  batch.end();
  for (const Flush& f : batch.take_ready()) (void)f;
  Timing t;
  t.wall_s = std::chrono::duration<double>(Clock::now() - t0).count();
  t.cpu_s = cpu_seconds_now() - c0;
  return t;
}

// EN: (b) -- the SAME `n`-command stream, through LayerQueue::push() -> drain_grouped() ->
//     SpriteBatch::draw_quad() replay -- the EXACT sequence draw2d.cpp's own end()-time replay
//     loop runs (push already-projected corners, drain grouped by scissor, replay each group's
//     commands through the untouched batcher, draining after every draw_quad() the same way (a)
//     does). `layer = i % 16` gives the sort real (non-trivial) work to do -- a constant-layer
//     stream would let it degenerate into a near-no-op and understate this mode's real cost. Same
//     "elapsed TIME on both clocks, not a rate" return convention as (a) above.
// PT: (b) -- o MESMO stream de `n` comandos, por LayerQueue::push() -> drain_grouped() -> replay
//     via SpriteBatch::draw_quad() -- a MESMA sequência que o próprio laço de replay no end() de
//     draw2d.cpp roda (empurra cantos já projetados, drena agrupado por scissor, reproduz os
//     comandos de cada grupo pelo batcher intocado, drenando após todo draw_quad() do mesmo jeito
//     que (a) faz). `layer = i % 16` dá trabalho real (não-trivial) pra ordenação fazer -- um
//     stream de camada constante deixaria a ordenação degenerar num quase-no-op e subestimaria o
//     custo real deste modo. Mesma convenção de retorno "TEMPO decorrido nos dois relógios, não
//     uma taxa" de
//     (a) acima.
Timing bench_layered(std::size_t n) {
  LayerQueue queue;
  SpriteBatch batch;
  const SpriteCorners corners{Vec2F{0.f, 0.f}, Vec2F{1.f, 0.f}, Vec2F{1.f, 1.f}, Vec2F{0.f, 1.f}};
  const double c0 = cpu_seconds_now();
  const auto t0 = Clock::now();
  batch.begin(1920, 1080);
  for (std::size_t i = 0; i < n; ++i) {
    LayerCommand cmd;
    cmd.corners = corners;
    cmd.texture_id = 1;
    cmd.tex_w = 4;
    cmd.tex_h = 4;
    cmd.src_px = RectF{};
    cmd.tint = ColorF{};
    cmd.scissor = ScissorSnapshot{};
    cmd.layer = static_cast<int>(i % 16);
    queue.push(cmd);
  }
  const std::vector<LayerGroup> groups = queue.drain_grouped();
  for (const LayerGroup& g : groups) {
    for (const LayerCommand& cmd : g.commands) {
      const Vec2F draw_corners[4] = {cmd.corners.tl, cmd.corners.tr, cmd.corners.br, cmd.corners.bl};
      batch.draw_quad(cmd.texture_id, cmd.tex_w, cmd.tex_h, draw_corners, cmd.src_px, cmd.tint);
      for (const Flush& f : batch.take_ready()) (void)f;
    }
  }
  batch.end();
  for (const Flush& f : batch.take_ready()) (void)f;
  Timing t;
  t.wall_s = std::chrono::duration<double>(Clock::now() - t0).count();
  t.cpu_s = cpu_seconds_now() - c0;
  return t;
}

// EN: Hand-crafted, uncompressed 24bpp TGA -- same fixture idiom as draw2d_render_sanity.cpp
//     (deliberately NOT a stb_image-vendored PNG, whose own values are an opaque detail this file
//     does not care about; a single solid colour is all metric (c) needs). Reused for every one
//     of the 10k sprites in the bracket below -- ONE texture, no texture-switch flush cost (the
//     declared downgrade in this file's own header comment).
// PT: TGA 24bpp não-comprimido feito à mão -- mesmo idioma de fixture do draw2d_render_sanity.cpp
//     (deliberadamente NÃO um PNG vendorizado via stb_image, cujos próprios valores são um
//     detalhe opaco que este arquivo não precisa). Reusado por cada um dos 10k sprites do bracket
//     abaixo -- UMA textura só, sem custo de flush por troca-de-textura (o downgrade declarado no
//     próprio comentário de cabeçalho deste arquivo).
bool write_solid_tga(const fs::path& path, int w, int h, unsigned char r, unsigned char g,
                     unsigned char b) {
  std::vector<unsigned char> f;
  auto push16 = [&](int v) {
    f.push_back(static_cast<unsigned char>(v & 0xFF));
    f.push_back(static_cast<unsigned char>((v >> 8) & 0xFF));
  };
  f.push_back(0);
  f.push_back(0);
  f.push_back(2); // uncompressed true-colour
  push16(0);
  push16(0);
  f.push_back(0);
  push16(0);
  push16(0);
  push16(w);
  push16(h);
  f.push_back(24);
  f.push_back(0x20); // top-down.
  for (int i = 0; i < w * h; ++i) {
    f.push_back(b);
    f.push_back(g);
    f.push_back(r);
  }
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) return false;
  out.write(reinterpret_cast<const char*>(f.data()), static_cast<std::streamsize>(f.size()));
  return out.good();
}

// EN: (c) -- real Draw2d brackets under a live GL 3.3 context (Xvfb/llvmpipe): 10k sprites (one
//     texture, scattered across the viewport so llvmpipe cannot degenerate the whole bracket into
//     one fully-overdrawn pixel column) followed by 1k draw_filled_rect primitives, glFinish()'d
//     before the clock stops (the same "force the CPU-rasterized work to actually complete before
//     timing" discipline fonteng_perf_bench.cpp's own --axis=frame uses). Returns the median of
//     `frames - warmup` samples; `-1.0` on any setup failure (host/init/texture), which the
//     caller treats as "metric (c) could not be measured", not a silent zero.
// PT: (c) -- brackets reais de Draw2d sob um contexto GL 3.3 vivo (Xvfb/llvmpipe): 10k sprites
//     (uma textura, espalhados pelo viewport para que o llvmpipe não degenere o bracket inteiro
//     numa única coluna de pixel totalmente sobre-desenhada) seguidos de 1k primitivas
//     draw_filled_rect, com glFinish() antes do cronômetro parar (a mesma disciplina "força o
//     trabalho rasterizado em CPU a de fato terminar antes de cronometrar" que o próprio
//     --axis=frame de fonteng_perf_bench.cpp usa). Devolve a mediana de `frames - warmup`
//     amostras; `-1.0` em qualquer falha de setup (host/init/textura), que o chamador trata como
//     "métrica (c) não pôde ser medida", não um zero silencioso.
double bench_e2e_median_ms(int frames, int warmup) {
  constexpr int W = 512, H = 512;
  constexpr int kSprites = 10000;
  constexpr int kPrimitives = 1000;

  glintfx::WindowGlfw host;
  if (!host.create("draw2d_perf_budget_host", W, H)) {
    std::fprintf(stderr, "draw2d_perf_budget: host window create failed (metric c skipped)\n");
    return -1.0;
  }

  Draw2d d2d;
  if (!d2d.init()) {
    std::fprintf(stderr, "draw2d_perf_budget: Draw2d::init() failed (metric c skipped)\n");
    return -1.0;
  }

  char tmpl[] = "/var/tmp/glintfx_draw2d_perf_XXXXXX";
  char* dir_c = mkdtemp(tmpl);
  if (!dir_c) {
    std::fprintf(stderr, "draw2d_perf_budget: mkdtemp failed (metric c skipped)\n");
    d2d.shutdown();
    return -1.0;
  }
  const fs::path dir(dir_c);
  const fs::path tex_path = dir / "solid.tga";
  write_solid_tga(tex_path, 4, 4, 200, 100, 50);

  Texture2d tex = d2d.load_texture(tex_path.c_str());
  if (!tex.ok()) {
    std::fprintf(stderr, "draw2d_perf_budget: fixture texture decode failed (metric c skipped)\n");
    d2d.shutdown();
    std::error_code ec;
    fs::remove_all(dir, ec);
    return -1.0;
  }

  glViewport(0, 0, W, H);

  std::vector<double> frame_ms;
  frame_ms.reserve(static_cast<std::size_t>(frames));
  for (int i = 0; i < frames; ++i) {
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    const auto t0 = Clock::now();

    d2d.begin(W, H);
    for (int j = 0; j < kSprites; ++j) {
      const float x = static_cast<float>((j * 7) % (W - 4));
      const float y = static_cast<float>((j * 13) % (H - 4));
      d2d.draw_sprite(tex, RectF{x, y, 4.f, 4.f});
    }
    d2d.end();

    d2d.begin(W, H);
    for (int j = 0; j < kPrimitives; ++j) {
      const float x = static_cast<float>((j * 11) % (W - 6));
      const float y = static_cast<float>((j * 17) % (H - 6));
      d2d.draw_filled_rect(RectF{x, y, 6.f, 6.f}, ColorF{0.2f, 0.6f, 0.9f, 0.8f});
    }
    d2d.end();

    glFinish();
    const double dt = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
    if (i >= warmup) frame_ms.push_back(dt);
  }

  d2d.shutdown();
  std::error_code ec;
  fs::remove_all(dir, ec);

  return median_of(frame_ms);
}

bool env_strict_perf() {
  const char* v = std::getenv("GLINTFX_PERF_STRICT");
  return v != nullptr && std::strcmp(v, "1") == 0;
}

// EN: `kAbRepeats` runs of (a)+(b) BACK-TO-BACK per repeat, discarding the FIRST as warm-up
//     (allocator growth to the vertex/command storage is a one-time cost, same "discard
//     cold-cache frames" convention fonteng_perf_bench.cpp uses). The gate's RATIO is
//     min(layered CPU time) / min(streaming CPU time), each arm minimised INDEPENDENTLY. The full
//     rationale, the measurements behind kAbRepeats = 64 and kAbCommands = 50000, and why the
//     previous statistic (median of paired WALL-clock ratios, which reddened main at 4.482) was
//     replaced live in the METHOD paragraph of this file's header comment -- read that before
//     touching any constant here. The paired median is still COMPUTED and PRINTED as a
//     diagnostic, never gated on: when the two disagree, the machine was contended while the
//     test ran, which is worth seeing in a CI log.
// PT: `kAbRepeats` execuções de (a)+(b) NAS COSTAS UMA DA OUTRA por repetição, descartando a
//     PRIMEIRA como aquecimento (o crescimento do alocador até o armazenamento de vértice/comando
//     é um custo de uma vez só, mesma convenção "descarte frames de cache frio" que o
//     fonteng_perf_bench.cpp usa). A RAZÃO do gate é min(tempo de CPU em camadas) / min(tempo de
//     CPU em streaming), cada braço minimizado de forma INDEPENDENTE. O racional completo, as
//     medições por trás de kAbRepeats = 64 e kAbCommands = 50000, e por que a estatística
//     anterior (mediana das razões PAREADAS de relógio de parede, que deixou a main vermelha em
//     4,482) foi substituída estão no parágrafo METHOD do comentário de cabeçalho deste arquivo
//     -- leia antes de mexer em qualquer constante daqui. A mediana pareada continua sendo
//     CALCULADA e IMPRESSA como diagnóstico, nunca gateada: quando as duas discordam, a máquina
//     estava disputada enquanto o teste rodava, e isso vale ser visto num log de CI.
constexpr int kAbRepeats = 64;
constexpr std::size_t kAbCommands = 50000;

struct AbResult {
  double streaming_qps = 0.0;      // metric (a), from the FASTEST streaming wall time.
  double layered_ratio = 0.0;      // metric (b), the GATED number: min CPU layered / min CPU streaming.
  double diag_paired_median = 0.0; // diagnostic only: median of paired per-repeat WALL ratios.
  int samples = 0;                 // repeats that actually counted (kAbRepeats minus the warm-up).
};

AbResult bench_ab(std::size_t n) {
  const double inf = std::numeric_limits<double>::infinity();
  double min_stream_wall = inf, min_stream_cpu = inf;
  double min_layered_cpu = inf;
  std::vector<double> paired_wall_ratios;
  paired_wall_ratios.reserve(static_cast<std::size_t>(kAbRepeats));

  for (int i = 0; i < kAbRepeats; ++i) {
    const Timing stream = bench_pure_batcher(n);
    const Timing layered = bench_layered(n);
    if (i == 0) continue; // warm-up repeat, discarded (allocator growth, see comment above).
    min_stream_wall = std::min(min_stream_wall, stream.wall_s);
    min_stream_cpu = std::min(min_stream_cpu, stream.cpu_s);
    min_layered_cpu = std::min(min_layered_cpu, layered.cpu_s);
    if (stream.wall_s > 0.0) paired_wall_ratios.push_back(layered.wall_s / stream.wall_s);
  }

  AbResult out;
  out.samples = static_cast<int>(paired_wall_ratios.size());
  out.streaming_qps = (min_stream_wall > 0.0 && min_stream_wall < inf)
                          ? static_cast<double>(n) / min_stream_wall
                          : 0.0;
  out.layered_ratio =
      (min_stream_cpu > 0.0 && min_stream_cpu < inf) ? min_layered_cpu / min_stream_cpu : 0.0;
  out.diag_paired_median = median_of(paired_wall_ratios);
  return out;
}

} // namespace

int main() {
  int failures = 0;
  const bool strict = env_strict_perf();

  // EN: BASELINES (see this file's own header comment for full provenance/caveat) -- measured on
  //     2026-07-23, sandboxed dev container, Fedora 44, Intel Core i5-12500H (12th gen, 16
  //     logical cores), 31 GiB RAM. NOT a dedicated stable machine -- STAND-IN until one exists
  //     again and produces real numbers to swap in (D30); the self-hosted runner that was going
  //     to do it was retired on 2026-07-25.
  // PT: BASELINES (ver o comentário de cabeçalho deste arquivo pra proveniência/ressalva
  //     completas) -- medidas em 2026-07-23, container de dev sandboxed, Fedora 44, Intel Core
  //     i5-12500H (12ª geração, 16 núcleos lógicos), 31 GiB RAM. NÃO uma máquina dedicada e
  //     estável -- PROVISÓRIA até existir uma de novo que produza números reais para substituir
  //     (D30); o runner self-hosted que faria isso foi aposentado em 2026-07-25.
  constexpr double kBaselinePureBatcherQuadsPerS = 8500000.0;
  constexpr double kBaselineE2eMedianMs = 5.0;
  constexpr double kStrictFactor = 1.5;
  constexpr double kSanityFactor = 10.0;
  // EN: 3.9 = worst reading measured over 30 process launches spanning idle, in-cgroup and host
  //     contention (3.558) plus a declared 10% margin. Derivation, margin rationale and the
  //     mutation/under-load proofs are in the CEILING paragraph of this file's header comment.
  //     Only valid for kAbCommands as it stands -- the ratio is workload-size dependent.
  // PT: 3,9 = pior leitura medida em 30 lançamentos de processo cobrindo máquina ociosa,
  //     disputa dentro do cgroup e disputa no host (3,558) mais uma margem declarada de 10%. A
  //     derivação, o racional da margem e as provas de mutação/sob-carga estão no parágrafo
  //     CEILING do comentário de cabeçalho deste arquivo. Só vale para o kAbCommands como está
  //     -- a razão depende do tamanho da carga.
  constexpr double kRatioGateMax = 3.9; // D30, re-derived 2026-07-25 -- unconditional, everywhere.

  // --- Metrics (a) + (b): pure CPU, no GL, no display -- kAbRepeats repeats, best-of-N ---------
  const AbResult ab = bench_ab(kAbCommands);
  const double pure_qps = ab.streaming_qps;
  const double layered_ratio = ab.layered_ratio;

  std::printf("GLINTFX_PERF pure_batcher_quads_per_s=%.1f\n", pure_qps);
  std::printf("GLINTFX_PERF layered_streaming_ratio=%.3f\n", layered_ratio);
  // EN: diagnostics, never gated -- a paired median far above the gated ratio means the machine
  //     was contended during the run, which is exactly what the gated statistic is built to
  //     ignore and exactly what a reader of a red CI log needs to know.
  // PT: diagnóstico, nunca gateado -- uma mediana pareada bem acima da razão gateada significa
  //     que a máquina estava disputada durante a execução, que é justamente o que a estatística
  //     gateada é feita para ignorar e justamente o que quem lê um log de CI vermelho precisa
  //     saber.
  std::printf("GLINTFX_PERF layered_streaming_ratio_paired_median_wall=%.3f\n",
              ab.diag_paired_median);
  std::printf("GLINTFX_PERF ab_samples=%d\n", ab.samples);

  // --- Metric (c): end-to-end under Xvfb/llvmpipe, real GL context ----------------------------
  const double e2e_median_ms = bench_e2e_median_ms(30, 2);
  std::printf("GLINTFX_PERF e2e_10k_sprite_1k_prim_median_ms=%.3f\n", e2e_median_ms);

  // --- Gate (a): pure_batcher_quads_per_s ------------------------------------------------------
  if (pure_qps < kBaselinePureBatcherQuadsPerS / kSanityFactor) {
    std::printf("FAIL: pure_batcher_quads_per_s below the 10x sanity ceiling (%.1f < %.1f)\n",
                pure_qps, kBaselinePureBatcherQuadsPerS / kSanityFactor);
    ++failures;
  }
  if (strict && pure_qps < kBaselinePureBatcherQuadsPerS / kStrictFactor) {
    std::printf(
        "FAIL: pure_batcher_quads_per_s below the strict 1.5x baseline (%.1f < %.1f, "
        "GLINTFX_PERF_STRICT=1)\n",
        pure_qps, kBaselinePureBatcherQuadsPerS / kStrictFactor);
    ++failures;
  }

  // --- Gate (b): layered_streaming_ratio -- STRICT EVERYWHERE, machine-relative (D30) ---------
  if (layered_ratio > kRatioGateMax) {
    std::printf(
        "FAIL: layered_streaming_ratio exceeds the %.1fx streaming-mode ceiling (%.3f > "
        "%.1f) -- risk 2 (layers vs batching), unconditional on every machine\n",
        kRatioGateMax, layered_ratio, kRatioGateMax);
    ++failures;
  }

  // --- Gate (c): e2e_10k_sprite_1k_prim_median_ms ----------------------------------------------
  if (e2e_median_ms < 0.0) {
    std::printf("FAIL: metric (c) could not be measured (GL/host/texture setup failed)\n");
    ++failures;
  } else {
    if (e2e_median_ms > kBaselineE2eMedianMs * kSanityFactor) {
      std::printf(
          "FAIL: e2e_10k_sprite_1k_prim_median_ms above the 10x sanity ceiling (%.3f > "
          "%.3f)\n",
          e2e_median_ms, kBaselineE2eMedianMs * kSanityFactor);
      ++failures;
    }
    if (strict && e2e_median_ms > kBaselineE2eMedianMs * kStrictFactor) {
      std::printf(
          "FAIL: e2e_10k_sprite_1k_prim_median_ms above the strict 1.5x baseline (%.3f > "
          "%.3f, GLINTFX_PERF_STRICT=1)\n",
          e2e_median_ms, kBaselineE2eMedianMs * kStrictFactor);
      ++failures;
    }
  }

  if (failures == 0) {
    std::puts("draw2d_perf_budget: PASS");
    return 0;
  }
  std::printf("draw2d_perf_budget: %d check(s) FAILED\n", failures);
  return failures;
}
