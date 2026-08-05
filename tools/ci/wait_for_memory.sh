#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# EN: MEMORY GATE for the heavy self-hosted CI (tools/ci/runner_up.sh). This is meant to
#     be the FIRST substantive step of every job in .github/workflows/heavy.yml, right
#     after `checkout` and before `cache`/`configure`/`build` -- nothing that consumes
#     memory should run before this gate has passed.
#
#     WHY THIS EXISTS: the self-hosted runner's container is capped at
#     `--memory=8g --memory-swap=8g` (see tools/ci/runner_up.sh), but /proc/meminfo INSIDE
#     the container reports the HOST's memory, not the container's cgroup -- there is no
#     lxcfs virtualizing it here. That is deliberate, not a bug this script works around:
#     the threshold is about the lead's WORKSTATION having headroom, not about the
#     container's own 8 GiB ceiling. The machine is his daily driver, not a dedicated
#     build box, so a heavy sanitizer/font-engine build should not start while he is
#     mid-task on something memory-hungry.
#
#     WHY MemAvailable AND NOT free's "available" column, parsed: MemAvailable is a value
#     the KERNEL itself computes (accounts for reclaimable page cache, not just literally
#     free pages), documented in Documentation/filesystems/proc.rst as "an estimate of how
#     much memory is available for starting new applications, without swapping" -- which is
#     exactly the question this gate is asking. Parsing `free -h` output would just be a
#     worse, locale- and column-order-fragile way to get the same number.
#
#     WHY FAIL ON TIMEOUT INSTEAD OF BUILDING ANYWAY (lead's decision, recorded here): a
#     job that goes red saying "there was not enough memory" is honest and re-runnable by
#     just pushing again or re-dispatching once the machine is free. Building anyway under
#     memory pressure risks an OOM kill mid-compile or mid-link, which surfaces as a
#     confusing build failure that LOOKS like a code regression and sends whoever is
#     debugging it down the wrong path.
#
#     CONFIG (env, all optional -- defaults must work with ZERO env set, the workflow YAML
#     is not allowed to be the only place these numbers live):
#       GLINTFX_CI_MIN_MEM_GIB    default 12    -- MemAvailable threshold, in GiB
#       GLINTFX_CI_MEM_TIMEOUT_S  default 2700  -- give up after this many seconds (45 min)
#       GLINTFX_CI_MEM_POLL_S     default 15    -- seconds between reads while waiting
#       GLINTFX_CI_MEMINFO_FILE   default /proc/meminfo -- test seam, see --selftest below
#
#     USAGE:
#       tools/ci/wait_for_memory.sh              # the gate itself; exit 0 = proceed
#       tools/ci/wait_for_memory.sh --selftest    # proves the three behaviours below
#                                                  # without waiting 45 real minutes
#       tools/ci/wait_for_memory.sh --help
#
# PT: GATE DE MEMÓRIA para o CI pesado self-hosted (tools/ci/runner_up.sh). Pensado para
#     ser o PRIMEIRO passo substantivo de todo job em .github/workflows/heavy.yml, logo
#     após o `checkout` e antes de `cache`/`configure`/`build` -- nada que consuma memória
#     deve rodar antes deste gate ter passado.
#
#     POR QUE ISTO EXISTE: o container do runner self-hosted é limitado a
#     `--memory=8g --memory-swap=8g` (ver tools/ci/runner_up.sh), mas o /proc/meminfo
#     DENTRO do container reporta a memória do HOST, não o cgroup do container -- não há
#     lxcfs virtualizando isso aqui. É de propósito, não um bug que este script contorna: o
#     limiar é sobre a ESTAÇÃO DE TRABALHO do líder ter folga, não sobre o teto de 8 GiB do
#     próprio container. A máquina é o dia a dia dele, não uma caixa de build dedicada,
#     então um build pesado de sanitizer/motor-de-fonte não deve começar enquanto ele está
#     no meio de algo que consome muita memória.
#
#     POR QUE MemAvailable E NÃO a coluna "available" do `free`, parseada: MemAvailable é
#     um valor que o próprio KERNEL calcula (considera page cache reclamável, não só
#     páginas literalmente livres), documentado em Documentation/filesystems/proc.rst como
#     "uma estimativa de quanta memória está disponível para iniciar novas aplicações, sem
#     fazer swap" -- que é exatamente a pergunta deste gate. Parsear a saída do `free -h`
#     seria só um jeito pior, frágil a locale e a ordem de coluna, de chegar no mesmo
#     número.
#
#     POR QUE FALHAR NO TIMEOUT EM VEZ DE BUILDAR ASSIM MESMO (decisão do líder, registrada
#     aqui): um job que fica vermelho dizendo "não havia memória suficiente" é honesto e
#     re-executável só empurrando de novo ou re-disparando quando a máquina estiver livre.
#     Buildar assim mesmo sob pressão de memória arrisca um OOM kill no meio da compilação
#     ou do link, o que aparece como uma falha de build confusa que PARECE regressão de
#     código e manda quem está debugando pro caminho errado.
#
#     CONFIG (env, todas opcionais -- os defaults têm que funcionar com ZERO env setado, o
#     YAML do workflow não pode ser a única fonte destes números):
#       GLINTFX_CI_MIN_MEM_GIB    default 12    -- limiar de MemAvailable, em GiB
#       GLINTFX_CI_MEM_TIMEOUT_S  default 2700  -- desiste depois de tantos segundos (45min)
#       GLINTFX_CI_MEM_POLL_S     default 15    -- segundos entre leituras enquanto espera
#       GLINTFX_CI_MEMINFO_FILE   default /proc/meminfo -- costura de teste, ver --selftest
#
#     USO:
#       tools/ci/wait_for_memory.sh              # o gate em si; exit 0 = pode seguir
#       tools/ci/wait_for_memory.sh --selftest    # prova os três comportamentos abaixo
#                                                  # sem esperar 45 minutos de verdade
#       tools/ci/wait_for_memory.sh --help
set -euo pipefail

