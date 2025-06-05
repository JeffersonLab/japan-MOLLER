void simple_inspect() {
    // Open the ROOT file
    TFile* file = TFile::Open("4_rntuple.root", "READ");
    if (!file || file->IsZombie()) {
        cout << "Error: Could not open file 4_rntuple.root" << endl;
        return;
    }
    
    cout << "=== Simple RNTuple Inspection ===" << endl;
    cout << "File: 4_rntuple.root" << endl << endl;
    
    // List all keys in the file
    TList* keys = file->GetListOfKeys();
    TIter keyIter(keys);
    TKey* key;
    
    vector<string> rntuple_names;
    
    while ((key = (TKey*)keyIter())) {
        string keyName = key->GetName();
        string className = key->GetClassName();
        
        cout << "Key: " << keyName << " (Class: " << className << ")" << endl;
        
        if (className.find("RNTuple") != string::npos) {
            rntuple_names.push_back(keyName);
        }
    }
    
    cout << "\nFound " << rntuple_names.size() << " RNTuples:" << endl;
    for (const auto& name : rntuple_names) {
        cout << "  - " << name << endl;
    }
    
    // Try to get basic info about each RNTuple using ROOT's built-in methods
    cout << "\n=== Attempting to read RNTuple info ===" << endl;
    
    for (const auto& name : rntuple_names) {
        cout << "\n--- " << name << " ---" << endl;
        
        // Try to get the object
        TObject* obj = file->Get(name.c_str());
        if (obj) {
            cout << "  Object type: " << obj->ClassName() << endl;
            obj->Print();
        } else {
            cout << "  Could not retrieve object" << endl;
        }
    }
    
    file->Close();
    delete file;
}
