# Drawing RNTuple output with panguin: mock data walkthrough

These steps generate mock data, analyze it with `qwparity` into a ROOT file that holds RNTuples instead of TTrees, and draw it with panguin. Use them to check panguin's RNTuple support or to reproduce its plots.

The panguin configuration file is
[`panguin/macros/rntuple_mock_test.cfg`](macros/rntuple_mock_test.cfg).
It reads `rootfiles/rntuple/isu_sample_4.root` from the top of the repository (`rootfile ../rootfiles/rntuple/isu_sample_4.root`, relative to the `panguin` directory).

## Requirements

- ROOT 6.36 or newer. RNTuple support is compiled in only when CMake finds ROOT 6.36 or later; look for `supports RNTuple - enabling RNTuple functionality` in the CMake output.
- About 20 GB of free disk space: about 6 GB for the mock data and 5 to 7 GB for each analyzed file.
- To use the GUI under WSL, a working X display (WSLg provides `DISPLAY=:0`).

Run every command below from the top of the repository unless a step says otherwise.

## 1. Build JAPAN and panguin

```bash
cmake -B build -S .
make -C build -j$(nproc)

cmake -B panguin/build -S panguin
make -C panguin/build -j$(nproc)
```

## 2. Generate the mock data

```bash
build/qwmockdatagenerator -r 4 -e 1:100000 --config qwparity_simple.conf \
    --detectors mock_newdets.map --data .
```

This writes `QwMock_4.log` (100,000 physics events, about 5.8 GB) in the current directory. The configuration and map files are in `Parity/prminput/`.

## 3. Analyze the mock data into RNTuples

```bash
mkdir -p rootfiles/rntuple
build/qwparity -r 4 --config qwparity_simple.conf --detectors mock_newdets.map \
    --datahandlers mock_datahandlers.map --data . --rootfiles rootfiles/rntuple \
    --enable-rntuples --disable-trees
```

The output is `rootfiles/rntuple/isu_sample_4.root`. The file name comes from `rootfile-stem = isu_sample_` in `qwparity_simple.conf`.

Options:

- `--enable-rntuples` writes RNTuples instead of TTrees: it turns TTree output off, because the two writers can't be used in the same run. `--disable-trees` makes that explicit.
- `--rntuple-channel-arrays` writes one array field per channel instead of one field per value. panguin reads both layouts with the same configuration file. `--rntuple-compression-level 1` makes array-layout writing faster.
- `--rntuple-writer-thread` writes the RNTuple entries in a separate thread: the event loop queues a copy of each entry and continues while the thread serializes, compresses and writes it. The output is identical.
- More options spread the event loop over threads; the output is identical with any combination of them:
  - `--async-histograms` fills the event histograms in worker threads, while the event is processed further. `--histogram-threads N` sets the number of threads; 2 is a good choice for this configuration.
  - `--pipeline` reads, decodes and cuts the events and passes them through the event ring in a reader thread, while the main thread processes the events that leave the ring.
  - `--pattern-thread`, together with `--pipeline`, `--enable-rntuples` and `--rntuple-writer-thread`, processes the helicity patterns in a third thread.
  - All of them together: `--rntuple-writer-thread --async-histograms --histogram-threads 2 --pipeline --pattern-thread`.
- `--disable-histos` skips histograms. The panguin configuration doesn't use them.

The option names must be spelled exactly as shown (`--disable-trees`, not `--disable-ttrees`). `qwparity` prints a warning for unrecognized options.

The file contains one RNTuple per TTree, with the same names: `evt`, `mul`, `pr`, `evts`, `muls`, `bursts`, the data-handler outputs `avgs`, `mulc_lrb`, `burst_avgs` and `burst_mulc_lrb`, and the correlator results such as `lrb_std`.

### Note: the slope files

The LinRegBlue correctors in `mock_datahandlers.map` read their slopes from `./blueR4new.slope.root` (and the related `blueR_*4new.slope.root` files) in the current directory. Each run then overwrites these files with its own slopes, so the corrected outputs (`mulc_lrb`, `burst_mulc_lrb`) depend on the run before. If the slope files are missing, run step 3 twice. To get the same corrected values on every run, save a copy of `blueR4new.slope.root` and restore it before each run. The plots in the configuration file don't use the corrected outputs.

## 4. Draw the RNTuples with panguin

panguin resolves the `rootfile` path relative to the current directory, so run it from `panguin/`.

Interactive GUI:

```bash
cd panguin
./build/panguin -f macros/rntuple_mock_test.cfg
```

