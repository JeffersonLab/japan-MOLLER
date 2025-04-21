#ifndef __QWROOTFILE__
#define __QWROOTFILE__

// System headers
#include <typeindex>
#include <unistd.h>
using std::type_info;

// ROOT headers
#include "TFile.h"
#include "ROOT/RNTuple.hxx"
#include "ROOT/RNTupleModel.hxx"
#include "ROOT/RField.hxx"
#include "ROOT/RNTupleWriter.hxx"
#include "ROOT/RNTupleReader.hxx"
#include "TPRegexp.h"
#include "TSystem.h"

// Qweak headers
#include "QwOptions.h"
#include "TMapFile.h"


// If one defines more than this number of words in the full ntuple,
// the results are going to get very very crazy.
#define BRANCH_VECTOR_MAX_SIZE 25000


/**
 *  \class QwRootTree
 *  \ingroup QwAnalysis
 *  \brief A wrapper class for a ROOT RNTuple
 *
 * This class provides the functionality to write to ROOT RNTuples using a vector
 * of doubles. The vector is part of this object, as well as a pointer to the
 * RNTuple model and writer that contains the fields. One ROOT RNTuple can have 
 * multiple QwRootTree objects, for example in tracking mode both parity and 
 * tracking detectors can be stored in the same RNTuple.
 */
class QwRootTree {

  public:

    /// Constructor with name, and description
    QwRootTree(const std::string& name, const std::string& desc, const std::string& prefix = "")
    : fName(name), fDesc(desc), fPrefix(prefix), fType("type undefined"),
      fCurrentEvent(0), fNumEventsCycle(0), fNumEventsToSave(0), fNumEventsToSkip(0) {
      // Construct RNTuple
      ConstructNewRNTuple();
    }

    /// Constructor with existing RNTuple
    QwRootTree(const QwRootTree* tree, const std::string& prefix = "")
    : fName(tree->GetName()), fDesc(tree->GetDesc()), fPrefix(prefix), fType("type undefined"),
      fCurrentEvent(0), fNumEventsCycle(0), fNumEventsToSave(0), fNumEventsToSkip(0) {
      QwMessage << "Existing RNTuple: " << tree->GetName() << ", " << tree->GetDesc() << QwLog::endl;
      fModel = tree->fModel;
      fWriter = tree->fWriter;
    }

    /// Constructor with name, description, and object
    template < class T >
    QwRootTree(const std::string& name, const std::string& desc, T& object, const std::string& prefix = "")
    : fName(name), fDesc(desc), fPrefix(prefix), fType("type undefined"),
      fCurrentEvent(0), fNumEventsCycle(0), fNumEventsToSave(0), fNumEventsToSkip(0) {
      // Construct RNTuple
      ConstructNewRNTuple();

      // Construct units field
      ConstructUnitsField();

      // Construct fields and vector
      ConstructFieldAndVector(object);
    }

    /// Constructor with existing RNTuple, and object
    template < class T >
    QwRootTree(const QwRootTree* tree, T& object, const std::string& prefix = "")
    : fName(tree->GetName()), fDesc(tree->GetDesc()), fPrefix(prefix), fType("type undefined"),
      fCurrentEvent(0), fNumEventsCycle(0), fNumEventsToSave(0), fNumEventsToSkip(0) {
      QwMessage << "Existing RNTuple: " << tree->GetName() << ", " << tree->GetDesc() << QwLog::endl;
      fModel = tree->fModel;
      fWriter = tree->fWriter;

      // Construct fields and vector
      ConstructFieldAndVector(object);
    }

    /// Destructor
    virtual ~QwRootTree() { }


  private:

    static const TString kUnitsName;
    static Double_t kUnitsValue[];

    /// Construct the RNTuple
    void ConstructNewRNTuple() {
      QwMessage << "New RNTuple: " << fName << ", " << fDesc << QwLog::endl;
      fModel = ROOT::Experimental::RNTupleModel::Create();
    }

