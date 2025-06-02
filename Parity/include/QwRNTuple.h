/********************************************************\
* File: QwRNTuple.h                                        *
*                                                          *
* Class to manage ROOT RNTuple data storage                *
* Based on ROOT's RNTuple documentation                    *
\********************************************************/

#ifndef __QWRNTUPLE__
#define __QWRNTUPLE__

// System headers
#include <vector>
#include <memory>

// ROOT headers
#include <TString.h>
#include <ROOT/RNTuple.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <ROOT/RNTupleWriter.hxx>

/**
 * A class to manage ROOT RNTuple storage for QwAnalysis
 */
class QwRNTuple {
public:
  QwRNTuple(const TString& name);
  virtual ~QwRNTuple();

  // Create a field in the RNTuple model
  template<typename T>
  std::shared_ptr<T> MakeField(const TString& name) {
    return fModel->MakeField<T>(name.Data());
  }

  // Add a field to the RNTuple model
  template<typename T>
  void AddField(const std::string& fieldName) {
    if (!fModel) {
      // Error handling would go here
      return;
    }
    auto field = fModel->MakeField<T>(fieldName);
  }

  // Get the RNTuple model
  std::shared_ptr<ROOT::RNTupleModel> GetModel() const {
    return fModel;
  }

  // Fill the RNTuple
  void Fill();

  // Add field from vector of doubles
  void AddValueFrom(const TString& fieldName, const std::vector<Double_t>& values, size_t index);

  // Set the output file
  void SetOutputFile(const TString& filename);

  // Close the RNTuple
  void Close();

private:
  TString fName;
  std::shared_ptr<ROOT::RNTupleModel> fModel;
  std::unique_ptr<ROOT::RNTupleWriter> fWriter;
};

#endif // __QWRNTUPLE__
