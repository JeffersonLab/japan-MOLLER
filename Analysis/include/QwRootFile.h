/*!
 * \file   QwRootFile.h
 * \brief  ROOT file and tree management wrapper classes
 */

#pragma once

// System headers
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <unistd.h>
using std::type_info;

// ROOT headers
#include "TFile.h"
#include "TTree.h"
#include "TPRegexp.h"
#include "TSystem.h"
#include "TString.h"

// Qweak headers
#include "QwOptions.h"
#include "TMapFile.h"
#include "QwRootTreeBranchVector.h"
#include "QwRootTree.h"
#include "QwRootNTuple.h"


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
 * The class keeps track of the registered tree names, and the types of objects
 * that have branches constructed in those trees (via QwRootTree).  In most
 * cases it should be possible to just call FillTreeBranches with only the object,
 * although in rare cases this could be ambiguous.
 *
 * The proper way to register a tree is by either calling ConstructTreeBranches
 * of NewTree first.  Then FillTreeBranches will fill the vector, and FillTree
 * will actually fill the tree.  FillTree should be called only once.
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
    static void SetDefaultRootFileDir(const std::string& dir);
    /// \brief Set default ROOT file stem
    static void SetDefaultRootFileStem(const std::string& stem);

    /// Is the ROOT file active?
    Bool_t IsRootFile() const;
    /// Is the map file active?
    Bool_t IsMapFile()  const;

    /// \brief Construct indices from one tree to another tree
    void ConstructIndices(const std::string& from, const std::string& to, bool reverse = true);

    /// \brief Construct the tree branches of a generic object
    template < class T >
    void ConstructTreeBranches(const std::string& name, const std::string& desc, T& object, const std::string& prefix = "");
    /// \brief Fill the tree branches of a generic object by tree name
    template < class T >
    void FillTreeBranches(const std::string& name, const T& object);
    /// \brief Fill the tree branches of a generic object by type only
    template < class T >
    void FillTreeBranches(const T& object);

#ifdef HAS_RNTUPLE_SUPPORT
    /// \brief Construct the RNTuple fields of a generic object
    template < class T >
    void ConstructNTupleFields(const std::string& name, const std::string& desc, T& object, const std::string& prefix = "");
    /// \brief Fill the RNTuple fields of a generic object by name
    template < class T >
    void FillNTupleFields(const std::string& name, const T& object);
    /// \brief Fill the RNTuple fields of a generic object by type only
    template < class T >
    void FillNTupleFields(const T& object);
#endif // HAS_RNTUPLE_SUPPORT


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
    void FillHistograms(T& object);

    /// Create a new tree with name and description
    void NewTree(const std::string& name, const std::string& desc);

#ifdef HAS_RNTUPLE_SUPPORT
    /// Create a new RNTuple with name and description
    void NewNTuple(const std::string& name, const std::string& desc);
    /// Fill the RNTuple with name
    void FillNTuple(const std::string& name);
    /// Fill all registered RNTuples
    void FillNTuples();
