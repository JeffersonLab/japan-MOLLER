void simple_examine() {
    // Simple examination of RNTuple file
    auto file = TFile::Open("4_rntuple.root");
    std::cout << "File contents:" << std::endl;
    file->ls();
    
    // Try to get basic info about one RNTuple
    std::cout << "\nTrying to read basic info..." << std::endl;
    auto keys = file->GetListOfKeys();
    for (int i = 0; i < keys->GetEntries(); i++) {
        auto key = (TKey*)keys->At(i);
        std::cout << "Key " << i << ": " << key->GetName() << " (" << key->GetClassName() << ")" << std::endl;
    }
    
    file->Close();
}