# EN: Absolute path to this script, so --selftest can re-invoke it as a real subprocess
#     (real exit code, no pipeline in the middle -- see the "measure exit code WITHOUT a
#     pipeline" warning this script's own header takes seriously).
# PT: Caminho absoluto deste script, para o --selftest poder re-invocá-lo como um
#     subprocesso de verdade (código de saída real, sem pipeline no meio -- ver o aviso
#     "meça o exit code SEM pipeline" que este próprio script leva a sério).
SELF="$(readlink -f "${BASH_SOURCE[0]}")"

usage() {
  cat <<'EOF'
Uso: wait_for_memory.sh [--selftest|--help]

Sem argumentos: roda o gate (le MemAvailable, espera ate o limiar ou timeout).
  Exit 0 = memoria suficiente, pode seguir para o build.
  Exit != 0 = timeout estourado ou falha ao ler MemAvailable.

--selftest: prova as tres garantias do gate sem esperar 45 minutos de verdade.
--help:     esta mensagem.

Env (todas opcionais, ver cabecalho do arquivo para o racional):
  GLINTFX_CI_MIN_MEM_GIB    default 12
  GLINTFX_CI_MEM_TIMEOUT_S  default 2700
  GLINTFX_CI_MEM_POLL_S     default 15
  GLINTFX_CI_MEMINFO_FILE   default /proc/meminfo
EOF
}

# EN: Reads MemAvailable (kB, per /proc/meminfo's own unit) from the given meminfo-shaped
#     file. Prints nothing and returns nonzero if the line is missing or the file cannot be
#     read -- callers must treat empty output as failure, not as "zero available".
# PT: Le MemAvailable (kB, na propria unidade do /proc/meminfo) do arquivo no formato de
#     meminfo dado. Nao imprime nada e retorna nao-zero se a linha faltar ou o arquivo nao
#     puder ser lido -- quem chama deve tratar saida vazia como falha, nao como "zero
#     disponivel".
mem_available_kib() {
  local file="$1"
  awk '/^MemAvailable:/ { print $2; exit }' "${file}" 2>/dev/null
}

