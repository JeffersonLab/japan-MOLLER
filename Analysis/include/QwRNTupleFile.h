#ifndef __QWRNTUPLEFILE__
#define __QWRNTUPLEFILE__

// System headers
#include <typeindex>
#include <unistd.h>
#include <memory>
using std::type_info;

// ROOT headers
#include "TFile.h"
#include "TPRegexp.h"
#include "TSystem.h"

// RNTuple headers
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleWriter.hxx>
#include <ROOT/RNTupleReader.hxx>
#include <ROOT/RField.hxx>
#include <ROOT/REntry.hxx>

// Qweak headers
#include "QwOptions.h"
#include "QwLog.h"

// If one defines more than this number of words in the full ntuple,
// the results are going to get very very crazy.
#define RNTUPLE_FIELD_MAX_SIZE 25000

/**
 *  \class QwRNTuple
 *  \ingroup QwAnalysis
 *  \brief A wrapper class for an RNTuple
 *
 * This class provides the functionality to write to RNTuples using a vector
 * of doubles, similar to QwRootTree but using ROOT's new RNTuple format.
 * The vector is part of this object, as well as a pointer to the writer
 * and model. One RNTuple can have multiple QwRNTuple objects, for example
 * in tracking mode both parity and tracking detectors can be stored in the 
 * same ntuple.
 */
class QwRNTuple {

  public:

    /// Constructor with name, and description
    QwRNTuple(const std::string& name, const std::string& desc, const std::string& prefix = "")
    : fName(name), fDesc(desc), fPrefix(prefix), fType("type undefined"),
      fCurrentEvent(0), fNumEventsCycle(0), fNumEventsToSave(0), fNumEventsToSkip(0) {
      // Construct new model and entry
      ConstructNewModel();
    }

    /// Constructor with existing model
    QwRNTuple(const QwRNTuple* ntuple, const std::string& prefix = "")
    : fName(ntuple->GetName()), fDesc(ntuple->GetDesc()), fPrefix(prefix), fType("type undefined"),
      fCurrentEvent(0), fNumEventsCycle(0), fNumEventsToSave(0), fNumEventsToSkip(0) {
      QwMessage << "Existing RNTuple: " << ntuple->GetName() << ", " << ntuple->GetDesc() << QwLog::endl;
      fModel = ntuple->fModel;
      // Create entry from the model using proper API
      if (fModel) {
        fEntry = fModel->CreateEntry();
      }
    }

    /// Constructor with name, description, and object
    template < class T >
    QwRNTuple(const std::string& name, const std::string& desc, T& object, const std::string& prefix = "")
    : fName(name), fDesc(desc), fPrefix(prefix), fType("type undefined"),
      fCurrentEvent(0), fNumEventsCycle(0), fNumEventsToSave(0), fNumEventsToSkip(0) {
      // Construct new model
      ConstructNewModel();

      // Construct fields and vector
      ConstructFieldsAndVector(object);
    }

    /// Constructor with existing model, and object
    template < class T >
    QwRNTuple(const QwRNTuple* ntuple, T& object, const std::string& prefix = "")
    : fName(ntuple->GetName()), fDesc(ntuple->GetDesc()), fPrefix(prefix), fType("type undefined"),
      fCurrentEvent(0), fNumEventsCycle(0), fNumEventsToSave(0), fNumEventsToSkip(0) {
      QwMessage << "Existing RNTuple: " << ntuple->GetName() << ", " << ntuple->GetDesc() << QwLog::endl;
      fModel = ntuple->fModel;
      fEntry = ntuple->fEntry;

      // Construct fields and vector
      ConstructFieldsAndVector(object);
    }

    /// Destructor
    virtual ~QwRNTuple() { }

  private:

    /// Construct the model and entry
    void ConstructNewModel() {
      QwMessage << "New RNTuple: " << fName << ", " << fDesc << QwLog::endl;
      fModel = ROOT::RNTupleModel::Create();
    }

    /// Construct the fields and vector for generic objects
    template < class T >
    void ConstructFieldsAndVector(T& object) {
      // Reserve space for the field vector
      fVector.reserve(RNTUPLE_FIELD_MAX_SIZE);
      
      // Associate fields with vector - this will be different from TTree approach
      // We'll need the object to provide a ConstructRNTupleFields method
      std::string prefix = fPrefix;
      object.ConstructRNTupleFields(fModel, prefix, fVector, fFields);

      // Create entry from model
      fEntry = fModel->CreateEntry();

      // Store the type of object
      fType = typeid(object).name();

      // Check memory reservation
      if (fVector.size() > RNTUPLE_FIELD_MAX_SIZE) {
        QwError << "The field vector is too large: " << fVector.size() << " fields!  "
                << "The maximum size is " << RNTUPLE_FIELD_MAX_SIZE << "."
                << QwLog::endl;
        exit(-1);
      }
    }