Print mode (no window). This writes all the tabs to one multi-page PDF, `summaryPlots_4_rntuple_mock_test.pdf`, in the current directory. The `4` is the run number, taken from the file name. To change the location or format, set `plotsdir` (an existing directory) and `plotFormat`; with `png` or `gif`, panguin writes one file per tab:

```bash
cd panguin
./build/panguin -f macros/rntuple_mock_test.cfg -P -v 1
```

With `-v 1`, print mode also writes one `PLOT page N pad M: ...` line per plot: entries, mean, RMS and axis ranges. These lines are useful for comparing two files.

### What to expect

The configuration has six tabs:

| Tab | Contents |
|-----|----------|
| 1D basics (mul) | Plain variables, cuts, `.leaf` names, arithmetic, `Entry$` in a defined cut |
| 2D draw types (mul) | Point, `box`, `prof`, `colz` and line draws; a cut using `Entries$` |
| Raw channels (evt) | Raw and per-block values, `SumSq`, `RawMin`/`RawMax`, `abs` and `sqrt` |
| Pairs, correlator, functions | The `pr` RNTuple, the `lrb_std` correlator matrix `A` and counter `n`, `TMath::Abs` |
| Units and integer-valued variables | Units (`ppm`, `um`), integer axes for counters and error codes |
| Expected failures | An unknown channel and an unknown cut. These two pads stay empty |

The two expected failures should be the only `Cannot draw:` lines in the output:

```
Cannot draw: no_such_channel not found
Cannot draw: no_such_cut (cut) not found in RNTuple mul
```

For the 100,000-event mock run, the first tab should show these entry counts:

| Plot | Entries |
|------|---------|
| `asym_bcm1h02a` | 1531 |
| `asym_bcm1h02a` with `ErrorFlag==0` | 1445 |
| `1e6*asym_bcm1h02a` with `goodcut` | 1435 |

### Under WSL

WSLg provides the X display:

```bash
export DISPLAY=:0
cd panguin
./build/panguin -f macros/rntuple_mock_test.cfg
```

## 5. Optional: compare with TTree output

Analyze the same data again into TTrees in a separate directory:

```bash
mkdir -p rootfiles/ttree
build/qwparity -r 4 --config qwparity_simple.conf --detectors mock_newdets.map \
    --datahandlers mock_datahandlers.map --data . --rootfiles rootfiles/ttree
```

Then copy the configuration file and point it at the TTree file:

```bash
cd panguin
sed 's#rootfiles/rntuple#rootfiles/ttree#' macros/rntuple_mock_test.cfg > /tmp/ttree_mock_test.cfg
./build/panguin -f /tmp/ttree_mock_test.cfg -P -v 1 > ttree.log
./build/panguin -f macros/rntuple_mock_test.cfg -P -v 1 > rntuple.log
# TTree::Draw makes TH1F/TH2F histograms and the RNTuple path makes TH1D/TH2D,
# so drop the F/D before comparing
plots() { grep ^PLOT "$1" | sed 's/TH\([123]\)[FD]/TH\1/'; }
diff <(plots ttree.log) <(plots rntuple.log)
```

Because `/tmp/ttree_mock_test.cfg` keeps the relative `rootfile` path, run panguin from `panguin/` as shown. Print mode writes the TTree plots to `summaryPlots_4_ttree_mock_test.pdf`.

All plots should match except these three:

```
< PLOT page 0 pad 4: nothing drawn
> PLOT page 0 pad 4: TH1 entries=1435 mean=1.862451615 rms=38.14147305 x=[-160,150]
< PLOT page 3 pad 5: nothing drawn
> PLOT page 3 pad 5: TH1 entries=1445 mean=30.59353199 rms=23.16962747 x=[0,148]
< PLOT page 4 pad 6: TH1 entries=1531 mean=3773364.985 rms=15467264.53 x=[0,73000000]
> PLOT page 4 pad 6: TH1 entries=1531 mean=3773364.985 rms=15467264.53 x=[0,70700000]
```

They come from known differences between the two modes:

- In TTree mode, panguin can't draw an expression that starts with a number (`1e6*asym_bcm1h02a`, page 0 pad 4) or that uses a namespace (`TMath::Abs(...)`, page 3 pad 5), so those pads stay empty. The RNTuple path draws both.
- `ErrorFlag` (page 4 pad 6) is stored as a double in the TTree, so TTree::Draw uses a non-integer axis for it. In RNTuple mode, panguin uses an integer axis because all the values are whole numbers. The data are the same; only the axis range differs.

See the "RNTuple files" section of [README.md](README.md) for the full list of supported names and draw options.