# EN: kB -> GiB, one decimal place. /proc/meminfo's "kB" is actually KiB (1024 bytes), a
#     long-standing kernel naming quirk -- dividing by 1024*1024 is correct here.
# PT: kB -> GiB, uma casa decimal. O "kB" do /proc/meminfo e na verdade KiB (1024 bytes),
#     uma peculiaridade de nomenclatura antiga do kernel -- dividir por 1024*1024 esta
#     correto aqui.
fmt_gib() {
  awk -v k="$1" 'BEGIN { printf "%.1f", k / 1024 / 1024 }'
}

fmt_elapsed() {
  local s="$1"
  printf '%02d:%02d' $(( s / 60 )) $(( s % 60 ))
}

# EN: The gate itself. Polls until MemAvailable clears the threshold, or bails at the
#     timeout. Prints a progress line on every read (a step silent for 40 minutes looks
#     hung), and always prints how long it waited on the way out -- including "waited 0 s"
#     on the happy-first-read path, so the log tells "did not wait" apart from "nobody
#     measured".
# PT: O gate em si. Faz polling ate o MemAvailable superar o limiar, ou desiste no timeout.
#     Imprime uma linha de progresso a cada leitura (um passo mudo por 40 minutos parece
#     travado), e sempre imprime quanto esperou na saida -- inclusive "esperou 0 s" no
#     caminho feliz de primeira leitura, para o log distinguir "nao esperou" de "ninguem
#     mediu".
gate() {
  local meminfo="${GLINTFX_CI_MEMINFO_FILE:-/proc/meminfo}"
  local min_gib="${GLINTFX_CI_MIN_MEM_GIB:-12}"
  local timeout_s="${GLINTFX_CI_MEM_TIMEOUT_S:-2700}"
  local poll_s="${GLINTFX_CI_MEM_POLL_S:-15}"
  local threshold_kib=$(( min_gib * 1024 * 1024 ))
  local start_s="${SECONDS}"
  local waited mem_kib

  while true; do
    mem_kib="$(mem_available_kib "${meminfo}")"
    if [ -z "${mem_kib}" ]; then
      echo "wait_for_memory: nao consegui ler MemAvailable de ${meminfo}" >&2
      return 1
    fi

    waited=$(( SECONDS - start_s ))

    if [ "${mem_kib}" -ge "${threshold_kib}" ]; then
      echo "[t+$(fmt_elapsed "${waited}")] MemAvailable=$(fmt_gib "${mem_kib}") GiB / alvo ${min_gib} GiB -- OK, seguindo"
      echo "wait_for_memory: esperou ${waited} s"
      return 0
    fi

    echo "[t+$(fmt_elapsed "${waited}")] MemAvailable=$(fmt_gib "${mem_kib}") GiB / alvo ${min_gib} GiB -- aguardando (faltam $(fmt_gib $(( threshold_kib - mem_kib ))) GiB)"

    if [ "${waited}" -ge "${timeout_s}" ]; then
      echo "wait_for_memory: timeout de ${timeout_s}s estourado -- esperou ${waited} s sem atingir ${min_gib} GiB livres de MemAvailable" >&2
      echo "wait_for_memory: esperou ${waited} s" >&2
      return 1
    fi

    sleep "${poll_s}"
  done
}