  public:

    /// Add a field to the RNTuple model and create corresponding entry field
    template<typename T>
    void AddField(const std::string& fieldName) {
      if (!fModel) {
        QwError << "RNTuple model not initialized" << QwLog::endl;
        return;
      }
      
      // Add field to model - this creates the field in the model
      auto field = fModel->MakeField<T>(fieldName);
      
      // For compatibility with the vector interface, we need to track the field index
      // The actual field values will be set through the model's entry system
      fVector.push_back(0.0); // Initialize with default value for vector index tracking
      
      QwVerbose << "Added RNTuple field: " << fieldName << " of type " << typeid(T).name() 
                << " at index " << (fVector.size() - 1) << QwLog::endl;
    }

    /// Fill the fields for generic objects
    template < class T >
    void FillRNTupleFields(const T& object) {
      if (typeid(object).name() == fType) {
        // Fill the field vector
        object.FillRNTupleVector(fVector);
        
        // Copy vector data to RNTuple entry fields
        for (size_t i = 0; i < fVector.size() && i < fFields.size(); ++i) {
          *(fFields[i]) = fVector[i];
        }
      } else {
        QwError << "Attempting to fill RNTuple vector for type " << fType << " with "
                << "object of type " << typeid(object).name() << QwLog::endl;
        exit(-1);
      }
    }

    /// Fill the RNTuple (to be called by RNTupleWriter)
    Int_t Fill() {
      fCurrentEvent++;

      // RNTuple prescaling (similar to TTree)
      if (fNumEventsCycle > 0) {
        fCurrentEvent %= fNumEventsCycle;
        if (fCurrentEvent > fNumEventsToSave)
          return 0;
      }

      // Note: Actual filling will be done by RNTupleWriter
      // This method just tracks events for prescaling
      return 1;
    }

    /// Print the RNTuple name and description
    void Print() const {
      QwMessage << GetName() << ", " << GetType();
      if (fPrefix != "")
        QwMessage << " (prefix " << GetPrefix() << ")";
      QwMessage << QwLog::endl;
    }

    /// Get the model pointer for low level operations
    std::shared_ptr<ROOT::RNTupleModel> GetModel() const { return fModel; };

    /// Get the entry pointer for low level operations
    std::unique_ptr<ROOT::REntry>& GetEntry() { return fEntry; };

  friend class QwRNTupleFile;

  private:

    /// Model and entry pointers
    std::shared_ptr<ROOT::RNTupleModel> fModel;
    std::unique_ptr<ROOT::REntry> fEntry;
    
    /// Vector of field values (similar to TTree approach)
    std::vector<Double_t> fVector;
    
    /// Vector of field pointers for direct access
    std::vector<std::shared_ptr<Double_t>> fFields;

    /// Name, description
    const std::string fName;
    const std::string fDesc;
    const std::string fPrefix;

    /// Get the name of the RNTuple
    const std::string& GetName() const { return fName; };
    /// Get the description of the RNTuple
    const std::string& GetDesc() const { return fDesc; };
    /// Get the prefix of the RNTuple
    const std::string& GetPrefix() const { return fPrefix; };

    /// Object type
    std::string fType;

    /// Get the object type
    std::string GetType() const { return fType; };

    /// RNTuple prescaling parameters
    UInt_t fCurrentEvent;
    UInt_t fNumEventsCycle;
    UInt_t fNumEventsToSave;
    UInt_t fNumEventsToSkip;

    /// Set RNTuple prescaling parameters
    void SetPrescaling(UInt_t num_to_save, UInt_t num_to_skip) {
      fNumEventsToSave = num_to_save;
      fNumEventsToSkip = num_to_skip;
      fNumEventsCycle = fNumEventsToSave + fNumEventsToSkip;
    }
};

/**
 *  \class QwRNTupleFile
 *  \ingroup QwAnalysis
 *  \brief A wrapper class for RNTuple files
 *
 * This class functions as a wrapper around ROOT RNTuple functionality,
 * providing an interface similar to QwRootFile but using the new RNTuple
 * format. It manages multiple RNTuples in a single ROOT file.
 *
 * The functionality of writing to the file is done by templated functions.
 * The objects that are passed to these functions have to provide the following
 * functions:
 * <ul>
 * <li>ConstructHistograms, FillHistograms (same as TTree version)
 * <li>ConstructRNTupleFields, FillRNTupleVector (new for RNTuple)
 * </ul>
 */