    void ConstructUnitsField() {
      fPpmField = fModel->MakeField<Double_t>("ppm");
      fPpbField = fModel->MakeField<Double_t>("ppb");
      fUmField = fModel->MakeField<Double_t>("um");
      fMmField = fModel->MakeField<Double_t>("mm");
      fMv_uaField = fModel->MakeField<Double_t>("mV_uA");
      fV_uaField = fModel->MakeField<Double_t>("V_uA");
      
      *fPpmField = kUnitsValue[0];
      *fPpbField = kUnitsValue[1];
      *fUmField = kUnitsValue[2];
      *fMmField = kUnitsValue[3];
      *fMv_uaField = kUnitsValue[4];
      *fV_uaField = kUnitsValue[5];
    }

    /// Construct index from this RNTuple to another RNTuple
    void ConstructIndexTo(QwRootTree* to) {
      std::string name = "previous_entry_in_" + to->fName;
      fModel->MakeField<UInt_t>(name, &(to->fCurrentEvent));
    }

    /// Construct the fields and vector for generic objects
    template < class T >
    void ConstructFieldAndVector(T& object) {
      // Reserve space for the field vector
      fVector.reserve(BRANCH_VECTOR_MAX_SIZE);
      // Associate fields with vector
      TString prefix = Form("%s", fPrefix.c_str());
      object.ConstructBranchAndVector(fModel, prefix, fVector);

      // Store the type of object
      fType = typeid(object).name();

      // Check memory reservation
      if (fVector.size() > BRANCH_VECTOR_MAX_SIZE) {
        QwError << "The field vector is too large: " << fVector.size() << " leaves!  "
                << "The maximum size is " << BRANCH_VECTOR_MAX_SIZE << "."
                << QwLog::endl;
        exit(-1);
      }
    }
   

  public:

    /// Fill the fields for generic objects
    template < class T >
    void FillTreeBranches(const T& object) {
      if (typeid(object).name() == fType) {
        // Fill the field vector
        object.FillTreeVector(fVector);
      } else {
        QwError << "Attempting to fill RNTuple vector for type " << fType << " with "
                << "object of type " << typeid(object).name() << QwLog::endl;
        exit(-1);
      }
    }

    void SetWriter(ROOT::Experimental::RNTupleWriter* writer) {
      fWriter = writer;
    }

    Long64_t AutoSave(Option_t *option = ""){
      // RNTuple doesn't have an AutoSave method, but we can flush the writer
      if (fWriter) {
        fWriter->Flush();
        return fWriter->GetNEntries();
      }
      return 0;
    }

    /// Fill the RNTuple
    Int_t Fill() {
      fCurrentEvent++;

      // RNTuple prescaling
      if (fNumEventsCycle > 0) {
        fCurrentEvent %= fNumEventsCycle;
        if (fCurrentEvent > fNumEventsToSave)
          return 0;
      }

      // Fill the RNTuple
      if (fWriter) {
        fWriter->Fill();
        return 1; // Success
      }
      return 0;
    }


    /// Print the RNTuple name and description
    void Print() const {
      QwMessage << GetName() << ", " << GetType();
      if (fPrefix != "")
        QwMessage << " (prefix " << GetPrefix() << ")";
      QwMessage << QwLog::endl;
    }

    /// Get the RNTuple model for low level operations
    ROOT::Experimental::RNTupleModel* GetModel() const { return fModel.get(); }


  friend class QwRootFile;

  private:

    /// RNTuple model and writer
    std::unique_ptr<ROOT::Experimental::RNTupleModel> fModel;
    ROOT::Experimental::RNTupleWriter* fWriter = nullptr; // We don't own this

    /// Units fields
    std::shared_ptr<Double_t> fPpmField;
    std::shared_ptr<Double_t> fPpbField;
    std::shared_ptr<Double_t> fUmField;
    std::shared_ptr<Double_t> fMmField;
    std::shared_ptr<Double_t> fMv_uaField;
    std::shared_ptr<Double_t> fV_uaField;

    /// Vector of leaves
    std::vector<Double_t> fVector;

    /// Name, description
    const std::string fName;
    const std::string fDesc;
    const std::string fPrefix;

    /// Get the name of the RNTuple
    const std::string& GetName() const { return fName; };
    /// Get the description of the RNTuple
    const std::string& GetDesc() const { return fDesc; };
    /// Get the description of the RNTuple
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

