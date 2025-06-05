#ifndef __QWRNTUPLEFILE__
#define __QWRNTUPLEFILE__

// System headers
#include <typeindex>
#include <unistd.h>
#include <memory>
#include <type_traits>
#include <exception>
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

// Qweak headers
#include "QwOptions.h"
#include "QwLog.h"

// If one defines more than this number of words in the full ntuple,
// the results are going to get very very crazy.
#define RNTUPLE_FIELD_MAX_SIZE 25000

/**
 *  \class QwRNTupleFieldBuilder
 *  \ingroup QwAnalysis
 *  \brief A bridge class to adapt the QwRNTuple interface for existing classes
 *
 * This class acts as a bridge between the QwRNTuple interface expected by 
 * existing classes (with AddField method) and the actual QwRNTuple object
 * that uses ROOT's RNTuple model.
 */
class QwRNTupleFieldBuilder {
  private:
    class QwRNTuple* fRNTuple;
    
  public:
    QwRNTupleFieldBuilder(class QwRNTuple* rntuple) : fRNTuple(rntuple) {}
    
    template<typename T>
    void AddField(const std::string& fieldName);
};

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
      
      // Use the correct method signature that actually exists in the codebase
      TString prefix_tstring(fPrefix.c_str());
      
      // Call the object's ConstructRNTupleFields method directly with this QwRNTuple
      object.ConstructRNTupleFields(this, prefix_tstring);

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
      
      QwVerbose << "Adding RNTuple field: " << fieldName << " of type " << typeid(T).name() << QwLog::endl;
      
      try {
        // Add field to model - this creates the field in the model and returns a shared_ptr<T>
        auto field = fModel->MakeField<T>(fieldName);
        
        if (!field) {
          QwError << "Failed to create field " << fieldName << " - MakeField returned null" << QwLog::endl;
          return;
        }
        
        // Store the actual field pointer to keep it alive
        fFieldPtrs.push_back(std::static_pointer_cast<void>(field));
        
        // Store field name for mapping
        fFieldNames.push_back(fieldName);
        
        // Store field type for type-aware filling
        fFieldTypes.push_back(typeid(T).name());
        
        // For Double_t fields, also store in the typed container
        if constexpr (std::is_same_v<T, Double_t>) {
          fFields.push_back(field);
        } else {
          // For non-Double_t types, create a placeholder for vector compatibility
          fFields.push_back(std::make_shared<Double_t>(0.0));
        }
        
        // For compatibility with the vector interface, we need to track the field index
        fVector.push_back(0.0); // Initialize with default value for vector index tracking
        
        QwVerbose << "Successfully added RNTuple field: " << fieldName << " of type " << typeid(T).name() 
                  << " at index " << (fVector.size() - 1) << QwLog::endl;
      } catch (const std::exception& e) {
        QwError << "Exception adding field " << fieldName << ": " << e.what() << QwLog::endl;
      }
    }

    /// Fill the fields for generic objects
    template < class T >
    void FillRNTupleFields(const T& object) {
      if (typeid(object).name() == fType) {
        // Fill the field vector using the object's FillRNTupleVector method
        object.FillRNTupleVector(fVector);
        
        // The actual data writing is handled by the RNTupleWriter when Fill() is called
        // The vector data will be copied to entry fields at that time
      } else {
        QwVerbose << "Type mismatch in FillRNTupleFields. Expected: " << fType 
                  << ", Got: " << typeid(object).name() << QwLog::endl;
        // For now, try to fill anyway since the object should have the method
        object.FillRNTupleVector(fVector);
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

    /// Get the vector for data transfer
    std::vector<Double_t>& GetVector() { return fVector; };

    /// Get the fields for data transfer
    std::vector<std::shared_ptr<Double_t>>& GetFields() { return fFields; };

    /// Get the field names for data transfer
    const std::vector<std::string>& GetFieldNames() const { return fFieldNames; }
    
    /// Get the field types for type-aware filling
    const std::vector<std::string>& GetFieldTypes() const { return fFieldTypes; };

  friend class QwRNTupleFile;

  private:

    /// Model and entry pointers
    std::shared_ptr<ROOT::RNTupleModel> fModel;
    
    /// Vector of field values (similar to TTree approach)
    std::vector<Double_t> fVector;
    
    /// Vector of field pointers for direct access
    std::vector<std::shared_ptr<Double_t>> fFields;
    
    /// Vector of field pointers (type-erased) to keep them alive
    std::vector<std::shared_ptr<void>> fFieldPtrs;
    
    /// Field name to index mapping for data transfer
    std::vector<std::string> fFieldNames;
    
    /// Field type information for type-aware filling
    std::vector<std::string> fFieldTypes;

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

// Template implementation for QwRNTupleFieldBuilder
template<typename T>
void QwRNTupleFieldBuilder::AddField(const std::string& fieldName) {
  if (fRNTuple) {
    fRNTuple->AddField<T>(fieldName);
  }
}

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
      if (fRNTupleWriters.find(name) != fRNTupleWriters.end() && 
          fConsolidatedFields.find(name) != fConsolidatedFields.end()) {
        
        // Collect data from all registered ntuples for this RNTuple
        if (!ntuples.empty()) {
          try {
            // Transfer data from QwRNTuple vectors to the writer's consolidated fields
            const auto& consolidated_fields = fConsolidatedFields[name];
            
            for (auto* ntuple : ntuples) {
              const auto& vector = ntuple->GetVector();
              const auto& field_names = ntuple->GetFieldNames();
              const auto& field_types = ntuple->GetFieldTypes();
              
              // Transfer each field's data to the corresponding consolidated field
              for (size_t i = 0; i < std::min({vector.size(), field_names.size(), field_types.size()}); ++i) {
                const std::string& field_name = field_names[i];
                const std::string& field_type = field_types[i];
                
                auto field_it = consolidated_fields.find(field_name);
                if (field_it != consolidated_fields.end()) {
                  // Cast back to the correct type and assign the value
                  if (field_type == typeid(Double_t).name()) {
                    auto double_field = std::static_pointer_cast<Double_t>(field_it->second);
                    if (double_field) {
                      *double_field = vector[i];
                      QwVerbose << "Filled field " << field_name << " with value " << vector[i] << QwLog::endl;
                    }
                  } else if (field_type == typeid(Float_t).name()) {
                    auto float_field = std::static_pointer_cast<Float_t>(field_it->second);
                    if (float_field) {
                      *float_field = static_cast<Float_t>(vector[i]);
                    }
                  } else if (field_type == typeid(Int_t).name()) {
                    auto int_field = std::static_pointer_cast<Int_t>(field_it->second);
                    if (int_field) {
                      *int_field = static_cast<Int_t>(vector[i]);
                    }
                  }
                  // Add more types as needed
                } else {
                  QwVerbose << "Field " << field_name << " not found in consolidated fields" << QwLog::endl;
                }
              }
            }
            
            // Fill the entry - the writer will use the consolidated field pointers
            return fRNTupleWriters[name]->Fill();
            
          } catch (const std::exception& e) {
            QwError << "Exception in FillRNTuple for " << name << ": " << e.what() << QwLog::endl;
          }
        }
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
    std::map<const std::string, std::map<std::string, std::shared_ptr<void>>> fConsolidatedFields;
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
        if (fRootFile) {
          try {
            QwMessage << "Creating RNTuple writer for " << name 
                      << " using consolidated model to file " << fRootFile->GetName() << QwLog::endl;
            
            // Create a new model for the writer since ROOT requires unique ownership
            auto writer_model = ROOT::RNTupleModel::Create();
            
            // Consolidate all field definitions from QwRNTuple objects into the writer's model
            // This is necessary because RNTupleWriter requires exclusive ownership of the model
            std::map<std::string, std::shared_ptr<void>> consolidated_fields;
            
            for (auto* qw_ntuple : fRNTupleByName[name]) {
              const auto& field_names = qw_ntuple->GetFieldNames();
              const auto& field_types = qw_ntuple->GetFieldTypes();
              
              for (size_t i = 0; i < field_names.size(); ++i) {
                const std::string& field_name = field_names[i];
                const std::string& field_type = field_types[i];
                
                // Avoid duplicate fields (multiple QwRNTuple objects may define the same fields)
                if (consolidated_fields.find(field_name) == consolidated_fields.end()) {
                  // Add field to writer model based on type
                  if (field_type == typeid(Double_t).name()) {
                    auto field_ptr = writer_model->MakeField<Double_t>(field_name);
                    consolidated_fields[field_name] = std::static_pointer_cast<void>(field_ptr);
                  } else if (field_type == typeid(Float_t).name()) {
                    auto field_ptr = writer_model->MakeField<Float_t>(field_name);
                    consolidated_fields[field_name] = std::static_pointer_cast<void>(field_ptr);
                  } else if (field_type == typeid(Int_t).name()) {
                    auto field_ptr = writer_model->MakeField<Int_t>(field_name);
                    consolidated_fields[field_name] = std::static_pointer_cast<void>(field_ptr);
                  }
                  // Add more types as needed
                  QwVerbose << "Added field " << field_name << " of type " << field_type 
                           << " to writer model" << QwLog::endl;
                }
              }
            }
            
            // Store consolidated field pointers for data transfer during Fill operations
            fConsolidatedFields[name] = std::move(consolidated_fields);
            
            // Now create the writer with the properly constructed model
            // ROOT::RNTupleWriter takes unique ownership of the model via std::move
            fRNTupleWriters[name] = ROOT::RNTupleWriter::Append(std::move(writer_model), name, *fRootFile);
            
            QwMessage << "RNTuple writer created for " << name 
                      << " writing to " << fRootFile->GetName() << QwLog::endl;
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
  
  // Try to find the matching QwRNTuple for this object type
  for (auto* ntuple : fRNTupleByName[name]) {
    if (ntuple->GetType() == typeid(object).name()) {
      ntuple->FillRNTupleFields(object);
      return;
    }
  }
  
  // If no exact match found, try using the first available RNTuple
  // since they should all have compatible interfaces
  if (!fRNTupleByName[name].empty()) {
    QwVerbose << "Using first available RNTuple for object type " << typeid(object).name() 
              << " in RNTuple " << name << QwLog::endl;
    fRNTupleByName[name][0]->FillRNTupleFields(object);
    return;
  }
  
  QwError << "No RNTuple found for object type " << typeid(object).name() 
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
  
  // If no exact match found, try using the first available RNTuple in each group
  for (auto& pair : fRNTupleByName) {
    if (!pair.second.empty()) {
      QwVerbose << "Using first available RNTuple in " << pair.first 
                << " for object type " << typeid(object).name() << QwLog::endl;
      pair.second[0]->FillRNTupleFields(object);
      return;
    }
  }
  
  QwError << "No RNTuple found for object type " << typeid(object).name() << QwLog::endl;
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
