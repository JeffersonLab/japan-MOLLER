#!/usr/bin/env python3
"""
Analyze ROOT RNTuple data to understand the physics measurements
"""

import ROOT
import os
import numpy as np
import statistics

def analyze_rntuple_data():
    """Analyze the actual data values in RNTuple files"""
    
    # Get all .root files in current directory
    root_files = [f for f in os.listdir('.') if f.endswith('.root')]
    
    print("=== ROOT RNTuple Data Analysis ===\n")
    
    for filename in sorted(root_files):
        print(f"\n{'='*60}")
        print(f"ANALYZING FILE: {filename}")
        print(f"{'='*60}")
        
        try:
            # Open the ROOT file
            file = ROOT.TFile.Open(filename)
            if not file or file.IsZombie():
                print(f"ERROR: Cannot open file {filename}")
                continue
            
            # Get list of RNTuples in the file
            keys = file.GetListOfKeys()
            rntuple_names = []
            
            for key in keys:
                if key.GetClassName() == "ROOT::Experimental::RNTuple":
                    rntuple_names.append(key.GetName())
            
            if not rntuple_names:
                print("No RNTuples found in this file")
                file.Close()
                continue
            
            # Analyze each RNTuple
            for rntuple_name in rntuple_names:
                print(f"\n--- RNTuple: {rntuple_name} ---")
                
                try:
                    # Open RNTuple reader
                    reader = ROOT.RNTupleReader.Open(rntuple_name, filename)
                    
                    print(f"Total entries: {reader.GetNEntries()}")
                    
                    # Get field descriptors
                    descriptor = reader.GetDescriptor()
                    field_descriptors = descriptor.GetFieldIterable()
                    
                    # Collect field information
                    fields_info = []
                    for field_desc in field_descriptors:
                        field_name = field_desc.GetFieldName()
                        field_type = field_desc.GetTypeName()
                        fields_info.append((field_name, field_type))
                    
                    print(f"Number of fields: {len(fields_info)}")
                    
                    # Sample a few entries to understand the data
                    sample_size = min(10, reader.GetNEntries())
                    print(f"\nSampling first {sample_size} entries:")
                    
                    # Try to read some representative fields
                    sample_fields = []
                    for field_name, field_type in fields_info[:5]:  # First 5 fields
                        try:
                            if 'double' in field_type.lower() or 'float' in field_type.lower():
                                sample_fields.append(field_name)
                        except:
                            continue
                    
                    if sample_fields:
                        print(f"Sampling fields: {', '.join(sample_fields[:3])}")
                        
                        # Read sample data
                        for i in range(sample_size):
                            reader.LoadEntry(i)
                            entry_data = []
                            
                            for field_name in sample_fields[:3]:
                                try:
                                    # Get field value
                                    field_value = reader.GetModel().Get[field_name]()
                                    if hasattr(field_value, '__call__'):
                                        value = field_value()
                                    else:
                                        value = field_value
                                    entry_data.append(f"{field_name}={value:.6g}")
                                except Exception as e:
                                    entry_data.append(f"{field_name}=ERROR")
                            
                            print(f"  Entry {i}: {', '.join(entry_data)}")
                    
                    # Statistical analysis for numerical fields
                    if reader.GetNEntries() > 1:
                        print(f"\nStatistical Analysis:")
                        
                        for field_name in sample_fields[:3]:
                            try:
                                values = []
                                max_entries = min(1000, reader.GetNEntries())  # Sample up to 1000 entries
                                
                                for i in range(max_entries):
                                    reader.LoadEntry(i)
                                    try:
                                        field_value = reader.GetModel().Get[field_name]()
                                        if hasattr(field_value, '__call__'):
                                            value = field_value()
                                        else:
                                            value = field_value
                                        
                                        if isinstance(value, (int, float)) and not (isinstance(value, float) and (value != value)):  # Check for NaN
                                            values.append(float(value))
                                    except:
                                        continue
                                
                                if values:
                                    print(f"  {field_name}:")
                                    print(f"    Count: {len(values)}")
                                    print(f"    Mean: {statistics.mean(values):.6g}")
                                    print(f"    Std Dev: {statistics.stdev(values) if len(values) > 1 else 0:.6g}")
                                    print(f"    Min: {min(values):.6g}")
                                    print(f"    Max: {max(values):.6g}")
                                
                            except Exception as e:
                                print(f"  {field_name}: Error in statistical analysis - {str(e)}")
                    
                    # Categorize fields by name patterns
                    print(f"\nField Categories:")
                    categories = {
                        'Correlations': [f for f, t in fields_info if 'cor_' in f],
                        'Yields': [f for f, t in fields_info if 'yield_' in f or 'Y_' in f],
                        'Asymmetries': [f for f, t in fields_info if 'asym_' in f or 'A_' in f],
                        'Beam Monitors': [f for f, t in fields_info if any(x in f.lower() for x in ['bcm', 'bpm', 'beam'])],
                        'Detectors': [f for f, t in fields_info if any(x in f.lower() for x in ['pd', 'sa', 'la', 'det'])],
                        'Other': []
                    }
                    
                    # Categorize remaining fields
                    categorized = set()
                    for cat_fields in categories.values():
                        categorized.update(cat_fields)
                    
                    categories['Other'] = [f for f, t in fields_info if f not in categorized]
                    
                    for category, cat_fields in categories.items():
                        if cat_fields:
                            print(f"  {category}: {len(cat_fields)} fields")
                            if len(cat_fields) <= 10:
                                print(f"    {', '.join(cat_fields[:10])}")
                            else:
                                print(f"    {', '.join(cat_fields[:5])} ... (+{len(cat_fields)-5} more)")
                    
                except Exception as e:
                    print(f"ERROR analyzing RNTuple {rntuple_name}: {str(e)}")
            
            file.Close()
            
        except Exception as e:
            print(f"ERROR processing file {filename}: {str(e)}")
    
    print(f"\n{'='*60}")
    print("Analysis complete!")

if __name__ == "__main__":
    analyze_rntuple_data()
