#include <ROOT/RNTupleInspector.hxx>
#include <iostream>

void examine_rntuple() {
    using namespace ROOT::Experimental;
    
    // List all RNTuple names in the file
    auto file = TFile::Open("4_rntuple.root");
    std::cout << "RNTuples in file:" << std::endl;
    file->ls();
    
    // Examine each RNTuple using Inspector
    std::vector<std::string> rntuple_names = {"evt", "evts", "mul", "muls", "pr", "bursts", "mulc_lrb", "burst_mulc_lrb"};
    
    for (const auto& name : rntuple_names) {
        try {
            std::cout << "\n=== RNTuple: " << name << " ===" << std::endl;
            auto inspector = RNTupleInspector::Create(name, "4_rntuple.root");
            std::cout << "Number of entries: " << inspector->GetDescriptor().GetNEntries() << std::endl;
            
            std::cout << "Top-level fields:" << std::endl;
            for (auto field : inspector->GetDescriptor().GetTopLevelFields()) {
                std::cout << "  - " << field.GetFieldName() << " (" << field.GetTypeName() << ")" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error reading " << name << ": " << e.what() << std::endl;
        }
    }
    
    file->Close();
}
