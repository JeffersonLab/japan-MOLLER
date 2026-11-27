/*!
 * \file   QwRootNTupleFields.cc
 * \brief  Helper for declaring the RNTuple fields of a channel's values
 */

#include "QwRootNTupleFields.h"

#ifdef HAS_RNTUPLE_SUPPORT

#include <ROOT/REntry.hxx>
#include <ROOT/RField.hxx>

QwRootNTupleFields::Layout QwRootNTupleFields::fgLayout = QwRootNTupleFields::Layout::kFieldPerValue;

std::string QwRootNTupleFields::ElementName(const std::string& suffix)
{
  if (suffix.empty()) return "value";
  return (suffix[0] == '_') ? suffix.substr(1) : suffix;
}

void QwRootNTupleFields::AddChannel(
    std::unique_ptr<ROOT::RNTupleModel>& model,
    const TString& basename,
    const std::vector<std::string>& suffixes,
    std::vector<Double_t>& values,
    std::vector<std::shared_ptr<Double_t>>& fieldPtrs)
{
  if (suffixes.empty()) return;

  if (fgLayout == Layout::kFieldPerValue || suffixes.size() == 1) {
    for (const auto& suffix : suffixes) {
      values.push_back(0.0);
      fieldPtrs.push_back(model->MakeField<Double_t>((basename + suffix.c_str()).Data()));
    }
    return;
  }

  const std::string name = basename.Data();
  const std::string type = "std::array<double," + std::to_string(suffixes.size()) + ">";
  auto field = ROOT::RFieldBase::Create(name, type).Unwrap();
  std::string description;
  for (const auto& suffix : suffixes) {
    if (!description.empty()) description += ':';
    description += ElementName(suffix);
  }
  field->SetDescription(description);
  model->AddField(std::move(field));

  // The first slot carries the array field (its buffer address identifies
  // the field); the remaining slots belong to the same array
  auto ptr = model->GetDefaultEntry().GetPtr<void>(name);
  fieldPtrs.push_back(std::shared_ptr<Double_t>(ptr, static_cast<Double_t*>(ptr.get())));
  values.push_back(0.0);
  for (size_t i = 1; i < suffixes.size(); ++i) {
    fieldPtrs.push_back(nullptr);
    values.push_back(0.0);
  }
}

#endif // HAS_RNTUPLE_SUPPORT
