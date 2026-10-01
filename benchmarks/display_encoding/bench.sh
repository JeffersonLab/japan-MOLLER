#!/bin/bash
# ---------------------------------------------------------------------------
# Three-encoding display benchmark for JAPAN-MOLLER.
#
# Same 4 per-pattern asymmetry channels, encoded three ways, all streamed live
# via TMapFile for Panguin:
#   BASE    : no display object (subsystem monitor histos only)  -> reference
#   TREE    : full per-pattern "mul" TTree (traditional monitoring baseline)
#   PROFILE : QwTrendHandler        -> 4 rolling TProfile strip charts
#   GRAPH   : QwBurstGraphHandler   -> per-burst TGraphs (burst_graphs tree)
#
# NOTE: the GRAPH method requires QwBurstGraphHandler, which lives on the
# BurstPatternGraphs branch. On branches without it (e.g. TTreesInProfiles),
# run only the other methods:  BENCH_METHODS='BASE TREE PROFILE' bash bench.sh
#
# Fairness: identical run (4), full events, --burstlength 15, same detectors,
# same mapfile update interval, all output to /dev/shm (RAM) to remove disk I/O.
#
# Two passes per method:
#   THROUGHPUT (x REPS, --enable-mapfile) : Real time used -> events/sec
#   FOOTPRINT  (x1, no mapfile)           : serialized bytes of the display
#                                           object (exact bytes a mapfile holds)
#
# Reported: median events/sec, overhead vs BASE, display-object footprint bytes.
#
# Prerequisites: source ROOT (thisroot.sh) and build qwparity first.
# Usage: bash benchmarks/display_encoding/bench.sh [REPS]   (default 3)
#   env: BENCH_OUT     override RAM scratch dir (default /dev/shm/japan_bench)
#        BENCH_METHODS override method list (default "BASE TREE PROFILE GRAPH")
# ---------------------------------------------------------------------------
set -u

# Resolve repo root from this script's location (benchmarks/display_encoding/).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT" || exit 1

if ! command -v root >/dev/null 2>&1; then
  echo "ERROR: ROOT not found on PATH. Source thisroot.sh before running." >&2
  exit 1
fi
if [ ! -x build/qwparity ]; then
  echo "ERROR: build/qwparity not found. Build it first:" >&2
  echo "       cmake --build build -j4 --target qwparity" >&2
  exit 1
fi

REPS="${1:-3}"
OUTBASE="${BENCH_OUT:-/dev/shm/japan_bench}"
OUTDIR="$OUTBASE/out"
LOGDIR="$OUTBASE/logs"
RESULTS="$OUTBASE/results.csv"
FOOTPRINT_C="$SCRIPT_DIR/footprint.C"
mkdir -p "$OUTDIR" "$LOGDIR"

COMMON="-r 4 --data . --config qwparity_simple.conf --detectors mock_newdets.map --rootfiles $OUTDIR --write-temporary-rootfiles 0 --burstlength 15"
MAP="--enable-mapfile --mapfile-update-interval 100"

declare -A EXTRA
EXTRA[BASE]="--disable-trees"
EXTRA[TREE]="--disable-mps-tree --disable-pair-tree"
EXTRA[PROFILE]="--disable-trees --datahandlers bench_dh_profile.map"
EXTRA[GRAPH]="--disable-mps-tree --disable-pair-tree --disable-hel-tree --datahandlers bench_dh_graph.map"

METHODS="${BENCH_METHODS:-BASE TREE PROFILE GRAPH}"

echo "method,rep,events,real_s,evps,map_du_kb" > "$RESULTS"
declare -A MEDIAN FOOT FOOTOBJS
median () { printf '%s\n' "$@" | sort -n | awk '{a[NR]=$0} END{print a[int((NR+1)/2)]}'; }

for M in $METHODS; do
  # ---- footprint pass (no mapfile) ----
  rm -rf "$OUTDIR"; mkdir -p "$OUTDIR"
  build/qwparity $COMMON ${EXTRA[$M]} > "$LOGDIR/${M}_foot.log" 2>&1
  OUT=$(ls "$OUTDIR"/*.root 2>/dev/null | head -1)
  FLINE=$(root -l -b -q "${FOOTPRINT_C}(\"${OUT:-}\",\"$M\")" 2>/dev/null | grep -E '^BYTES=')
  FOOT[$M]=$(echo "$FLINE" | grep -oE 'BYTES=[0-9NA]+' | cut -d= -f2)
  FOOTOBJS[$M]=$(echo "$FLINE" | grep -oE 'OBJS=[0-9]+' | cut -d= -f2)

  # ---- throughput passes (mapfile) ----
  evps_list=()
  for r in $(seq 1 "$REPS"); do
    rm -f /dev/shm/QwMemMapFile.map
    rm -rf "$OUTDIR"; mkdir -p "$OUTDIR"
    L="$LOGDIR/${M}_tp${r}.log"
    build/qwparity $COMMON $MAP ${EXTRA[$M]} > "$L" 2>&1
    ev=$(grep -oE '[0-9]+ physics events were processed' "$L" | grep -oE '^[0-9]+' | tail -1)
    rt=$(grep -oE 'Real time used: [0-9.]+' "$L" | grep -oE '[0-9.]+' | tail -1)
    du=$(du -k /dev/shm/QwMemMapFile.map 2>/dev/null | awk '{print $1}')
    evps="NA"
    if [ -n "${ev:-}" ] && [ -n "${rt:-}" ]; then evps=$(echo "scale=1; $ev/$rt" | bc); evps_list+=("$evps"); fi
    echo "$M,$r,${ev:-NA},${rt:-NA},$evps,${du:-NA}" >> "$RESULTS"
  done
  MEDIAN[$M]=$(median "${evps_list[@]:-NA}")
done

echo ""
echo "================= DISPLAY-ENCODING BENCHMARK ================="
echo "run 4 | full events | --burstlength 15 | mapfile update=100 | reps=$REPS | out=$OUTDIR"
printf "%-8s %14s %14s %16s %6s\n" "method" "evts/s(med)" "overhead/s" "footprint(B)" "objs"
base_med="${MEDIAN[BASE]:-}"
for M in $METHODS; do
  ov="NA"
  if [ -n "$base_med" ] && [ "${MEDIAN[$M]:-NA}" != "NA" ]; then ov=$(echo "scale=1; $base_med - ${MEDIAN[$M]}" | bc); fi
  printf "%-8s %14s %14s %16s %6s\n" "$M" "${MEDIAN[$M]:-NA}" "$ov" "${FOOT[$M]:-NA}" "${FOOTOBJS[$M]:-0}"
done
echo "============================================================="
echo "results CSV: $RESULTS    logs: $LOGDIR"
echo "(overhead = BASE_median - method_median ; footprint = serialized bytes of the display object)"
