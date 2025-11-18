#include "QwRootTree.h"

const TString QwRootTree::kUnitsName = "ppm/D:ppb/D:um/D:mm/D:mV_uA/D:V_uA/D";
Double_t QwRootTree::kUnitsValue[] = { 1e-6, 1e-9, 1e-3, 1 , 1e-3, 1};

QwRootTree::QwRootTree(const std::string& name, const std::string& desc, const std::string& prefix)
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
}

QwRootTree::QwRootTree(const QwRootTree* tree, const std::string& prefix)
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
}

void QwRootTree::ConstructNewTree()
{
  QwMessage << "New tree: " << fName << ", " << fDesc << QwLog::endl;

  fTree = new TTree(fName.c_str(), fDesc.c_str());
  // Ensure tree is in the current directory
  if (gDirectory) { // Is gDirectory ever NULL?
  	QwMessage << "gDirectory is not NULL!\n";
    fTree->SetDirectory(gDirectory);
  } else {
  	QwMessage << "gDirectory is NULL!\n";
  }
  getchar();
}

void QwRootTree::ConstructUnitsBranch()
{
  std::string name = "units";
  fTree->Branch(name.c_str(), &kUnitsValue, kUnitsName);
}

void QwRootTree::ConstructIndexTo(QwRootTree* to)
{
  std::string name = "previous_entry_in_" + to->fName;
  fTree->Branch(name.c_str(), &(to->fCurrentEvent));
}

Long64_t QwRootTree::AutoSave(Option_t *option)
{
  return fTree->AutoSave(option);
}


Int_t QwRootTree::Fill()
{
  fCurrentEvent++;

  // Tree prescaling
  if (fNumEventsCycle > 0) {
    fCurrentEvent %= fNumEventsCycle;
    if (fCurrentEvent > fNumEventsToSave)
      return 0;
  }

  // Fill the tree
  Int_t retval = fTree->Fill();
  // Check for errors
  if (retval < 0) {
    QwError << "Writing tree failed!  Check disk space or quota." << QwLog::endl;
    exit(retval);
  }
  return retval;
}

void QwRootTree::Print() const
{
  QwMessage << GetName() << ", " << GetType();
  if (fPrefix != "")
    QwMessage << " (prefix " << GetPrefix() << ")";
  QwMessage << QwLog::endl;
}

const std::string& QwRootTree::GetName()   const { return fName  ; };
const std::string& QwRootTree::GetDesc()   const { return fDesc  ; };
const std::string& QwRootTree::GetPrefix() const { return fPrefix; };
const std::string& QwRootTree::GetType()   const { return fType  ; };

void QwRootTree::SetPrescaling(UInt_t num_to_save, UInt_t num_to_skip)
{
  fNumEventsToSave = num_to_save;
  fNumEventsToSkip = num_to_skip;
  fNumEventsCycle = fNumEventsToSave + fNumEventsToSkip;
}

void QwRootTree::SetMaxTreeSize(Long64_t maxsize)
{
  fMaxTreeSize = maxsize;
  if (fTree) fTree->SetMaxTreeSize(maxsize);
}

void QwRootTree::SetAutoFlush(Long64_t autoflush)
{
  fAutoFlush = autoflush;
#if ROOT_VERSION_CODE >= ROOT_VERSION(5,26,00)
    if (fTree) fTree->SetAutoFlush(autoflush);
 #endif
}

void QwRootTree::SetAutoSave(Long64_t autosave)
{
  fAutoSave = autosave;
  if (fTree) fTree->SetAutoSave(autosave);
}

void QwRootTree::SetBasketSize(Int_t basketsize)
{
  fBasketSize = basketsize;
  if (fTree) fTree->SetBasketSize("*",basketsize);
}

void QwRootTree::SetCircular(Long64_t buff)
{
  if (fTree) fTree->SetCircular(buff);
}
