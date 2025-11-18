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
/**
 *  \class QwRootTreeBranchVector
 *  \ingroup QwAnalysis
 *  \brief A helper class to manage a vector of branch entries for ROOT trees
 *
 * This class provides functionality to manage a collection of branch entries,
 * including their names, types, offsets, and sizes. It supports adding new entries,
 * accessing entries by index or name, and generating leaf lists for ROOT trees.
 */

using size_type = std::size_t;
class QwRootTreeBranchVector {
public:
  struct Entry {
    std::string name;
    std::size_t offset;
    std::size_t size;
    char type;
  };

  QwRootTreeBranchVector() = default;

  void reserve(size_type count);
  void shrink_to_fit();
  void clear();
  size_type size() const noexcept;
  bool empty()     const noexcept;

  template <typename T = uint8_t>
  const T& operator[](size_type index) const;

  template <typename T = uint8_t>
  T& operator[](size_type index);

  template <typename T>
  T& value(size_type index);

  template <typename T>
  const T& value(size_type index) const;

  // Disble Implicit Conversion
  template<typename T>
  void SetValue(size_type index, T val) = delete;
  void SetValue(size_type index, Double_t val);
  void SetValue(size_type index, Float_t val);
  void SetValue(size_type index, Int_t val);
  void SetValue(size_type index, Long64_t val);
  void SetValue(size_type index, Short_t val);
  void SetValue(size_type index, UShort_t val);
  void SetValue(size_type index, UInt_t val);
  void SetValue(size_type index, ULong64_t val);

  void* data() noexcept;
  const void* data() const noexcept;
  size_type data_size() const noexcept;

  template <typename T> T& back();
  template <typename T> const T& back() const;

  // Overload for TString (ROOT's string class)
  void push_back(const   TString   &name, const char type = 'D');
  void push_back(const std::string &name, const char type = 'D');
  void push_back(const    char*     name, const char type = 'D');

  std::string LeafList(size_type start_index = 0) const;
  std::string Dump(size_type start_index = 0, size_type end_index = 0) const;

private:
  static std::size_t GetTypeSize(char type);
  static std::size_t AlignOffset(std::size_t offset);

  std::vector<Entry> m_entries;
  std::vector<std::uint8_t> m_buffer;

  std::string FormatValue(const Entry& entry, size_type index) const;

  template <typename T>
  static std::string FormatNumeric(T input);
};

/// IMPLEMENTATION ///
template <typename T = uint8_t>
const T& QwRootTreeBranchVector::operator[](size_type index) const
{
  return value<T>(index);
}
template <typename T = uint8_t>
T& QwRootTreeBranchVector::operator[](size_type index)
{
  return value<T>(index);
}

template <typename T>
T& QwRootTreeBranchVector::value(size_type index)
{
  auto& entry = m_entries.at(index);
  return *reinterpret_cast<T*>(m_buffer.data() + entry.offset);
}

template <typename T>
const T& QwRootTreeBranchVector::value(size_type index) const
{
  const auto& entry = m_entries.at(index);
  return *reinterpret_cast<const T*>(m_buffer.data() + entry.offset);
}
template <typename T>
T& QwRootTreeBranchVector::back()
{
  if (m_entries.empty()) {
    throw std::out_of_range("QwRootTreeBranchVector::back() called on empty container");
  }
  const auto& last_entry = m_entries.back();
  return *reinterpret_cast<T*>(m_buffer.data() + last_entry.offset);
}

template <typename T>
const T& QwRootTreeBranchVector::back() const
{
  if (m_entries.empty()) {
    throw std::out_of_range("QwRootTreeBranchVector::back() called on empty container");
  }
  const auto& last_entry = m_entries.back();
  return *reinterpret_cast<const T*>(m_buffer.data() + last_entry.offset);
}
template <typename T>
std::string QwRootTreeBranchVector::FormatNumeric(T input) {
  std::ostringstream stream;
  stream << input;
  return stream.str();
}