    /// RNTuple configuration parameters
    // Note: RNTuple doesn't have direct equivalents for all TTree parameters, we'll simulate where possible
    Long64_t fMaxNTupleSize;
    Long64_t fAutoFlush;
    Long64_t fAutoSave;
    Int_t fCompressionSettings;

    /// Set maximum RNTuple size (not directly supported, but we'll keep the parameter)
    void SetMaxNTupleSize(Long64_t maxsize = 1900000000) {
      fMaxNTupleSize = maxsize;
    }

    /// Set autoflush size (we'll use this to flush the writer periodically)
    void SetAutoFlush(Long64_t autoflush = 30000000) {
      fAutoFlush = autoflush;
    }

    /// Set autosave size (not directly supported, but we'll keep the parameter)
    void SetAutoSave(Long64_t autosave = 300000000) {
      fAutoSave = autosave;
    }

    /// Set compression settings
    void SetCompressionSettings(Int_t settings = ROOT::RCompressionSetting::EDfaults::kUseCompiledDefault) {
      fCompressionSettings = settings;
    }

    // RNTuple doesn't support circular buffer directly, but we'll include this for API compatibility
    void SetCircular(Long64_t buff = 100000) {
      // No direct equivalent in RNTuple
    }
};



/**
 *  \class QwRootFile
 *  \ingroup QwAnalysis
 *  \brief A wrapper class for a ROOT file or memory mapped file
 *
 * This class functions as a wrapper around a ROOT TFile or a TMapFile.  The
 * common inheritance of both is only TObject, so there is a lot that we have
 * to wrap (rather than inherit).  Theoretically you could have both a TFile
 * and a TMapFile represented by an object of this class at the same time, but
 * that is untested.
 *
 * The functionality of writing to the file is done by templated functions.
 * The objects that are passed to these functions have to provide the following
 * functions:
 * <ul>
 * <li>ConstructHistograms, FillHistograms
 * <li>ConstructBranchAndVector, FillTreeVector
 * </ul>
 *
 * The class keeps track of the registered RNTuple names, and the types of objects
 * that have fields constructed in those RNTuples (via QwRootTree).  In most
 * cases it should be possible to just call FillTreeBranches with only the object,
 * although in rare cases this could be ambiguous.
 *
 * The proper way to register an RNTuple is by either calling ConstructTreeBranches
 * of NewTree first.  Then FillTreeBranches will fill the vector, and FillTree
 * will actually fill the RNTuple.  FillTree should be called only once.
 */
class QwRootFile {

  public:

    /// \brief Constructor with run label
    QwRootFile(const TString& run_label);
    /// \brief Destructor
    virtual ~QwRootFile();


    /// \brief Define the configuration options
    static void DefineOptions(QwOptions &options);
    /// \brief Process the configuration options
    void ProcessOptions(QwOptions &options);
    /// \brief Set default ROOT files dir
    static void SetDefaultRootFileDir(const std::string& dir) {
      fDefaultRootFileDir = dir;
    }
    /// \brief Set default ROOT file stem
    static void SetDefaultRootFileStem(const std::string& stem) {
      fDefaultRootFileStem = stem;
    }


    /// Is the ROOT file active?
    Bool_t IsRootFile() const { return (fRootFile); };
    /// Is the map file active?
    Bool_t IsMapFile()  const { return (fMapFile); };

    /// \brief Construct indices from one RNTuple to another RNTuple
    void ConstructIndices(const std::string& from, const std::string& to, bool reverse = true);

    /// \brief Construct the RNTuple fields of a generic object
    template < class T >
    void ConstructTreeBranches(const std::string& name, const std::string& desc, T& object, const std::string& prefix = "");
    /// \brief Fill the RNTuple fields of a generic object by RNTuple name
    template < class T >
    void FillTreeBranches(const std::string& name, const T& object);
    /// \brief Fill the RNTuple fields of a generic object by type only
    template < class T >
    void FillTreeBranches(const T& object);


    template < class T >
    Int_t WriteParamFileList(const TString& name, T& object);


    /// \brief Construct the histograms of a generic object
    template < class T >
    void ConstructObjects(const std::string& name, T& object);