class QwRNTupleFile {

  public:

    /// \brief Constructor with run label
    QwRNTupleFile(const TString& run_label);
    /// \brief Destructor
    virtual ~QwRNTupleFile();

    /// \brief Define the configuration options
    static void DefineOptions(QwOptions &options);
    /// \brief Process the configuration options
    void ProcessOptions(QwOptions &options);

    /// Is the ROOT file active?
    Bool_t IsRootFile() const { return (fRootFile != nullptr); };

    /// \brief Construct the RNTuple fields of a generic object
    template < class T >
    void ConstructRNTupleFields(const std::string& name, const std::string& desc, T& object, const std::string& prefix = "");

    /// \brief Fill the RNTuple fields of a generic object by name
    template < class T >
    void FillRNTupleFields(const std::string& name, const T& object);

    /// \brief Fill the RNTuple fields of a generic object by type only
    template < class T >
    void FillRNTupleFields(const T& object);

    /// \brief Construct the histograms of a generic object (same as QwRootFile)
    template < class T >
    void ConstructHistograms(const std::string& name, T& object);

    /// Fill histograms of the subsystem array (same as QwRootFile)
    template < class T >
    void FillHistograms(T& object) {
      // Update regularly
      static Int_t update_count = 0;
      update_count++;
      if ((fUpdateInterval > 0) && ( update_count % fUpdateInterval == 0)) Update();
      if (! HasDirByType(object)) return;
      // Fill histograms
      object.FillHistograms();
    }

    /// Create a new RNTuple with name and description
    void NewRNTuple(const std::string& name, const std::string& desc) {
      if (IsRNTupleDisabled(name)) return;
      this->cd();
      QwRNTuple *ntuple = 0;
      if (! HasRNTupleByName(name)) {
        ntuple = new QwRNTuple(name, desc);
      } else {
        ntuple = new QwRNTuple(fRNTupleByName[name].front());
      }
      fRNTupleByName[name].push_back(ntuple);
    }

    /// Fill the RNTuple with name
    Int_t FillRNTuple(const std::string& name) {
      if (! HasRNTupleByName(name)) return 0;
      // Fill all registered objects for this RNTuple
      std::vector<QwRNTuple*>& ntuples = fRNTupleByName[name];
      for (auto* ntuple : ntuples) {
        ntuple->Fill();
      }
      // Actually write to RNTuple using the writer
      if (fRNTupleWriters.find(name) != fRNTupleWriters.end()) {
        return fRNTupleWriters[name]->Fill();
      }
      return 0;
    }

    /// Fill all registered RNTuples
    Int_t FillRNTuples() {
      Int_t retval = 0;
      for (auto& pair : fRNTupleByName) {
        retval += FillRNTuple(pair.first);
      }
      return retval;
    }

    /// Print registered RNTuples
    void PrintRNTuples() const {
      QwMessage << "RNTuples: " << QwLog::endl;
      for (const auto& pair : fRNTupleByName) {
        QwMessage << pair.first << ": " << pair.second.size()
                  << " objects registered" << QwLog::endl;
        for (const auto* ntuple : pair.second) {
          ntuple->Print();
        }
      }
    }

    /// Write any object to the ROOT file (only valid for TFile)
    template < class T >
    Int_t WriteObject(const T* obj, const char* name, Option_t* option = "", Int_t bufsize = 0) {
      Int_t retval = 0;
      if (fRootFile) retval = fRootFile->WriteObject(obj, name, option, bufsize);
      return retval;
    }

    // Wrapped functionality
    void Update() {
      if (fRootFile) {
        fRootFile->Write();
        QwMessage << "RNTuple file updated" << QwLog::endl;
      }
    }

    void Write() {
      if (fRootFile) {
        // Commit all RNTuple writers
        for (auto& pair : fRNTupleWriters) {
          if (pair.second) {
            pair.second->CommitDataset();
          }
        }
        fRootFile->Write();
        QwMessage << "RNTuple file written" << QwLog::endl;
      }
    }

    void Close() {
      Write();
      if (fRootFile) {
        fRootFile->Close();
        delete fRootFile;
        fRootFile = nullptr;
      }
    }

    void cd() {
      if (fRootFile) fRootFile->cd();
    }

  private:

    /// Check if RNTuple name is registered
    Bool_t HasRNTupleByName(const std::string& name) const {
      return fRNTupleByName.find(name) != fRNTupleByName.end();
    }

