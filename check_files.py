#!/usr/bin/env python3
"""
Simple check of ROOT file contents to find RNTuples
"""

import ROOT
import os

def check_root_files():
    """Check what's inside the ROOT files"""
    
    root_files = [f for f in os.listdir('.') if f.endswith('.root')]
    
    for filename in sorted(root_files):
        print(f"\n=== {filename} ===")
        
        try:
            file = ROOT.TFile.Open(filename)
            if not file or file.IsZombie():
                print("Cannot open file or file is corrupted")
                continue
            
            # List all keys in the file
            keys = file.GetListOfKeys()
            print(f"Number of keys: {keys.GetSize()}")
            
            rntuple_count = 0
            other_objects = []
            
            for key in keys:
                class_name = key.GetClassName()
                obj_name = key.GetName()
                
                if "RNTuple" in class_name:
                    rntuple_count += 1
                    print(f"  RNTuple found: {obj_name} (class: {class_name})")
                else:
                    other_objects.append(f"{obj_name} ({class_name})")
            
            if rntuple_count == 0:
                print("  No RNTuples found")
                if other_objects:
                    print("  Other objects:")
                    for obj in other_objects[:10]:  # Show first 10
                        print(f"    {obj}")
                    if len(other_objects) > 10:
                        print(f"    ... and {len(other_objects) - 10} more")
            
            file.Close()
            
        except Exception as e:
            print(f"Error: {str(e)}")

if __name__ == "__main__":
    check_root_files()
