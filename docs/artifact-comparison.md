# Artifact Comparison Workflow

This document describes the automatic artifact comparison workflow implemented for the japan-MOLLER repository.

## Overview

The `compare-artifacts.yml` workflow automatically compares prompt summary artifacts between pull requests and the target branch (main). This helps catch unintended changes in analysis output that go beyond expected timestamp differences.

## How It Works

1. **Trigger**: The workflow runs on pull requests to the main branch, but only when relevant files are changed (Analysis/, Parity/, build configs, etc.)

2. **Generate Current Artifacts**: 
   - Runs the same build and analysis process as `build-lcg-cvmfs.yml`
   - Uses the same matrix of LCG configurations (LCG_106/gcc13, LCG_107/clang16, LCG_107/gcc14)
   - Generates mock data and runs qwparity with `--write-promptsummary`
   - Uploads the generated `summary_*.txt` files as artifacts

3. **Download Target Artifacts**:
   - Uses `dawidd6/action-download-artifact` to download corresponding artifacts from the target branch
   - Falls back gracefully if no target artifacts exist (e.g., first run or target branch not recently built)

4. **Compare Artifacts**:
   - Compares each summary file between current PR and target branch
   - Filters out timestamp lines (`Start Time:` and `End Time:`) that naturally vary between runs
   - Uses `diff` to detect meaningful differences in the remaining content
   - Fails the workflow if differences are found beyond timestamps

## Expected Output Format

The prompt summary files have the following format:
```
Run: 4 
Start Time: 2024-01-01 10:00:00
End Time: 2024-01-01 10:30:00
Number of events processed: 20000
Number of good events: 19500
Reference element: bcm1h02a
=========================================================================
                                   Yields
=========================================================================
bcm1h02a,1234.567,5.432,units
bcm1h02b,2345.678,6.543,units
...
=========================================================================
                          Asymmetries/Differences
=========================================================================
bcm1h02a-bcm1h15,0.123,0.001,ppm
...
```

## When the Workflow Fails

The workflow will fail and report an error if:
- Different yield values are detected
- Different asymmetry/difference values are detected  
- Different detector configurations are used
- The structure or format of the output changes

The workflow will NOT fail for:
- Different timestamps in `Start Time:` and `End Time:` lines
- Missing target artifacts (warns but continues)

## Benefits

- **Automatic validation**: Catches unintended changes in analysis output
- **CI integration**: Prevents merging PRs that change results unexpectedly
- **Reduced manual review**: Automates the comparison process mentioned in the issue
- **Clear feedback**: Provides detailed diff output when differences are found

## Configuration

The workflow can be customized by:
- Modifying the `paths` filter to include/exclude different file types
- Adjusting the matrix configurations to test different LCG versions
- Changing the retention period for artifacts
- Modifying the comparison logic to ignore additional variable content

## Testing

The comparison logic has been validated with a test script that verifies:
- Files with different timestamps but identical content pass comparison
- Files with actual content differences are correctly detected
- The grep filtering properly excludes timestamp lines