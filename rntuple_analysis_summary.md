# RNTuple Analysis Summary - 4_rntuple.root

## Overview
The `4_rntuple.root` file contains 8 RNTuples with extensive physics data from the QwAnalysis framework. Based on the field inspection, this appears to be one of the main RNTuples (likely "evt" or "evts") containing yield data from various detector subsystems.

## Data Structure Analysis

### Field Naming Convention
All fields follow a consistent pattern:
- **Prefix**: `yield_` - indicates these are yield measurements
- **Device Name**: Specific detector/instrument identifier
- **Measurement Type**: Various suffixes indicating the type of measurement
- **Statistics**: Each measurement includes multiple statistical quantities

### Field Suffixes and Their Meanings
1. **`_hw_sum`** - Hardware sum (total accumulated signal)
2. **`_block0`, `_block1`, `_block2`, `_block3`** - Individual block measurements (4 blocks total)
3. **`_num_samples`** - Number of samples used in the measurement
4. **`_Device_Error_Code`** - Error status for the device

## Detector Subsystems Identified

### 1. Beam Position Monitors (BPM)
**Location Codes:**
- `1c20` - Hall C, position 20
- `1p02b`, `1p03a` - Hall/area 1, positions with sub-designations
- `1h01` through `1h14` - Hall 1, positions 01-14
- `0i01a`, `1i02`, `1i04`, `1i06`, `2i01`, `2i02` - Injector area BPMs
- `0l01` through `0l10` - Linac area BPMs, positions 01-10
- `target` - Target area BPM

**Measurement Types per BPM:**
- **`WS`** - Wire Scanner measurement
- **`Elli`** - Elliptical/integrated measurement
- **`X`** - X-position measurement
- **`Y`** - Y-position measurement

**Special Target BPM Fields:**
- `bpm_targetXSlope`, `bpm_targetYSlope` - Slope measurements
- `bpm_targetXIntercept`, `bpm_targetYIntercept` - Intercept measurements
- `bpm_targetXMinChiSquare`, `bpm_targetYMinChiSquare` - Fit quality metrics

### 2. Beam Current Monitors (BCM)
**Devices:**
- `bcm1h02a`, `bcm1h02b` - Hall 1, position 02, channels a/b
- `bcm1h04a` - Hall 1, position 04, channel a
- `bcm1h12a`, `bcm1h12b`, `bcm1h12c` - Hall 1, position 12, channels a/b/c
- `bcm1h13`, `bcm1h15` - Hall 1, positions 13 and 15
- `bcm_target` - Target area BCM
- `bcm0l02` - Linac area BCM

### 3. Energy Measurement
- `target_energy` - Target energy measurement

### 4. Battery/Reference Signals
- `batery6`, `batery7` - Battery reference signals (likely for stability monitoring)

### 5. Phase Monitor
- `phasemonitor` - RF phase monitoring

## Physics Interpretation

### Block Structure
The 4-block structure (`block0` through `block3`) suggests this is **helicity-correlated data**:
- Each block likely represents measurements during different helicity states
- This is consistent with parity violation experiments where beam helicity is systematically varied
- The blocks may represent: [Right+, Right-, Left+, Left-] or similar helicity combinations

### Statistical Quantities
- **`hw_sum`**: Total accumulated signal across all samples
- **`num_samples`**: Sample count for statistical weight calculation
- **`Device_Error_Code`**: Quality flag (0 = good, non-zero = error conditions)

### Coordinate System
- **X/Y positions**: Beam position in transverse coordinates
- **WS (Wire Scanner)**: Beam profile measurements
- **Elli (Elliptical)**: Integrated beam intensity measurements

## Data Quality Indicators
- All measurements include error codes for data quality assessment
- Sample counts allow for proper statistical weighting
- Multiple measurement types per device provide cross-checks

## Analysis Recommendations

1. **Check sample counts** - Verify `num_samples` fields are reasonable
2. **Monitor error codes** - Filter data where `Device_Error_Code != 0`
3. **Helicity analysis** - Use block structure for asymmetry calculations
4. **Position correlation** - Analyze X/Y correlations for beam stability
5. **Current normalization** - Use BCM data to normalize other measurements

## Next Steps
1. Extract and plot sample data values from a few representative fields
2. Examine the other 7 RNTuples to understand the complete data structure
3. Map RNTuple names (evt, evts, mul, muls, pr, bursts, etc.) to their specific purposes
4. Create analysis scripts for asymmetry calculations using the block structure
