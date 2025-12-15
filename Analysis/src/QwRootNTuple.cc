#ifdef HAS_RNTUPLE_SUPPORT
#include "QwRootNTuple.h"

// System Headers
#include <string>

// ROOT Headers
#include "Rtypes.h"
#include "TFile.h"

// QWeak Headers
#include "QwLog.h"


QwRootNTuple::QwRootNTuple(const std::string& name, const std::string& desc, const std::string& prefix)
: fName(name)
, fDesc(desc)
, fPrefix(prefix)
, fType("type undefined")
, fCurrentEvent(0)
, fNumEventsCycle(0)
, fNumEventsToSave(0)
, fNumEventsToSkip(0)
{
  fModel = ROOT::RNTupleModel::Create();
}

QwRootNTuple::~QwRootNTuple()
{
    Close();
}

void QwRootNTuple::Close()
{
  if (fWriter) {
    // ROOT::~RNTupleWriter writes to disk
    fWriter.reset();
  }
}

void QwRootNTuple::InitializeWriter(TFile* file)
{
  if (!fModel) {
    QwError << "RNTuple model not created for " << fName << QwLog::endl;
    return;
  }

  // Before creating the writer, ensure all fields are added to the model
  if (fVector.empty()) {
    QwError << "No fields defined in RNTuple model for " << fName << QwLog::endl;
    return;
  }

  try {
    // Create the writer with the model (transfers ownership)
    // Use Append to add RNTuple to existing TFile
    fWriter = ROOT::RNTupleWriter::Append(std::move(fModel), fName, *file);
    QwMessage << "Created RNTuple '" << fName << "' in file " << file->GetName() << QwLog::endl;

  } catch (const std::exception& e) {
    QwError << "Failed to create RNTuple writer for '" << fName << "': " << e.what() << QwLog::endl;
  }
}

const std::string& QwRootNTuple::GetName()  const { return fName;  }
/// Get the description of the RNTuple
const std::string& QwRootNTuple::GetDesc()  const { return fDesc;  }
/// Get the prefix of the RNTuple
const std::string& QwRootNTuple::GetPrefix()const { return fPrefix;}
/// Get the object type
const std::string& QwRootNTuple::GetType()  const { return fType;  }

void QwRootNTuple::SetPrescaling(UInt_t num_to_save, UInt_t num_to_skip)
{
  fNumEventsToSave = num_to_save;
  fNumEventsToSkip = num_to_skip;
  fNumEventsCycle = fNumEventsToSave + fNumEventsToSkip;
}

void QwRootNTuple::Print() const
{
  QwMessage << GetName() << ", " << GetType();
  if (fPrefix != "")
    QwMessage << " (prefix " << GetPrefix() << ")";
  QwMessage << QwLog::endl;
}
#endif // HAS_RNTUPLE_SUPPORT