# EN: Three scenarios, run as real subprocesses of this same script (never sourced, never
#     piped -- exit codes read straight from `$?` right after the call, per the session's
#     own "measure exit code WITHOUT a pipeline" rule). Each scenario points
#     GLINTFX_CI_MEMINFO_FILE at a throwaway file this function controls, so none of it
#     touches the real /proc/meminfo or waits anywhere close to 45 real minutes.
# PT: Tres cenarios, rodados como subprocessos de verdade deste mesmo script (nunca dado
#     source, nunca em pipe -- exit codes lidos direto de `$?` logo apos a chamada, pela
#     regra da propria sessao de "medir exit code SEM pipeline"). Cada cenario aponta
#     GLINTFX_CI_MEMINFO_FILE para um arquivo descartavel controlado por esta funcao, entao
#     nada disto toca o /proc/meminfo real nem espera perto de 45 minutos de verdade.
selftest() {
  local tmp
  tmp="$(mktemp -d)"
  # shellcheck disable=SC2064
  trap "rm -rf '${tmp}'" RETURN

  local failures=0

  # --- cenario 1: limiar ja satisfeito na primeira leitura -> exit 0 imediato, sem sleep.
  echo "MemAvailable:   20971520 kB" > "${tmp}/mem_ok"
  local out1 rc1
  out1="$(GLINTFX_CI_MEMINFO_FILE="${tmp}/mem_ok" \
          GLINTFX_CI_MIN_MEM_GIB=12 \
          GLINTFX_CI_MEM_TIMEOUT_S=5 \
          GLINTFX_CI_MEM_POLL_S=1 \
          "${SELF}" 2>&1)" && rc1=0 || rc1=$?
  echo "=== selftest 1/3: limiar ja satisfeito -> exit 0 imediato ==="
  echo "${out1}"
  if [ "${rc1}" -eq 0 ] && printf '%s' "${out1}" | grep -qw -- "OK,"; then
    echo "selftest 1/3: PASS (exit=${rc1})"
  else
    echo "selftest 1/3: FAIL (exit=${rc1}, esperado 0)"
    failures=$(( failures + 1 ))
  fi

  # --- cenario 2: limiar nunca satisfeito + timeout curto -> exit != 0, mensagem de porque.
  echo "MemAvailable:   1048576 kB" > "${tmp}/mem_low"
  local out2 rc2
  out2="$(GLINTFX_CI_MEMINFO_FILE="${tmp}/mem_low" \
          GLINTFX_CI_MIN_MEM_GIB=12 \
          GLINTFX_CI_MEM_TIMEOUT_S=2 \
          GLINTFX_CI_MEM_POLL_S=1 \
          "${SELF}" 2>&1)" && rc2=0 || rc2=$?
  echo "=== selftest 2/3: limiar nunca satisfeito, timeout curto -> exit != 0 ==="
  echo "${out2}"
  if [ "${rc2}" -ne 0 ] && printf '%s' "${out2}" | grep -qw -- "timeout"; then
    echo "selftest 2/3: PASS (exit=${rc2})"
  else
    echo "selftest 2/3: FAIL (exit=${rc2}, esperado != 0 com mensagem de timeout)"
    failures=$(( failures + 1 ))
  fi

  # --- cenario 3: limiar satisfeito depois de N leituras -> exit 0, esperou > 0 s.
  #     Um updater em background troca o conteudo do arquivo depois de ~2 s; o gate faz
  #     polling a cada 1 s, entao leva 2-3 leituras ate ver o valor alto.
  echo "MemAvailable:   1048576 kB" > "${tmp}/mem_rises"
  (
    sleep 2
    echo "MemAvailable:   20971520 kB" > "${tmp}/mem_rises"
  ) &
  local updater_pid=$!
  local out3 rc3
  out3="$(GLINTFX_CI_MEMINFO_FILE="${tmp}/mem_rises" \
          GLINTFX_CI_MIN_MEM_GIB=12 \
          GLINTFX_CI_MEM_TIMEOUT_S=10 \
          GLINTFX_CI_MEM_POLL_S=1 \
          "${SELF}" 2>&1)" && rc3=0 || rc3=$?
  wait "${updater_pid}" 2>/dev/null || true
  echo "=== selftest 3/3: limiar satisfeito depois de N leituras -> exit 0, esperou > 0 s ==="
  echo "${out3}"
  local waited3
  waited3="$(printf '%s\n' "${out3}" | awk -F': esperou | s$' '/esperou/ {print $2; exit}')"
  if [ "${rc3}" -eq 0 ] && [ -n "${waited3}" ] && [ "${waited3}" -gt 0 ]; then
    echo "selftest 3/3: PASS (exit=${rc3}, esperou ${waited3} s)"
  else
    echo "selftest 3/3: FAIL (exit=${rc3}, esperou='${waited3}', esperado exit=0 e esperou > 0)"
    failures=$(( failures + 1 ))
  fi

  echo "=== selftest: ${failures} falha(s) de 3 cenarios ==="
  [ "${failures}" -eq 0 ]
}

main() {
  case "${1:-}" in
    --selftest)
      selftest
      ;;
    --help|-h)
      usage
      ;;
    "")
      gate
      ;;
    *)
      echo "wait_for_memory: argumento desconhecido: ${1}" >&2
      usage >&2
      return 2
      ;;
  esac
}

main "$@"
