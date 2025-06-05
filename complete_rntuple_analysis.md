# Complete RNTuple Structure Analysis - 4_rntuple.root

## Executive Summary
The `4_rntuple.root` file contains 8 distinct RNTuples representing different data aggregation levels and purposes in the QwAnalysis framework. This appears to be a complete parity violation experiment dataset with hierarchical data organization.

## RNTuple Structure Overview

| RNTuple Name | Entries | Fields | Purpose | Data Level |
|--------------|---------|--------|---------|------------|
| **evt** | 19,989 | 5,468 | Raw event data | Individual events |
| **pr** | 9,003 | 1,144 | Processed events | Pairs/patterns |
| **mul** | 281 | 2,285 | Multiplet data | ~71 events/multiplet |
| **mulc_lrb** | 281 | 2,310 | Corrected multiplet data | L-R-B corrected |
| **evts** | 1 | 2,727 | Event statistics | Run summary |
| **muls** | 1 | 2,937 | Multiplet statistics | Run summary |
| **bursts** | 1 | 2,937 | Burst statistics | Run summary |
| **burst_mulc_lrb** | 1 | 2,970 | Burst corrected stats | Run summary |

## Data Hierarchy and Processing Flow

```
Raw Events (evt: 19,989 entries)
    ↓ [Event filtering/processing]
Processed Events (pr: 9,003 entries) 
    ↓ [Multiplet grouping: ~32 events/multiplet]
Multiplets (mul: 281 entries)
    ↓ [L-R-B corrections applied]
Corrected Multiplets (mulc_lrb: 281 entries)
    ↓ [Statistical aggregation]
Run Summaries (evts, muls, bursts, burst_mulc_lrb: 1 entry each)
```

## Field Type Patterns

### 1. Raw Data Fields (evt, pr)
- **Pattern**: `devicename_measurement`
- **Example**: `bpm1c10WS_block0`, `bcm1h02a_hw_sum`
- **Structure**: Individual measurements without statistical aggregation

### 2. Yield Fields (mul, muls, bursts)  
- **Pattern**: `yield_devicename_measurement`
- **Example**: `yield_bpm1c10WS_block0`, `yield_bcm1h02a_hw_sum`
- **Structure**: Processed yield measurements

### 3. Corrected Fields (mulc_lrb, burst_mulc_lrb)
- **Pattern**: `cor_devicename_measurement`
- **Example**: `cor_tq01_r1_block0`, `cor_tq01_r2_hw_sum`
- **Structure**: L-R-B (Left-Right-Blank) corrected data

### 4. Statistical Summary Fields
- **Pattern**: `measurement_hw_sum_m2`, `measurement_hw_sum_err`
- **Purpose**: Mean, variance, and error calculations
- **Found in**: Summary RNTuples (evts, muls, bursts)

## Physics Data Categories

### A. Beam Position Monitors (BPM)
- **Count**: ~100+ devices across different regions
- **Measurements**: WS (Wire Scanner), Elli (Elliptical), X, Y positions
- **Locations**: Hall areas (1c, 1h, 1p), Injector (0i, 1i, 2i), Linac (0l)

### B. Beam Current Monitors (BCM)
- **Count**: ~10 devices
- **Locations**: Hall 1 (1h02a, 1h04a, etc.), Target area, Linac (0l02)
- **Purpose**: Beam current normalization and monitoring

### C. Toroids (TQ) - In Corrected Data
- **Pattern**: `cor_tq01_r1`, `cor_tq01_r2`
- **Purpose**: Precise current measurements for asymmetry analysis

### D. Control/Metadata Fields
- **Coda_CleanData**: Data quality flag
- **Coda_ScanData1/2**: Scan parameters
- **Device_Error_Code**: Individual device status

## Helicity Block Structure
All measurements include 4-block structure:
- `block0`, `block1`, `block2`, `block3`
- Likely represents: [R+, R-, L+, L-] helicity states
- Essential for parity violation asymmetry calculations

## Data Processing Levels Explained

### 1. Event Level (evt: 19,989 entries)
- Raw detector readouts
- All devices included
- Highest time resolution
- Individual beam pulses/events

### 2. Processed Level (pr: 9,003 entries)  
- ~55% event retention (quality cuts applied)
- Same field structure as evt
- Filtered for data quality

### 3. Multiplet Level (mul: 281 entries)
- ~32 events grouped per multiplet
- Yield calculations performed
- Helicity sequence organization

### 4. Corrected Level (mulc_lrb: 281 entries)
- Systematic corrections applied
- L-R-B false asymmetry removal
- Reduced device set (focus on toroids)

### 5. Summary Level (1 entry each)
- Complete run statistics
- Mean, variance, error calculations
- Final physics results

## Key Insights

### 1. Data Quality
- Significant event filtering: 19,989 → 9,003 events (~55% retention)
- Comprehensive error flagging system
- Multiple data validation levels

### 2. Physics Focus
- Parity violation experiment (4-block helicity structure)
- Beam position and current monitoring emphasis
- Systematic error correction pipeline (L-R-B)

### 3. Analysis Strategy
- Hierarchical data structure enables multiple analysis approaches
- Event-by-event studies (evt/pr) vs. statistical analysis (mul/corrected)
- Comprehensive systematic studies possible

## Recommended Analysis Steps

1. **Data Quality Assessment**
   ```cpp
   // Check event retention rates
   // Examine Coda_CleanData and Device_Error_Code distributions
   ```

2. **Helicity Analysis**
   ```cpp
   // Extract block0-block3 data for asymmetry calculations
   // Verify helicity pattern integrity
   ```

3. **Beam Monitoring**
   ```cpp
   // Analyze BPM position stability
   // Check BCM current correlations
   ```

4. **Systematic Studies**
   ```cpp
   // Compare corrected vs. uncorrected data
   // Study L-R-B correction effectiveness
   ```

This represents a complete, well-structured parity violation experiment dataset ready for comprehensive physics analysis.