    /// \brief Construct the histograms of a generic object
    template < class T >
    void ConstructHistograms(const std::string& name, T& object);
    /// Fill histograms of the subsystem array
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
    void NewTree(const std::string& name, const std::string& desc) {
      if (IsTreeDisabled(name)) return;
      this->cd();
      QwRootTree* tree = nullptr;
      if (!HasTreeByName(name)) {
        tree = new QwRootTree(name, desc);
        
        // Create and set the RNTuple writer
        if (fRootFile) {
          auto writer = ROOT::Experimental::RNTupleWriter::Recreate(tree->GetModel(), name, *fRootFile);
          tree->SetWriter(writer.get());
          fRNTupleWriters[name] = std::move(writer);
        }
      } else {
        tree = new QwRootTree(fTreeByName[name].front());
      }
      fTreeByName[name].push_back(tree);
    }

    /// Get the RNTuple model with name
    ROOT::Experimental::RNTupleModel* GetRNTupleModel(const std::string& name) {
      if (!HasTreeByName(name)) return nullptr;
      else return fTreeByName[name].front()->GetModel();
    }

    /// Fill the RNTuple with name
    Int_t FillTree(const std::string& name) {
      if (!HasTreeByName(name)) return 0;
      else return fTreeByName[name].front()->Fill();
    }

    /// Fill all registered RNTuples
    Int_t FillTrees() {
      // Loop over all registered RNTuple names
      Int_t retval = 0;
      std::map<const std::string, std::vector<QwRootTree*>>::iterator iter;
      for (iter = fTreeByName.begin(); iter != fTreeByName.end(); iter++) {
        retval += iter->second.front()->Fill();
      }
      return retval;
    }

    /// Print registered RNTuples
    void PrintTrees() const {
      QwMessage << "RNTuples: " << QwLog::endl;
      // Loop over all registered RNTuple names
      std::map<const std::string, std::vector<QwRootTree*>>::const_iterator iter;
      for (iter = fTreeByName.begin(); iter != fTreeByName.end(); iter++) {
        QwMessage << iter->first << ": " << iter->second.size()
                  << " objects registered" << QwLog::endl;
        // Loop over all registered objects for this RNTuple
        std::vector<QwRootTree*>::const_iterator tree;
        for (tree = iter->second.begin(); tree != iter->second.end(); tree++) {
          (*tree)->Print();
        }
      }
    }
    /// Print registered histogram directories
    void PrintDirs() const {
      QwMessage << "Dirs: " << QwLog::endl;
      // Loop over all registered directories
      std::map<const std::string, TDirectory*>::const_iterator iter;
      for (iter = fDirsByName.begin(); iter != fDirsByName.end(); iter++) {
        QwMessage << iter->first << QwLog::endl;
      }
    }


    /// Write any object to the ROOT file (only valid for TFile)
    template < class T >
    Int_t WriteObject(const T* obj, const char* name, Option_t* option = "", Int_t bufsize = 0) {
      Int_t retval = 0;
      // TMapFile has no support for WriteObject
      if (fRootFile) retval = fRootFile->WriteObject(obj, name, option, bufsize);
      return retval;
    }


    // Wrapped functionality
    void Update() {
      if (fMapFile) {
        QwMessage << "TMapFile memory resident size: "
                  << ((int*)fMapFile->GetBreakval() - (int*)fMapFile->GetBaseAddr()) *
                     4 / sizeof(int32_t) / 1024 / 1024 << " MiB"
                  << QwLog::endl;
        fMapFile->Update();
      } else {
        // Flush all RNTuple writers
        Long64_t nBytes = 0;
        for (auto& pair : fRNTupleWriters) {
          pair.second->Flush();
          nBytes += pair.second->GetNEntries() * 100; // Rough estimate
        }
        
        QwMessage << "TFile saved: "
                  << nBytes/1000000 << "MB (estimate)" 
                  << QwLog::endl;
      }
    }
    void Print()  { if (fMapFile) fMapFile->Print();  if (fRootFile) fRootFile->Print(); }
    void ls()     { if (fMapFile) fMapFile->ls();     if (fRootFile) fRootFile->ls(); }
    void Map()    { if (fRootFile) fRootFile->Map(); }
    void Close()  {
      if (!fMakePermanent) fMakePermanent = HasAnyFilled();
      
      // Close all RNTuple writers
      fRNTupleWriters.clear();
      
      if (fMapFile) fMapFile->Close();
      if (fRootFile) fRootFile->Close();
    }

