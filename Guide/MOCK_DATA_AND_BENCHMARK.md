# Running JAPAN‑MOLLER on Mock Data + Display‑Encoding Benchmark

This guide explains how to run the analyzer (`qwparity`) against generated mock MOLLER data
, how to stream results live to Panguin via a memory‑mapped
file, and how the display‑encoding benchmark (TGraph vs TProfile vs TTree)
was performed.

## 1. Run from mock data (offline → ROOT file)

The minimal command that analyzes the full mock run and writes a ROOT file:

```bash
build/qwparity \
  -r 4 \
  --data . \
  --config qwparity_simple.conf \
  --detectors mock_newdets.map \
  --burstlength 15
```

What each flag does:

| Flag | Meaning |
|------|---------|
| `-r 4` | Run number 4 → reads `QwMock_4.log`. |
| `--data .` | **Required.** Directory holding the data file. The default is `/adaq1/data1/apar`; the mock log lives at the repo root, so point it at `.`. (Also settable via `$QW_DATA`.) |
| `--config qwparity_simple.conf` | Top‑level options file ([Parity/prminput/qwparity_simple.conf](../Parity/prminput/qwparity_simple.conf)). |
| `--detectors mock_newdets.map` | **Required.** Loads the full mock beamline (Main + Injector) that actually contains channels like `bcm1h02a`. The config's default `isu_detectors.map` is a minimal test stand **without** those channels, so omitting this yields "Dependent variable … not found" and empty output. |
| `--burstlength 15` | 15 **patterns** per burst → 90 patterns → 6 bursts. |

Output ROOT file(s) are written to the working directory (or `--rootfiles <dir>`).

### Useful run options

| Flag | Effect |
|------|--------|
| `-e :10000` | Process only the first 10000 events (quick smoke test). |
| `--rootfiles <dir>` | Directory for output ROOT files. |
| `--write-temporary-rootfiles 0` | Do not write intermediate/temporary ROOT files. |
| `--datahandlers <map>` | Attach data handlers (trend, burst‑graph, etc.), see below. |
| `--disable-trees` | Skip per‑event/pattern trees (histograms only). |
| `--disable-mps-tree` / `--disable-pair-tree` / `--disable-hel-tree` | Disable the `evt` / `pr` / `mul` streaming trees individually. |

> **Tree naming:** singular names (`evt`/`pr`/`mul`) are the streaming
> per‑event / per‑pair / per‑pattern trees; plural (`evts`/`prs`/`muls`) are the
> 1‑entry running‑sum summary trees (always present).

---

## 2. Run with a live memory‑mapped display (Panguin)

`qwparity` can publish histograms/objects into a shared `TMapFile` so Panguin
can display them live while analysis runs.

### 2a. Producer (analyzer)

```bash
build/qwparity \
  -r 4 --data . \
  --config qwparity_simple.conf \
  --detectors mock_newdets.map \
  --datahandlers mock_datahandlers_live.map \
  --burstlength 15 \
  --enable-mapfile \
  --mapfile-dir /dev/shm \
  --mapfile-update-interval 1000
```

| Flag | Meaning |
|------|---------|
| `--enable-mapfile` | Publish objects to a `TMapFile` instead of (only) a ROOT file. |
| `--mapfile-dir <dir>` | Directory that holds `QwMemMapFile.map`. Default `/dev/shm` (Linux tmpfs). Override on platforms without `/dev/shm` (e.g. **macOS**: use a writable RAM/scratch path). |
| `--mapfile-update-interval N` | Refresh the map every *N* events. Use a **large** value (~1000) for live viewing to keep the reader/writer race window tiny. |
| `--datahandlers mock_datahandlers_live.map` | Attaches the `QwTrendHandler` that produces the live `trend_*` strip charts. |

### 2b. Consumer (Panguin)

Build Panguin once (`cd panguin && cmake -B build -S . && cmake --build build -j4`),
then:

```bash
# GUI (needs a display; under WSLg use DISPLAY=:0)
DISPLAY=:0 panguin/build/panguin -f panguin/macros/panguin_trend_live.cfg

# or batch to a PDF
panguin/build/panguin -f panguin/macros/panguin_trend_live.cfg -P
```

[panguin/macros/panguin_trend_live.cfg](../panguin/macros/panguin_trend_live.cfg)
reads `rootfile /dev/shm/QwMemMapFile.map` and draws `trend_bcm1h02a`,
`trend_bcm1h15`, `trend_bcm_target`, `trend_bpm1h04X`.

> **Note on histogram names in the map:** a `TMapFile` has no subdirectories, so
> every group's histograms share one flat top‑level directory. Histograms are
> therefore **namespaced by their group** (`evt_histo_*`, `mul_histo_*`,
> `burst_histo_*`) to avoid `TDirectoryFile::Append: Replacing existing TH1`
> collisions. On‑disk (ROOT‑file) names are unchanged. The `trend_*` handler
> names above are unaffected.

---

## 3. Display‑encoding benchmark (TGraph vs TProfile vs TTree)


The same 4 per‑pattern asymmetry channels (`bcm1h02a`, `bcm1h15`, `bcm_target`,
`bpm1h04X`) are encoded three ways and compared against a no‑display baseline:

| Method | Display object | Handler |
|--------|----------------|---------|
| **BASE** | none (subsystem monitor histograms only) — reference | `--disable-trees` |
| **TREE** | full per‑pattern `mul` TTree (traditional baseline) | `--disable-mps-tree --disable-pair-tree` |
| **PROFILE** | 4 rolling `TProfile` strip charts | `--disable-trees --datahandlers bench_dh_profile.map` |
| **GRAPH** | per‑burst `TGraph`s (`burst_graphs` tree) | `--disable-mps-tree --disable-pair-tree --disable-hel-tree --datahandlers bench_dh_graph.map` |

> **GRAPH requires the burst‑graph handler.** The GRAPH method uses
> `QwBurstGraphHandler`, which lives on the `BurstPatternGraphs` branch. On
> branches without it (e.g. the `TTreesInProfiles` PR branch), only BASE, TREE
> and PROFILE run — pass `BENCH_METHODS='BASE TREE PROFILE'` (see below).

### 3b. Two measurement passes per method

1. **Throughput** — run with `--enable-mapfile --mapfile-update-interval 100`,
   *N* reps (default 3), take the **median** of `events / Real time used` from
   the run log.
2. **Footprint** — run **without** the mapfile, then serialize the display object
   with `TBufferFile(kWrite).WriteObject(obj).Length()`. This is the exact number
   of bytes a live `TMapFile` would hold for that object (the pre‑allocated
   ~142 MiB map is constant across methods and is *not* a differentiator, so the
   serialized object size is the meaningful footprint metric).

### 3c. Running the benchmark

The harness lives in [benchmarks/display_encoding/](../benchmarks/display_encoding/)
(source ROOT and build `qwparity` first):

```bash
bash benchmarks/display_encoding/bench.sh          # 3 reps, all 4 methods
bash benchmarks/display_encoding/bench.sh 5        # 5 reps

# On a branch without QwBurstGraphHandler (e.g. TTreesInProfiles), skip GRAPH:
BENCH_METHODS='BASE TREE PROFILE' bash benchmarks/display_encoding/bench.sh
```

It writes per‑rep rows and logs under `/dev/shm/japan_bench/` (override with the
`BENCH_OUT` env var), invokes
[benchmarks/display_encoding/footprint.C](../benchmarks/display_encoding/footprint.C)
for the footprint value, and prints a summary table. Supporting maps:
`bench_dh_profile.map`, `bench_dh_graph.map`, `bench_burstgraph.map`
(in [Parity/prminput/](../Parity/prminput/)).


---

## 4. Common gotchas

- **macOS:** there is no `/dev/shm`; pass `--mapfile-dir <writable RAM/scratch dir>`.
