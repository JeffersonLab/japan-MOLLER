void check_data_content() {
    TFile* file = TFile::Open("4_rntuple.root", "READ");
    if (!file || file->IsZombie()) {
        cout << "Error: Could not open file 4_rntuple.root" << endl;
        return;
    }
    
    cout << "=== Data Content Analysis ===" << endl;
    
    try {
        auto reader = ROOT::RNTupleReader::Open("evt", "4_rntuple.root");
        
        // Check various fields for non-zero values
        auto cleanData = reader->GetModel().GetDefaultEntry().GetPtr<double>("Coda_CleanData");
        auto scanData1 = reader->GetModel().GetDefaultEntry().GetPtr<double>("Coda_ScanData1");
        auto scanData2 = reader->GetModel().GetDefaultEntry().GetPtr<double>("Coda_ScanData2");
        
        cout << "Checking " << reader->GetNEntries() << " events for data content..." << endl;
        
        // Statistics counters
        int clean_neg1 = 0, clean_zero = 0, clean_pos = 0;
        int scan1_nonzero = 0, scan2_nonzero = 0;
        double max_scan1 = 0, max_scan2 = 0;
        
        // Sample first 100 events and last 100 events
        vector<int> sample_indices;
        for (int i = 0; i < min(100, (int)reader->GetNEntries()); i++) {
            sample_indices.push_back(i);
        }
        for (int i = max(100, (int)reader->GetNEntries()-100); i < (int)reader->GetNEntries(); i++) {
            sample_indices.push_back(i);
        }
        
        for (int idx : sample_indices) {
            reader->LoadEntry(idx);
            
            if (*cleanData < 0) clean_neg1++;
            else if (*cleanData == 0) clean_zero++;
            else clean_pos++;
            
            if (*scanData1 != 0) {
                scan1_nonzero++;
                max_scan1 = max(max_scan1, abs(*scanData1));
            }
            if (*scanData2 != 0) {
                scan2_nonzero++;
                max_scan2 = max(max_scan2, abs(*scanData2));
            }
        }
        
        cout << "\nCoda_CleanData analysis:" << endl;
        cout << "  Negative values: " << clean_neg1 << endl;
        cout << "  Zero values: " << clean_zero << endl;
        cout << "  Positive values: " << clean_pos << endl;
        
        cout << "\nScan Data analysis:" << endl;
        cout << "  ScanData1 non-zero: " << scan1_nonzero << " (max: " << max_scan1 << ")" << endl;
        cout << "  ScanData2 non-zero: " << scan2_nonzero << " (max: " << max_scan2 << ")" << endl;
        
        // Check if this might be a special run type
        cout << "\n--- Run Type Analysis ---" << endl;
        if (clean_neg1 == sample_indices.size()) {
            cout << "All CleanData = -1.0 → This appears to be a PEDESTAL or BACKGROUND run" << endl;
        }
        if (scan1_nonzero > 0 || scan2_nonzero > 0) {
            cout << "Non-zero scan data found → This might be a SCAN run" << endl;
        }
        
        // Check a few BPM fields to see if any have data
        cout << "\n--- Checking for any non-zero physics data ---" << endl;
        vector<string> test_fields = {
            "bpm1c20WS_hw_sum", "bpm1c20X_hw_sum", "bcm1h02a_hw_sum", 
            "target_energy_hw_sum", "phasemonitor_hw_sum"
        };
        
        for (const string& field : test_fields) {
            try {
                auto field_ptr = reader->GetModel().GetDefaultEntry().GetPtr<double>(field);
                reader->LoadEntry(0);
                cout << field << ": " << *field_ptr << endl;
            } catch (...) {
                cout << field << ": [field not found]" << endl;
            }
        }
        
    } catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    
    file->Close();
    delete file;
}