    // Wrapped functionality
    Bool_t cd(const char* path = 0) {
      Bool_t status = kTRUE;
      if (fMapFile)  status &= fMapFile->cd(path);
      if (fRootFile) status &= fRootFile->cd(path);
      return status;
    }

    // Wrapped functionality
    TDirectory* mkdir(const char* name, const char* title = "") {
      // TMapFile has no support for mkdir
      if (fRootFile) return fRootFile->mkdir(name, title);
      else return 0;
    }

    // Wrapped functionality
    Int_t Write(const char* name = 0, Int_t option = 0, Int_t bufsize = 0) {
      Int_t retval = 0;
      // TMapFile has no support for Write
      if (fRootFile) retval = fRootFile->Write(name, option, bufsize);
      return retval;
    }


  private:

    /// Private default constructor
    QwRootFile();


    /// ROOT file
    TFile* fRootFile;

    /// ROOT files dir
    TString fRootFileDir;
    /// Default ROOT files dir
    static std::string fDefaultRootFileDir;

    /// ROOT file stem
    TString fRootFileStem;
    /// Default ROOT file stem
    static std::string fDefaultRootFileStem;

    /// While the file is open, give it a temporary filename.  Perhaps
    /// change to a permanent name when closing the file.
    TString fPermanentName;
    Bool_t fMakePermanent;
    Bool_t fUseTemporaryFile;

    /// Search for non-empty RNTuples or histograms in the file
    Bool_t HasAnyFilled(void);
    Bool_t HasAnyFilled(TDirectory* d);

    /// Map file
    TMapFile* fMapFile;
    Bool_t fEnableMapFile;
    Int_t fUpdateInterval;
    Int_t fCompressionLevel;
    Int_t fBasketSize;
    Int_t fAutoFlush;
    Int_t fAutoSave;

    /// Map of RNTuple writers
    std::map<std::string, std::unique_ptr<ROOT::Experimental::RNTupleWriter>> fRNTupleWriters;

  private:

    /// List of excluded RNTuples
    std::vector<TPRegexp> fDisabledTrees;
    std::vector<TPRegexp> fDisabledHistos;

    /// Add regexp to list of disabled RNTuple names
    void DisableTree(const TString& regexp) {
      fDisabledTrees.push_back(regexp);
    }
    /// Does this RNTuple name match a disabled RNTuple name?
    bool IsTreeDisabled(const std::string& name) {
      for (size_t i = 0; i < fDisabledTrees.size(); i++)
        if (fDisabledTrees.at(i).Match(name)) return true;
      return false;
    }
    /// Add regexp to list of disabled histogram directories
    void DisableHisto(const TString& regexp) {
      fDisabledHistos.push_back(regexp);
    }
    /// Does this histogram directory match a disabled histogram directory?
    bool IsHistoDisabled(const std::string& name) {
      for (size_t i = 0; i < fDisabledHistos.size(); i++)
        if (fDisabledHistos.at(i).Match(name)) return true;
      return false;
    }


  private:

    /// RNTuple names, addresses, and types
    std::map<const std::string, std::vector<QwRootTree*>> fTreeByName;
    std::map<const void*, std::vector<QwRootTree*>> fTreeByAddr;
    std::map<const std::type_index, std::vector<QwRootTree*>> fTreeByType;
    // ... Are type_index objects really unique? Let's hope so.

    /// Is a RNTuple registered for this name
    bool HasTreeByName(const std::string& name) {
      if (fTreeByName.count(name) == 0) return false;
      else return true;
    }
    /// Is a RNTuple registered for this type
    template < class T >
    bool HasTreeByType(const T& object) {
      const std::type_index type = typeid(object);
      if (fTreeByType.count(type) == 0) return false;
      else return true;
    }
    /// Is a RNTuple registered for this object
    template < class T >
    bool HasTreeByAddr(const T& object) {
      const void* addr = static_cast<const void*>(&object);
      if (fTreeByAddr.count(addr) == 0) return false;
      else return true;
    }

