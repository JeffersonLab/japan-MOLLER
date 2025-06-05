#include <iostream>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RField.hxx>
#include <TFile.h>
#include <TKey.h>
#include <TList.h>

void inspect_fields() {
    // Open the ROOT file
    TFile* file = TFile::Open("4_rntuple.root", "READ");
    if (!file || file->IsZombie()) {
        std::cout << "Error: Could not open file 4_rntuple.root" << std::endl;
        return;
    }
    
    std::cout << "=== RNTuple Field Analysis ===" << std::endl;
    std::cout << "File: 4_rntuple.root" << std::endl << std::endl;
    
    // Get list of keys (RNTuples)
    TList* keys = file->GetListOfKeys();
    TIter keyIter(keys);
    TKey* key;
    
    while ((key = (TKey*)keyIter())) {
        std::string keyName = key->GetName();
        std::string className = key->GetClassName();
        
        // Check if this is an RNTuple
        if (className.find("RNTuple") != std::string::npos) {
            std::cout << "--- RNTuple: " << keyName << " ---" << std::endl;
            
            try {
                // Open the RNTuple reader
                auto reader = ROOT::RNTupleReader::Open(keyName, "4_rntuple.root");
                
                // Get the descriptor to access field information
                const auto& descriptor = reader->GetDescriptor();
                
                // Print basic info
                std::cout << "  Entries: " << reader->GetNEntries() << std::endl;
                std::cout << "  Fields: " << std::endl;
                
                // Get the top-level field descriptor and iterate through its sub-fields
                const auto& topFieldDesc = descriptor.GetFieldDescriptor(descriptor.GetFieldZeroId());
                auto fieldIterable = descriptor.GetFieldIterable(topFieldDesc);
                
                // Iterate through all fields
                for (const auto& fieldDesc : fieldIterable) {
                    std::cout << "    - " << fieldDesc.GetFieldName() 
                             << " (Type: " << fieldDesc.GetTypeName() << ")" << std::endl;
                }
                
                std::cout << std::endl;
                
            } catch (const std::exception& e) {
                std::cout << "  Error reading RNTuple: " << e.what() << std::endl << std::endl;
            }
        }
    }
    
    file->Close();
    delete file;
}
