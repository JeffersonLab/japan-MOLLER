void analyze_all_rntuples() {
    // Open the ROOT file
    TFile* file = TFile::Open("../4_rntuple.root", "READ");
    if (!file || file->IsZombie()) {
        cout << "Error: Could not open file 4_rntuple.root" << endl;
        return;
    }
    
    cout << "=== Complete RNTuple Analysis ===" << endl;
    cout << "File: 4_rntuple.root" << endl << endl;
    
    // Get list of keys (RNTuples)
    TList* keys = file->GetListOfKeys();
    TIter keyIter(keys);
    TKey* key;
    
    vector<string> rntuple_names;
    
    while ((key = (TKey*)keyIter())) {
        string keyName = key->GetName();
        string className = key->GetClassName();
        
        if (className.find("RNTuple") != string::npos) {
            rntuple_names.push_back(keyName);
        }
    }
    
    cout << "Found " << rntuple_names.size() << " RNTuples:" << endl;
    
    // Analyze each RNTuple
    for (const auto& name : rntuple_names) {
        cout << "\n--- RNTuple: " << name << " ---" << endl;
        
        try {
            // Open the RNTuple reader
            auto reader = ROOT::RNTupleReader::Open(name, "../4_rntuple.root");
            
            // Get the descriptor
            const auto& descriptor = reader->GetDescriptor();
            
            // Print basic info
            cout << "  Entries: " << reader->GetNEntries() << endl;
            
            // Count fields by type
            int field_count = 0;
            map<string, int> field_type_counts;
            vector<string> sample_fields;
            
            // Get the top-level field descriptor and iterate through its sub-fields
            const auto& topFieldDesc = descriptor.GetFieldDescriptor(descriptor.GetFieldZeroId());
            auto fieldIterable = descriptor.GetFieldIterable(topFieldDesc);
            
            for (const auto& fieldDesc : fieldIterable) {
                field_count++;
                string field_name = fieldDesc.GetFieldName();
                string field_type = fieldDesc.GetTypeName();
                
                field_type_counts[field_type]++;
                
                // Collect first 10 field names as samples
                if (sample_fields.size() < 10) {
                    sample_fields.push_back(field_name + " (" + field_type + ")");
                }
            }
            
            cout << "  Total Fields: " << field_count << endl;
            cout << "  Field Types:" << endl;
            for (const auto& pair : field_type_counts) {
                cout << "    " << pair.first << ": " << pair.second << " fields" << endl;
            }
            
            cout << "  Sample Fields:" << endl;
            for (const auto& field : sample_fields) {
                cout << "    - " << field << endl;
            }
            
            if (field_count > 10) {
                cout << "    ... (" << (field_count - 10) << " more fields)" << endl;
            }
            
        } catch (const exception& e) {
            cout << "  Error reading RNTuple: " << e.what() << endl;
        }
    }
    
    file->Close();
    delete file;
}