    /// Directories
    std::map<const std::string, TDirectory*> fDirsByName;
    std::map<const std::string, std::vector<std::string>> fDirsByType;

    /// Is a RNTuple registered for this name
    bool HasDirByName(const std::string& name) {
      if (fDirsByName.count(name) == 0) return false;
      else return true;
    }
    /// Is a directory registered for this type
    template < class T >
    bool HasDirByType(const T& object) {
      std::string type = typeid(object).name();
      if (fDirsByType.count(type) == 0) return false;
      else return true;
    }


  private:

    /// Prescaling of events written to RNTuple
    UInt_t fNumMpsEventsToSkip;
    UInt_t fNumMpsEventsToSave;
    UInt_t fNumHelEventsToSkip;
    UInt_t fNumHelEventsToSave;
    UInt_t fCircularBufferSize;
    UInt_t fCurrentEvent;

    /// Maximum RNTuple size
    static const Long64_t kMaxTreeSize;
    static const Int_t    kMaxMapFileSize;
};

/**
 * Construct the indices from one RNTuple to another RNTuple, and optionally in reverse as well.
 * @param from Name of RNTuple where index will be created
 * @param to Name of RNTuple to which index will point
 * @param reverse Flag to create indices in both direction
 */
inline void QwRootFile::ConstructIndices(const std::string& from, const std::string& to, bool reverse)
{
  // Return if we do not want this RNTuple information
  if (IsTreeDisabled(from)) return;
  if (IsTreeDisabled(to)) return;

  // If the RNTuples are defined
  if (fTreeByName.count(from) > 0 && fTreeByName.count(to) > 0) {

    // Construct index from the first RNTuple to the second RNTuple
    fTreeByName[from].front()->ConstructIndexTo(fTreeByName[to].front());

    // Construct index from the second RNTuple back to the first RNTuple
    if (reverse)
      fTreeByName[to].front()->ConstructIndexTo(fTreeByName[from].front());
  }
}

/**
 * Construct the RNTuple fields of a generic object
 * @param name Name for RNTuple
 * @param desc Description for RNTuple
 * @param object Subsystem array
 * @param prefix Prefix for the RNTuple
 */
template < class T >
void QwRootFile::ConstructTreeBranches(
        const std::string& name,
        const std::string& desc,
        T& object,
        const std::string& prefix)
{
  // Return if we do not want this RNTuple information
  if (IsTreeDisabled(name)) return;

  // Pointer to new RNTuple
  QwRootTree* tree = nullptr;

  // If the RNTuple does not exist yet, create it
  if (fTreeByName.count(name) == 0) {

    // Go to top level directory
    this->cd();

    // New RNTuple with name, description, object, prefix
    tree = new QwRootTree(name, desc, object, prefix);

    // Create and set the RNTuple writer
    if (fRootFile) {
      auto writer = ROOT::Experimental::RNTupleWriter::Recreate(tree->GetModel(), name, *fRootFile);
      tree->SetWriter(writer.get());
      fRNTupleWriters[name] = std::move(writer);
    }

    // Settings only relevant for new RNTuples
    if (name == "evt")
      tree->SetPrescaling(fNumMpsEventsToSave, fNumMpsEventsToSkip);
    else if (name == "mul")
      tree->SetPrescaling(fNumHelEventsToSave, fNumHelEventsToSkip);

    tree->SetAutoFlush(fAutoFlush);
    tree->SetAutoSave(fAutoSave);
    tree->SetCompressionSettings(fCompressionLevel);
    tree->SetMaxNTupleSize(kMaxTreeSize);

    if (fCircularBufferSize > 0)
      tree->SetCircular(fCircularBufferSize);

  } else {

    // New RNTuple based on existing RNTuple
    tree = new QwRootTree(fTreeByName[name].front(), object, prefix);
  }

   // Add the fields to the list of RNTuples by name, object, type
  const void* addr = static_cast<const void*>(&object);
  const std::type_index type = typeid(object);
  fTreeByName[name].push_back(tree);
  fTreeByAddr[addr].push_back(tree);
  fTreeByType[type].push_back(tree);
}