#endif // HAS_RNTUPLE_SUPPORT

    /// Get the tree with name
    TTree* GetTree(const std::string& name);
    /// Fill the tree with name
    Int_t FillTree(const std::string& name);
    /// Fill all registered trees
    Int_t FillTrees();

    /// Print registered trees
    void PrintTrees() const;
    /// Print registered histogram directories
    void PrintDirs() const;

    /// Write any object to the ROOT file (only valid for TFile)
    template < class T >
    Int_t WriteObject(const T* obj, const char* name, Option_t* option = "", Int_t bufsize = 0);


    // Wrapped functionality
    void Update();
    void Print() const;
    void ls()    const;
    void Map();
    void Close();
    Bool_t cd(const char* path = 0);
    TDirectory* mkdir(const char* name, const char* title = "");
    Int_t Write(const char* name = 0, Int_t option = 0, Int_t bufsize = 0);

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

    /// Search for non-empty trees or histograms in the file
    Bool_t HasAnyFilled(void);
    Bool_t HasAnyFilled(TDirectory* d);

    /// Map file
    TMapFile* fMapFile;
    Bool_t fEnableMapFile;
    Int_t fUpdateInterval;
    Int_t fCompressionAlgorithm;
    Int_t fCompressionLevel;
    Int_t fBasketSize;
    Int_t fAutoFlush;
    Int_t fAutoSave;



  private:
    /// List of excluded trees
    std::vector< TPRegexp > fDisabledTrees;
    std::vector< TPRegexp > fDisabledHistos;

    /// Add regexp to list of disabled trees names
    void DisableTree(const TString& regexp);
    /// Does this tree name match a disabled tree name?
    bool IsTreeDisabled(const std::string& name);
    /// Add regexp to list of disabled histogram directories
    void DisableHisto(const TString& regexp);
    /// Does this histogram directory match a disabled histogram directory?
    bool IsHistoDisabled(const std::string& name);


  private:
    /// Tree names, addresses, and types
    std::map< const std::string, std::vector<QwRootTree*> > fTreeByName;
    std::map< const void*      , std::vector<QwRootTree*> > fTreeByAddr;
    std::map< const std::type_index , std::vector<QwRootTree*> > fTreeByType;
    // ... Are type_index objects really unique? Let's hope so.

#ifdef HAS_RNTUPLE_SUPPORT
    /// RNTuple names, addresses, and types
    std::map< const std::string, std::vector<QwRootNTuple*> > fNTupleByName;
    std::map< const void*      , std::vector<QwRootNTuple*> > fNTupleByAddr;
    std::map< const std::type_index , std::vector<QwRootNTuple*> > fNTupleByType;

    /// RNTuple support flag
    Bool_t fEnableRNTuples;
#endif // HAS_RNTUPLE_SUPPORT

    /// Is a tree registered for this name
    bool HasTreeByName(const std::string& name);
    /// Is a tree registered for this type
    template < class T >
    bool HasTreeByType(const T& object);
    /// Is a tree registered for this object
    template < class T >
    bool HasTreeByAddr(const T& object);

#ifdef HAS_RNTUPLE_SUPPORT
    /// Is an RNTuple registered for this name
    bool HasNTupleByName(const std::string& name);
    /// Is an RNTuple registered for this type
    template < class T >
    bool HasNTupleByType(const T& object);
    /// Is an RNTuple registered for this object
    template < class T >
    bool HasNTupleByAddr(const T& object);
#endif // HAS_RNTUPLE_SUPPORT

    /// Directories
    std::map< const std::string, TDirectory* > fDirsByName;
    std::map< const std::string, std::vector<std::string> > fDirsByType;

    /// Is a tree registered for this name
    bool HasDirByName(const std::string& name);
    /// Is a directory registered for this type
    template < class T >
    bool HasDirByType(const T& object);

  private:
    /// Prescaling of events written to tree
    UInt_t fNumMpsEventsToSkip;
    UInt_t fNumMpsEventsToSave;
    UInt_t fNumHelEventsToSkip;
    UInt_t fNumHelEventsToSave;
    UInt_t fCircularBufferSize;
    UInt_t fCurrentEvent;

    /// Maximum tree size
    static const Long64_t kMaxTreeSize;
    static const Int_t    kMaxMapFileSize;
};

/// Template Implementation ///

/**
 * Construct the indices from one tree to another tree, and optionally in reverse as well.
 * @param from Name of tree where index will be created
 * @param to Name of tree to which index will point
 * @param reverse Flag to create indices in both direction
 */
inline void QwRootFile::ConstructIndices(const std::string& from, const std::string& to, bool reverse)
{
  // Return if we do not want this tree information
  if (IsTreeDisabled(from)) return;
  if (IsTreeDisabled(to)) return;

  // If the trees are defined
  if (fTreeByName.count(from) > 0 && fTreeByName.count(to) > 0) {

    // Construct index from the first tree to the second tree
    fTreeByName[from].front()->ConstructIndexTo(fTreeByName[to].front());

    // Construct index from the second tree back to the first tree
    if (reverse)
      fTreeByName[to].front()->ConstructIndexTo(fTreeByName[from].front());
  }
}

