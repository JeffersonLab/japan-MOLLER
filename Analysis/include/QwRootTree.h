#pragma once

// System Headers
#include <cstddef>
#include <string>
#include <sstream>
#include <iomanip>

// ROOT Headers
#include "TSystem.h"
#include "TTree.h"
#include "TString.h"

// QWEAK Headers
#include "QwOptions.h"
#include "QwRootTreeBranchVector.h"


/**
 *  \class QwRootTree
 *  \ingroup QwAnalysis
 *  \brief Wrapper class for ROOT tree management with vector-based data storage
 *
 * Provides functionality to write to ROOT trees using vectors of doubles,
 * with support for branch construction, event filtering, and tree sharing.
 * Handles both new tree creation and attachment to existing trees, enabling
 * multiple subsystems to contribute data to a single ROOT tree.
 */

// If one defines more than this number of words in the full ttree,
// the results are going to get very very crazy.
constexpr std::size_t BRANCH_VECTOR_MAX_SIZE = 25000;
class QwRootTree {

public:
  /// Constructor with name, and description
  QwRootTree(const std::string& name, const std::string& desc, const std::string& prefix = "");
  /// Constructor with existing tree
  QwRootTree(const QwRootTree* tree, const std::string& prefix = "");

  /// Constructor with name, description, and object
  template < class T >
  QwRootTree(const std::string& name, const std::string& desc, T& object, const std::string& prefix = "");
  /// Constructor with existing tree, and object
  template < class T >
  QwRootTree(const QwRootTree* tree, T& object, const std::string& prefix = "");

  /// Destructor
  virtual ~QwRootTree() { }


private:
  static const TString kUnitsName;
  static Double_t kUnitsValue[];

  /// Construct the tree
  void ConstructNewTree();
  void ConstructUnitsBranch();
  /// Construct the branches and vector for generic objects
  template < class T >
  void ConstructBranchAndVector(T& object);

public:
  /// Construct index from this tree to another tree
  void ConstructIndexTo(QwRootTree* to);

  /// Fill the branches for generic objects
  template < class T >
  void FillTreeBranches(const T& object);
  Long64_t AutoSave(Option_t *option);

  /// Fill the tree
  [[nodiscard]]
  Int_t Fill();

  /// Print the tree name and description
  void Print() const;
  /// Get the tree pointer for low level operations
  TTree* GetTree() const { return fTree; };

  uint64_t GetNEntriesFilled() const { return fTree->GetEntries(); }

  /// Set tree prescaling parameters
  void SetPrescaling(UInt_t num_to_save, UInt_t num_to_skip);
  /// Set maximum tree size
  void SetMaxTreeSize(Long64_t maxsize = 1900000000);
  /// Set autoflush size
  void SetAutoFlush(Long64_t autoflush = 30000000);
  /// Set autosave size
  void SetAutoSave(Long64_t autosave = 300000000);
  /// Set basket size
  void SetBasketSize(Int_t basketsize = 16000);
  //Set circular buffer size for the memory resident tree
  void SetCircular(Long64_t buff = 100000);

private:
  /// Tree pointer
  TTree* fTree;
  /// Vector of leaves
  QwRootTreeBranchVector fVector;


  /// Name, description, Obj. Type
  const std::string fName;
  const std::string fDesc;
  const std::string fPrefix;
  std::string fType;

  /// Get the name of the tree
  const std::string& GetName()  const;
  /// Get the description of the tree
  const std::string& GetDesc()  const;
  /// Get the description of the tree
  const std::string& GetPrefix()const;
  /// Get the object type
  const std::string& GetType()  const;


  /// Tree prescaling parameters
  UInt_t fCurrentEvent;
  UInt_t fNumEventsCycle;
  UInt_t fNumEventsToSave;
  UInt_t fNumEventsToSkip;



  /// Maximum tree size, autoflush and autosave
  Long64_t fMaxTreeSize;
  Long64_t fAutoFlush;
  Long64_t fAutoSave;
  Int_t fBasketSize;

};

/// Template Implementation ///
template < class T >
QwRootTree::QwRootTree(const std::string& name, const std::string& desc, T& object, const std::string& prefix)
: fName(name)
, fDesc(desc)
, fPrefix(prefix)
, fType("type undefined")
, fCurrentEvent(0)
, fNumEventsCycle(0)
, fNumEventsToSave(0)
, fNumEventsToSkip(0)
{
  ConstructNewTree();
  ConstructUnitsBranch();
  ConstructBranchAndVector(object);
}

template < class T >
QwRootTree::QwRootTree(const QwRootTree* tree, T& object, const std::string& prefix)
: fName(tree->GetName())
, fDesc(tree->GetDesc())
, fPrefix(prefix)
, fType("type undefined")
, fCurrentEvent(0)
, fNumEventsCycle(0)
, fNumEventsToSave(0)
, fNumEventsToSkip(0)
{
  QwMessage << "Existing tree: " << tree->GetName() << ", " << tree->GetDesc() << QwLog::endl;
  fTree = tree->fTree;

  ConstructBranchAndVector(object);
}

template < class T >
void QwRootTree::ConstructBranchAndVector(T& object)
{
  // TODO:
  // Reserve space for the branch vector
  fVector.reserve(BRANCH_VECTOR_MAX_SIZE);
  // Associate branches with vector

  TString prefix = Form("%s",fPrefix.c_str());
  object.ConstructBranchAndVector(fTree, prefix, fVector);

  // Store the type of object
  fType = typeid(object).name();

  // Check memory reservation
  /*
  if (fVector.size() > BRANCH_VECTOR_MAX_SIZE) {
    QwError << "The branch vector is too large: " << fVector.size() << " leaves!  "
            << "The maximum size is " << BRANCH_VECTOR_MAX_SIZE << "."
            << QwLog::endl;
    exit(-1);
  }
  */
}

template < class T >
void QwRootTree::FillTreeBranches(const T& object)
{
  if (typeid(object).name() == fType) {
    // Fill the branch vector
    object.FillTreeVector(fVector);
  } else {
    QwError << "Attempting to fill tree vector for type " << fType << " with "
            << "object of type " << typeid(object).name() << QwLog::endl;
    exit(-1);
  }
}
