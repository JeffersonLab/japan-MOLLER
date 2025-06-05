import ROOT
import sys

def inspect_rntuples():
    print("=== RNTuple Field Inspection with PyROOT ===")
    print("File: 4_rntuple.root")
    print("ROOT version:", ROOT.gROOT.GetVersion())
    print()
    
    # Open the file
    try:
        file = ROOT.TFile.Open("4_rntuple.root", "READ")
        if not file or file.IsZombie():
            print("Error: Could not open file 4_rntuple.root")
            return
        
        # Get the list of keys
        keys = file.GetListOfKeys()
        rntuple_names = []
        
        print("Keys in file:")
        for key in keys:
            key_name = key.GetName()
            class_name = key.GetClassName()
            print(f"  {key_name} ({class_name})")
            
            if "RNTuple" in class_name:
                rntuple_names.append(key_name)
        
        print(f"\nFound {len(rntuple_names)} RNTuples:")
        for name in rntuple_names:
            print(f"  - {name}")
        
        # Try to inspect each RNTuple
        print("\n=== RNTuple Details ===")
        for name in rntuple_names:
            print(f"\n--- {name} ---")
            try:
                # Try to open the RNTuple reader
                reader = ROOT.RNTupleReader.Open(name, "4_rntuple.root")
                print(f"  Entries: {reader.GetNEntries()}")
                
                # Get field information
                desc = reader.GetDescriptor()
                print("  Fields:")
                
                # Try to iterate over fields
                for field_id in range(desc.GetNFields()):
                    try:
                        field_desc = desc.GetFieldDescriptor(field_id)
                        field_name = field_desc.GetFieldName()
                        field_type = field_desc.GetTypeName()
                        print(f"    - {field_name} ({field_type})")
                    except Exception as e:
                        print(f"    Error accessing field {field_id}: {e}")
                        break
                
            except Exception as e:
                print(f"  Error opening RNTuple: {e}")
        
        file.Close()
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    inspect_rntuples()