/**
 * Fill the RNTuple fields of a generic object by name
 * @param name Name for RNTuple
 * @param object Subsystem array
 */
template < class T >
void QwRootFile::FillTreeBranches(
        const std::string& name,
        const T& object)
{
  // If this name has no registered RNTuples
  if (!HasTreeByName(name)) return;
  // If this type has no registered RNTuples
  if (!HasTreeByType(object)) return;

  // Get the address of the object
  const void* addr = static_cast<const void*>(&object);

  // Fill the RNTuples with the correct address
  for (size_t tree = 0; tree < fTreeByAddr[addr].size(); tree++) {
    if (fTreeByAddr[addr].at(tree)->GetName() == name) {
      fTreeByAddr[addr].at(tree)->FillTreeBranches(object);
    }
  }
}


/**
 * Fill the RNTuple fields of a generic object by type only
 * @param object Subsystem array
 */
template < class T >
void QwRootFile::FillTreeBranches(
        const T& object)
{
  // If this address has no registered RNTuples
  if (!HasTreeByAddr(object)) return;

  // Get the address of the object
  const void* addr = static_cast<const void*>(&object);

  // Fill the RNTuples with the correct address
  for (size_t tree = 0; tree < fTreeByAddr[addr].size(); tree++) {
    fTreeByAddr[addr].at(tree)->FillTreeBranches(object);
  }
}


/**
 * Construct the objects directory of a generic object
 * @param name Name for objects directory
 * @param object Subsystem array
 */
template < class T >
void QwRootFile::ConstructObjects(const std::string& name, T& object)
{
  // Create the objects in a directory
  if (fRootFile) {
    std::string type = typeid(object).name();
    fDirsByName[name] =
        fRootFile->GetDirectory(("/" + name).c_str()) ?
            fRootFile->GetDirectory(("/" + name).c_str()) :
            fRootFile->GetDirectory("/")->mkdir(name.c_str());
    fDirsByType[type].push_back(name);
    object.ConstructObjects(fDirsByName[name]);
  }

  // No support for directories in a map file
  if (fMapFile) {
    QwMessage << "QwRootFile::ConstructObjects::detectors address "
	      << &object
	      << " and its name " << name
	      << QwLog::endl;

    std::string type = typeid(object).name();
    fDirsByName[name] = fMapFile->GetDirectory()->mkdir(name.c_str());
    fDirsByType[type].push_back(name);
    object.ConstructObjects();
  }
}


/**
 * Construct the histogram of a generic object
 * @param name Name for histogram directory
 * @param object Subsystem array
 */
template < class T >
void QwRootFile::ConstructHistograms(const std::string& name, T& object)
{
  // Return if we do not want this histogram information
  if (IsHistoDisabled(name)) return;

  // Create the histograms in a directory
  if (fRootFile) {
    std::string type = typeid(object).name();
    fDirsByName[name] =
        fRootFile->GetDirectory(("/" + name).c_str()) ?
            fRootFile->GetDirectory(("/" + name).c_str()) :
            fRootFile->GetDirectory("/")->mkdir(name.c_str());
    fDirsByType[type].push_back(name);
    object.ConstructHistograms(fDirsByName[name]);
  }

  // No support for directories in a map file
  if (fMapFile) {
    QwMessage << "QwRootFile::ConstructHistograms::detectors address "
	      << &object  
	      << " and its name " << name 
	      << QwLog::endl;

    std::string type = typeid(object).name();
    fDirsByName[name] = fMapFile->GetDirectory()->mkdir(name.c_str());
    fDirsByType[type].push_back(name);
    //object.ConstructHistograms(fDirsByName[name]);
    object.ConstructHistograms();
  }
}


template < class T >
Int_t QwRootFile::WriteParamFileList(const TString &name, T& object)
{
  Int_t retval = 0;
  if (fRootFile) {
    TList *param_list = (TList*) fRootFile->FindObjectAny(name);
    if (not param_list) {
      retval = fRootFile->WriteObject(object.GetParamFileNameList(name), name);
    }
  }
  return retval;
}


#endif // __QWROOTFILE__