    /// Check if RNTuple is disabled
    Bool_t IsRNTupleDisabled(const std::string& name) const {
      return fDisabledRNTuples.find(name) != fDisabledRNTuples.end();
    }

    /// Check if directory exists by type
    template < class T >
    Bool_t HasDirByType(const T& object) const {
      return fDirsByType.find(std::type_index(typeid(object))) != fDirsByType.end();
    }

    /// ROOT file pointer
    TFile* fRootFile;

    /// RNTuple containers
    std::map<const std::string, std::vector<QwRNTuple*>> fRNTupleByName;
    std::map<const std::string, std::unique_ptr<ROOT::RNTupleWriter>> fRNTupleWriters;
    std::set<std::string> fDisabledRNTuples;

    /// Directory management (same as QwRootFile)
    std::map<const std::string, TDirectory*> fDirsByName;
    std::map<std::type_index, TDirectory*> fDirsByType;

    /// File management
    std::string fRunLabel;
    static std::string fDefaultRootFileDir;
    static std::string fDefaultRootFileStem;
    
    /// Update interval for histograms
    Int_t fUpdateInterval;

    /// Helper method to create RNTuple writers
    void CreateRNTupleWriter(const std::string& name) {
      if (fRNTupleWriters.find(name) == fRNTupleWriters.end() && 
          HasRNTupleByName(name) && !fRNTupleByName[name].empty()) {
        auto source_model = fRNTupleByName[name].front()->GetModel();
        if (source_model && fRootFile) {
          try {
            // Create a new model with same structure for the writer
            std::unique_ptr<ROOT::RNTupleModel> writer_model = ROOT::RNTupleModel::Create();
            
            // Create writer with the new model
            fRNTupleWriters[name] = std::unique_ptr<ROOT::RNTupleWriter>(
              ROOT::RNTupleWriter::Append(std::move(writer_model), name, *fRootFile).release());
            
            QwMessage << "RNTuple writer created for " << name << QwLog::endl;
          } catch (const std::exception& e) {
            QwError << "Exception creating RNTuple writer for " << name << ": " 
                   << e.what() << QwLog::endl;
          }
        }
      }
    }
};

// Template implementations

template < class T >
void QwRNTupleFile::ConstructRNTupleFields(const std::string& name, const std::string& desc, T& object, const std::string& prefix) {
  if (IsRNTupleDisabled(name)) return;
  this->cd();
  
  QwRNTuple *ntuple = 0;
  if (! HasRNTupleByName(name)) {
    ntuple = new QwRNTuple(name, desc, object, prefix);
    fRNTupleByName[name].push_back(ntuple);
  } else {
    ntuple = new QwRNTuple(fRNTupleByName[name].front(), object, prefix);
    fRNTupleByName[name].push_back(ntuple);
  }
  
  // Create writer if this is the first object for this RNTuple
  CreateRNTupleWriter(name);
}

template < class T >
void QwRNTupleFile::FillRNTupleFields(const std::string& name, const T& object) {
  if (! HasRNTupleByName(name)) return;
  
  // Find the matching QwRNTuple for this object type
  for (auto* ntuple : fRNTupleByName[name]) {
    if (ntuple->GetType() == typeid(object).name()) {
      ntuple->FillRNTupleFields(object);
      return;
    }
  }
  
  QwError << "No matching RNTuple found for object type " << typeid(object).name() 
          << " in RNTuple " << name << QwLog::endl;
}

template < class T >
void QwRNTupleFile::FillRNTupleFields(const T& object) {
  // Find RNTuple by object type alone
  for (auto& pair : fRNTupleByName) {
    for (auto* ntuple : pair.second) {
      if (ntuple->GetType() == typeid(object).name()) {
        ntuple->FillRNTupleFields(object);
        return;
      }
    }
  }
  
  QwError << "No matching RNTuple found for object type " << typeid(object).name() << QwLog::endl;
}

template < class T >
void QwRNTupleFile::ConstructHistograms(const std::string& name, T& object) {
  // Same implementation as QwRootFile - histograms are independent of RNTuple
  this->cd();
  
  // Create directory if it doesn't exist
  TDirectory* dir = nullptr;
  if (fDirsByName.find(name) == fDirsByName.end()) {
    dir = fRootFile->mkdir(name.c_str());
    fDirsByName[name] = dir;
  } else {
    dir = fDirsByName[name];
  }
  
  // Store directory by type for quick lookup
  fDirsByType[std::type_index(typeid(object))] = dir;
  
  dir->cd();
  object.ConstructHistograms();
  this->cd();
}

#endif // __QWRNTUPLEFILE__