/**
 * Construct the tree branches of a generic object
 * @param name Name for tree
 * @param desc Description for tree
 * @param object Subsystem array
 * @param prefix Prefix for the tree
 */
template < class T >
void QwRootFile::ConstructTreeBranches(
        const std::string& name,
        const std::string& desc,
        T& object,
        const std::string& prefix)
{
  // Return if we do not want this tree information
  if (IsTreeDisabled(name)) return;

  // Pointer to new tree
  QwRootTree* tree = 0;

  // If the tree does not exist yet, create it
  if (fTreeByName.count(name) == 0) {

    // Go to top level directory

    this->cd();

    // New tree with name, description, object, prefix
    tree = new QwRootTree(name, desc, object, prefix);

    // Settings only relevant for new trees
    if (name == "evt")
      tree->SetPrescaling(fNumMpsEventsToSave, fNumMpsEventsToSkip);
    else if (name == "mul")
      tree->SetPrescaling(fNumHelEventsToSave, fNumHelEventsToSkip);

    #if ROOT_VERSION_CODE >= ROOT_VERSION(5,26,00)
    tree->SetAutoFlush(fAutoFlush);
    #endif
    tree->SetAutoSave(fAutoSave);
    tree->SetBasketSize(fBasketSize);
    tree->SetMaxTreeSize(kMaxTreeSize);

    if (fCircularBufferSize > 0)
      tree->SetCircular(fCircularBufferSize);

  } else {

    // New tree based on existing tree
    tree = new QwRootTree(fTreeByName[name].front(), object, prefix);
  }

   // Add the branches to the list of trees by name, object, type
  const void* addr = static_cast<const void*>(&object);
  const std::type_index type = typeid(object);
  fTreeByName[name].push_back(tree);
  fTreeByAddr[addr].push_back(tree);
  fTreeByType[type].push_back(tree);
}


/**
 * Fill the tree branches of a generic object by name
 * @param name Name for tree
 * @param object Subsystem array
 */
template < class T >
void QwRootFile::FillTreeBranches(
        const std::string& name,
        const T& object)
{
  // If this name has no registered trees
  if (! HasTreeByName(name)) return;
  // If this type has no registered trees
  if (! HasTreeByType(object)) return;

  // Get the address of the object
  const void* addr = static_cast<const void*>(&object);

  // Fill the trees with the correct address
  for (size_t tree = 0; tree < fTreeByAddr[addr].size(); tree++) {
    if (fTreeByAddr[addr].at(tree)->GetName() == name) {
      fTreeByAddr[addr].at(tree)->FillTreeBranches(object);
    }
  }
}


/**
 * Fill the tree branches of a generic object by type only
 * @param object Subsystem array
 */
template < class T >
void QwRootFile::FillTreeBranches(
        const T& object)
{
  // If this address has no registered trees
  if (! HasTreeByAddr(object)) return;

  // Get the address of the object
  const void* addr = static_cast<const void*>(&object);

  // Fill the trees with the correct address
  for (size_t tree = 0; tree < fTreeByAddr[addr].size(); tree++) {
    fTreeByAddr[addr].at(tree)->FillTreeBranches(object);
  }
}


#ifdef HAS_RNTUPLE_SUPPORT
/**
 * Construct the RNTuple fields of a generic object
 * @param name Name for RNTuple
 * @param desc Description for RNTuple
 * @param object Subsystem array
 * @param prefix Prefix for the RNTuple
 */
template < class T >
void QwRootFile::ConstructNTupleFields(
        const std::string& name,
        const std::string& desc,
        T& object,
        const std::string& prefix)
{
  // Return if RNTuples are disabled
  if (!fEnableRNTuples) return;

  // Pointer to new RNTuple
  QwRootNTuple* ntuple = 0;

  // If the RNTuple does not exist yet, create it
  if (fNTupleByName.count(name) == 0) {

    // New RNTuple with name, description, object, prefix
    ntuple = new QwRootNTuple(name, desc, object, prefix);

    // Initialize the writer with our file
    ntuple->InitializeWriter(fRootFile);

    // Settings only relevant for new RNTuples
    if (name == "evt")
      ntuple->SetPrescaling(fNumMpsEventsToSave, fNumMpsEventsToSkip);
    else if (name == "mul")
      ntuple->SetPrescaling(fNumHelEventsToSave, fNumHelEventsToSkip);

  } else {

    // For simplicity, don't support multiple RNTuples with same name yet
    QwError << "Cannot create duplicate RNTuple: " << name << QwLog::endl;
    return;
  }

   // Add the fields to the list of RNTuples by name, object, type
  const void* addr = static_cast<const void*>(&object);
  const std::type_index type = typeid(object);
  fNTupleByName[name].push_back(ntuple);
  fNTupleByAddr[addr].push_back(ntuple);
  fNTupleByType[type].push_back(ntuple);
}


