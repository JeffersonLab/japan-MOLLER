/*!
 * \file   QwCorrelatorNTuple.cc
 * \brief  RNTuple output of the correlator regression results
 */

#include "QwCorrelatorNTuple.h"

#ifdef HAS_RNTUPLE_SUPPORT

#include <ROOT/REntry.hxx>
#include <ROOT/RField.hxx>

#include "QwRootFile.h"

void QwCorrelatorNTuple::Begin(const std::string& prefix)
{
  fPrefix = prefix;
  fModel = ROOT::RNTupleModel::Create();
  fWriter = nullptr;
  fScalars32.clear();
  fScalars64.clear();
  fMatrices.clear();
  fVectors.clear();
}

void QwCorrelatorNTuple::AddScalar(const std::string& name, const Int_t* value)
{
  if (!fModel) return;
  fScalars32.push_back({value, fModel->MakeField<std::int32_t>(fPrefix + name)});
}

void QwCorrelatorNTuple::AddScalar(const std::string& name, const Long64_t* value)
{
  if (!fModel) return;
  fScalars64.push_back({value, fModel->MakeField<std::int64_t>(fPrefix + name)});
}

std::shared_ptr<std::vector<double>> QwCorrelatorNTuple::AddArrayField(
    const std::string& name, const std::string& description)
{
  auto field = ROOT::RFieldBase::Create(name, "std::vector<double>").Unwrap();
  field->SetDescription(description);
  fModel->AddField(std::move(field));
  return fModel->GetDefaultEntry().GetPtr<std::vector<double>>(name);
}

void QwCorrelatorNTuple::AddMatrix(const std::string& name, const TMatrixD* matrix)
{
  if (!fModel) return;
  const std::string full = fPrefix + name;
  Matrix m;
  m.source = matrix;
  m.elements = AddArrayField(full, "row-major matrix; dimensions in " + full + "_nrows and " + full + "_ncols");
  m.nrows = fModel->MakeField<std::int32_t>(full + "_nrows");
  m.ncols = fModel->MakeField<std::int32_t>(full + "_ncols");
  fMatrices.push_back(m);
}

void QwCorrelatorNTuple::AddVector(const std::string& name, const TVectorD* vector)
{
  if (!fModel) return;
  fVectors.push_back({vector, AddArrayField(fPrefix + name, "vector")});
}

bool QwCorrelatorNTuple::Create(QwRootFile* file, const std::string& name)
{
  fWriter = nullptr;
  if (file && fModel) fWriter = file->NewNTupleWriter(name, std::move(fModel));
  fModel.reset();
  return IsActive();
}

bool QwCorrelatorNTuple::Fill()
{
  if (!fWriter) return false;
  for (auto& s : fScalars32) *s.value = *s.source;
  for (auto& s : fScalars64) *s.value = *s.source;
  for (auto& m : fMatrices) {
    const Double_t* data = m.source->GetMatrixArray();
    m.elements->assign(data, data + m.source->GetNoElements());
    *m.nrows = m.source->GetNrows();
    *m.ncols = m.source->GetNcols();
  }
  for (auto& v : fVectors) {
    const Double_t* data = v.source->GetMatrixArray();
    v.elements->assign(data, data + v.source->GetNoElements());
  }
  fWriter->Fill();
  return true;
}

#endif // HAS_RNTUPLE_SUPPORT
