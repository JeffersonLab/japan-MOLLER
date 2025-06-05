#!/usr/bin/env python3
"""
Detailed analysis of the RNTuple data in 4_rntuple.root
"""

import ROOT
import statistics
import numpy as np

def analyze_main_rntuple_file():
    """Analyze the 4_rntuple.root file in detail"""
    
    filename = "4_rntuple.root"
    
    print("=== Detailed RNTuple Analysis ===")
    print(f"File: {filename}\n")
    
    try:
        file = ROOT.TFile.Open(filename)
        if not file or file.IsZombie():
            print(f"ERROR: Cannot open file {filename}")
            return
        
        # Get all RNTuple names
        keys = file.GetListOfKeys()
        rntuple_names = []
        
        for key in keys:
            if "RNTuple" in key.GetClassName():
                rntuple_names.append(key.GetName())
        
        print(f"Found {len(rntuple_names)} RNTuples:")
        for name in rntuple_names:
            print(f"  - {name}")
        print()
        
        # Analyze each RNTuple
        for rntuple_name in sorted(rntuple_names):
            print(f"{'='*60}")
            print(f"ANALYZING: {rntuple_name}")
            print(f"{'='*60}")
            
            try:
                # Open RNTuple reader
                reader = ROOT.RNTupleReader.Open(rntuple_name, filename)
                
                print(f"Total entries: {reader.GetNEntries()}")
                
                # Get field information
                model = reader.GetModel()
                field_names = []
                
                # Try to get field names from the model
                try:
                    # Get the descriptor to access field information
                    descriptor = reader.GetDescriptor()
                    field_descriptors = descriptor.GetFieldIterable()
                    
                    for field_desc in field_descriptors:
                        field_name = field_desc.GetFieldName()
                        field_type = field_desc.GetTypeName()
                        field_names.append((field_name, field_type))
                    
                    print(f"Number of fields: {len(field_names)}")
                    
                    # Show field summary
                    if field_names:
                        print("\nField Summary:")
                        field_types = {}
                        for fname, ftype in field_names:
                            ftype_clean = ftype.split('<')[0] if '<' in ftype else ftype
                            if ftype_clean not in field_types:
                                field_types[ftype_clean] = []
                            field_types[ftype_clean].append(fname)
                        
                        for ftype, names in field_types.items():
                            print(f"  {ftype}: {len(names)} fields")
                            if len(names) <= 5:
                                print(f"    {', '.join(names)}")
                            else:
                                print(f"    {', '.join(names[:3])} ... (+{len(names)-3} more)")
                    
                    # Sample data analysis
                    if reader.GetNEntries() > 0:
                        print(f"\nData Sampling (first few entries):")
                        
                        # Select a few representative fields for sampling
                        numeric_fields = []
                        for fname, ftype in field_names[:10]:  # Check first 10 fields
                            if any(t in ftype.lower() for t in ['double', 'float', 'int']):
                                numeric_fields.append(fname)
                        
                        if numeric_fields:
                            sample_size = min(5, reader.GetNEntries())
                            print(f"Sampling {len(numeric_fields[:3])} numeric fields for {sample_size} entries:")
                            
                            for i in range(sample_size):
                                reader.LoadEntry(i)
                                entry_values = []
                                
                                for field_name in numeric_fields[:3]:
                                    try:
                                        # Try different ways to access the field
                                        value = None
                                        
                                        # Method 1: Direct access
                                        try:
                                            field_ptr = model.GetDefaultEntry().Get[field_name]
                                            if field_ptr:
                                                value = field_ptr()
                                        except:
                                            pass
                                        
                                        # Method 2: Alternative access
                                        if value is None:
                                            try:
                                                field_ptr = model.Get[field_name]
                                                if field_ptr:
                                                    value = field_ptr()
                                            except:
                                                pass
                                        
                                        if value is not None:
                                            entry_values.append(f"{field_name}={value:.4g}")
                                        else:
                                            entry_values.append(f"{field_name}=N/A")
                                    
                                    except Exception as e:
                                        entry_values.append(f"{field_name}=ERROR")
                                
                                print(f"  Entry {i}: {', '.join(entry_values)}")
                        
                        # Statistical analysis on a subset of data
                        if numeric_fields and reader.GetNEntries() > 1:
                            print(f"\nStatistical Analysis:")
                            
                            for field_name in numeric_fields[:2]:  # Analyze first 2 numeric fields
                                try:
                                    values = []
                                    max_entries = min(100, reader.GetNEntries())
                                    
                                    for i in range(max_entries):
                                        reader.LoadEntry(i)
                                        try:
                                            # Try to get field value
                                            field_ptr = model.GetDefaultEntry().Get[field_name]
                                            if field_ptr:
                                                value = field_ptr()
                                                if isinstance(value, (int, float)) and not (isinstance(value, float) and value != value):
                                                    values.append(float(value))
                                        except:
                                            try:
                                                field_ptr = model.Get[field_name]
                                                if field_ptr:
                                                    value = field_ptr()
                                                    if isinstance(value, (int, float)) and not (isinstance(value, float) and value != value):
                                                        values.append(float(value))
                                            except:
                                                continue
                                    
                                    if len(values) > 0:
                                        print(f"  {field_name} (n={len(values)}):")
                                        print(f"    Range: [{min(values):.4g}, {max(values):.4g}]")
                                        if len(values) > 1:
                                            print(f"    Mean ± Std: {statistics.mean(values):.4g} ± {statistics.stdev(values):.4g}")
                                        else:
                                            print(f"    Value: {values[0]:.4g}")
                                    else:
                                        print(f"  {field_name}: No valid numeric data found")
                                
                                except Exception as e:
                                    print(f"  {field_name}: Analysis error - {str(e)[:50]}")
                    
                    # Field categorization
                    print(f"\nField Categorization:")
                    categories = {
                        'BCM (Beam Current Monitors)': [],
                        'BPM (Beam Position Monitors)': [],
                        'Yields': [],
                        'Asymmetries': [],
                        'Correlations': [],
                        'Target/Beamline': [],
                        'Time/Event Info': [],
                        'Other': []
                    }
                    
                    for fname, ftype in field_names:
                        fname_lower = fname.lower()
                        categorized = False
                        
                        if 'bcm' in fname_lower:
                            categories['BCM (Beam Current Monitors)'].append(fname)
                            categorized = True
                        elif 'bpm' in fname_lower:
                            categories['BPM (Beam Position Monitors)'].append(fname)
                            categorized = True
                        elif any(x in fname_lower for x in ['yield', 'y_']):
                            categories['Yields'].append(fname)
                            categorized = True
                        elif any(x in fname_lower for x in ['asym', 'a_']):
                            categories['Asymmetries'].append(fname)
                            categorized = True
                        elif 'cor' in fname_lower or 'corr' in fname_lower:
                            categories['Correlations'].append(fname)
                            categorized = True
                        elif any(x in fname_lower for x in ['target', 'beam', 'halo']):
                            categories['Target/Beamline'].append(fname)
                            categorized = True
                        elif any(x in fname_lower for x in ['time', 'event', 'entry', 'run', 'seq']):
                            categories['Time/Event Info'].append(fname)
                            categorized = True
                        
                        if not categorized:
                            categories['Other'].append(fname)
                    
                    for category, fields in categories.items():
                        if fields:
                            print(f"  {category}: {len(fields)} fields")
                            if len(fields) <= 3 and len(fields) > 0:
                                print(f"    Examples: {', '.join(fields)}")
                            elif len(fields) > 3:
                                print(f"    Examples: {', '.join(fields[:3])} ... (+{len(fields)-3} more)")
                
                except Exception as e:
                    print(f"ERROR accessing field information: {str(e)}")
                
            except Exception as e:
                print(f"ERROR analyzing RNTuple {rntuple_name}: {str(e)}")
            
            print()  # Add spacing between RNTuples
        
        file.Close()
        
    except Exception as e:
        print(f"ERROR processing file: {str(e)}")

if __name__ == "__main__":
    analyze_main_rntuple_file()