/**
 * Fill the RNTuple fields of a generic object by name
 * @param name Name for RNTuple
 * @param object Subsystem array
 */
template < class T >
void QwRootFile::FillNTupleFields(
        const std::string& name,
        const T& object)
{
  // If this name has no registered RNTuples
  if (! HasNTupleByName(name)) return;
  // If this type has no registered RNTuples
  if (! HasNTupleByType(object)) return;

  // Get the address of the object
  const void* addr = static_cast<const void*>(&object);

  // Fill the RNTuples with the correct address
  for (size_t ntuple = 0; ntuple < fNTupleByAddr[addr].size(); ntuple++) {
    if (fNTupleByAddr[addr].at(ntuple)->GetName() == name) {
      fNTupleByAddr[addr].at(ntuple)->FillNTupleFields(object);
    }
  }
}


/**
 * Fill the RNTuple fields of a generic object by type only
 * @param object Subsystem array
 */
template < class T >
void QwRootFile::FillNTupleFields(
        const T& object)
{
  // If this address has no registered RNTuples
  if (! HasNTupleByAddr(object)) return;

  // Get the address of the object
  const void* addr = static_cast<const void*>(&object);

  // Fill the RNTuples with the correct address
  for (size_t ntuple = 0; ntuple < fNTupleByAddr[addr].size(); ntuple++) {
    fNTupleByAddr[addr].at(ntuple)->FillNTupleFields(object);
  }
}
#endif // HAS_RNTUPLE_SUPPORT


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
template < class T >
void QwRootFile::FillHistograms(T& object)
{
  // Update regularly
  static Int_t update_count = 0;
  update_count++;
  if ((fUpdateInterval > 0) && ( update_count % fUpdateInterval == 0)) Update();

  // Debug directory registration
  std::string type = typeid(object).name();
  bool hasDir = HasDirByType(object);

  if (! hasDir) return;
  // Fill histograms
  object.FillHistograms();
}

template < class T >
Int_t QwRootFile::WriteObject(const T* obj, const char* name, Option_t* option, Int_t bufsize)
{
  Int_t retval = 0;
  // TMapFile has no support for WriteObject
  if (fRootFile) retval = fRootFile->WriteObject(obj,name,option,bufsize);
  return retval;
}

template < class T >
bool QwRootFile::HasTreeByType(const T& object)
{
  const std::type_index type = typeid(object);
  if (fTreeByType.count(type) == 0) return false;
  else return true;
}

template < class T >
bool QwRootFile::HasTreeByAddr(const T& object)
{
  const void* addr = static_cast<const void*>(&object);
  if (fTreeByAddr.count(addr) == 0) return false;
  else return true;
}

template < class T >
bool QwRootFile::HasDirByType(const T& object)
{
  std::string type = typeid(object).name();
  if (fDirsByType.count(type) == 0) return false;
  else return true;
}

#ifdef HAS_RNTUPLE_SUPPORT
template < class T >
bool QwRootFile::HasNTupleByType(const T& object)
{
  const std::type_index type = typeid(object);
  if (fNTupleByType.count(type) == 0) return false;
  else return true;
}
/// Is an RNTuple registered for this object
template < class T >
bool QwRootFile::HasNTupleByAddr(const T& object)
{
  const void* addr = static_cast<const void*>(&object);
  if (fNTupleByAddr.count(addr) == 0) return false;
  else return true;
}
#endif // HAS_RNTUPLE_SUPPORT
