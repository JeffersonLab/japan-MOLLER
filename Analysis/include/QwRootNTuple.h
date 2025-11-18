#pragma once

#ifdef HAS_RNTUPLE_SUPPORT
// System Headers
#include <string>

// ROOT Headers
#include "TString.h"
#include "TFile.h"
#include "ROOT/RNTuple.hxx"
#include "ROOT/RNTupleModel.hxx"
#include "ROOT/RField.hxx"
#include "ROOT/RNTupleWriter.hxx"

// QWeak Headers
#include "QwLog.h"


// If one defines more than this number of words in the full ntuple,
// the results are going to get very very crazy.
constexpr std::size_t RNTUPLE_MAX_SIZE = 25000;

/**
 *  \class QwRootNTuple
 *  \ingroup QwAnalysis
 *  \brief A wrapper class for a ROOT RNTuple
 *
 * This class provides the functionality to write to ROOT RNTuples using a vector
 * of doubles, similar to QwRootTree but using the newer RNTuple format.
 */
class QwRootNTuple {

public:
  /// Constructor with name and description
  QwRootNTuple(const std::string& name, const std::string& desc, const std::string& prefix = "");
  virtual ~QwRootNTuple();

  /// Constructor with name, description, and object
  template < class T >
  QwRootNTuple(const std::string& name, const std::string& desc, T& object, const std::string& prefix = "");

  /// Close and finalize the RNTuple writer
  void Close();

private:
  /// Construct the fields and vector for generic objects
  template < class T >
  void ConstructFieldsAndVector(T& object);

public:

  /// Initialize the RNTuple writer with a file
  void InitializeWriter(TFile* file);

  /// Fill the fields for generic objects
  template < class T >
  void FillNTupleFields(const T& object);
  /// Fill the RNTuple (called by FillTree wrapper methods)
  void Fill() {
    // This method is now called indirectly - the actual filling happens in FillNTupleFields
    // Just here for compatibility with the tree interface
  }

  /// Get the name of the RNTuple
  const std::string& GetName()  const;
  /// Get the description of the RNTuple
  const std::string& GetDesc()  const;
  /// Get the prefix of the RNTuple
  const std::string& GetPrefix()const;
  /// Get the object type
  const std::string& GetType()  const;

  /// Set prescaling parameters
  void SetPrescaling(UInt_t num_to_save, UInt_t num_to_skip);

  /// Print the RNTuple name and description
  void Print() const;

  ROOT::RNTupleWriter* GetWriter() const { return fWriter.get(); };
  uint64_t GetNEntriesFilled() const { return fWriter->GetNEntries(); }


private:
  /// RNTuple model and writer
  std::unique_ptr<ROOT::RNTupleModel> fModel;
  std::unique_ptr<ROOT::RNTupleWriter> fWriter;

  /// Vector of values and shared field pointers (for RNTuple)
  std::vector<Double_t> fVector;
  std::vector<std::shared_ptr<Double_t>> fFieldPtrs;

  /// Name, description, prefix
  const std::string fName;
  const std::string fDesc;
  const std::string fPrefix;

  /// Object type
  std::string fType;

  /// RNTuple prescaling parameters
  UInt_t fCurrentEvent;
  UInt_t fNumEventsCycle;
  UInt_t fNumEventsToSave;
  UInt_t fNumEventsToSkip;

};

/// Template Implementation ///
template <typename T>
QwRootNTuple::QwRootNTuple(const std::string& name, const std::string& desc, T& object, const std::string& prefix)
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
  ConstructFieldsAndVector(object);
}

template < class T >
void QwRootNTuple::ConstructFieldsAndVector(T& object)
{
  // TODO:
  // Reserve space for the field vector
  fVector.reserve(RNTUPLE_MAX_SIZE);

  // Associate fields with vector - now using shared field pointers
  TString prefix = Form("%s", fPrefix.c_str());
  object.ConstructNTupleAndVector(fModel, prefix, fVector, fFieldPtrs);

  // Store the type of object
  fType = typeid(object).name();

  // Check memory reservation
  if (fVector.size() > RNTUPLE_MAX_SIZE) {
    QwError << "The field vector is too large: " << fVector.size() << " fields!  "
            << "The maximum size is " << RNTUPLE_MAX_SIZE << "."
            << QwLog::endl;
    exit(-1);
  }

  // Shrink memory reservation
  fVector.shrink_to_fit();
}

template < class T >
void QwRootNTuple::FillNTupleFields(const T& object)
{
  if (typeid(object).name() == fType) {
    // Fill the field vector
    object.FillNTupleVector(fVector);

    // Use the shared field pointers which remain valid
    if (fWriter) {
      for (size_t i = 0; i < fVector.size() && i < fFieldPtrs.size(); ++i) {
        if (fFieldPtrs[i]) {
          *(fFieldPtrs[i]) = fVector[i];
        }
      }

      // CRITICAL: Actually commit the data to the RNTuple
      fWriter->Fill();

      // Update event counter
      fCurrentEvent++;
      // RNTuple prescaling
      if (fNumEventsCycle > 0) {
        fCurrentEvent %= fNumEventsCycle;
      }
    } else {
      QwError << "RNTuple writer not initialized for " << fName << QwLog::endl;
    }
  } else {
    QwError << "Attempting to fill RNTuple vector for type " << fType << " with "
            << "object of type " << typeid(object).name() << QwLog::endl;
    exit(-1);
  }
}

#endif // HAS_RNTUPLE_SUPPORT
