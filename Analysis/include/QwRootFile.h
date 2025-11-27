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
#include "TKey.h"
#include "TPRegexp.h"
#include "TSystem.h"
#include "TString.h"

// RNTuple headers (modern ROOT namespace) - only if supported
#ifdef HAS_RNTUPLE_SUPPORT
#include "ROOT/RNTuple.hxx"
#include "ROOT/RNTupleModel.hxx"
#include "ROOT/RField.hxx"
#include "ROOT/RNTupleWriter.hxx"
#endif

// Qweak headers
#include "QwOptions.h"
#include "TMapFile.h"

// If one defines more than this number of words in the full ntuple,
// the results are going to get very very crazy.
#define BRANCH_VECTOR_MAX_SIZE 25000

/**
 *  \class QwRootTreeBranchVector
 *  \ingroup QwAnalysis
 *  \brief A helper class to manage a vector of branch entries for ROOT trees
 *
 * This class provides functionality to manage a collection of branch entries,
 * including their names, types, offsets, and sizes. It supports adding new entries,
 * accessing entries by index or name, and generating leaf lists for ROOT trees.
 */
class QwRootTreeBranchVector {
public:
  struct Entry {
    std::string name;
    std::size_t offset;
    std::size_t size;
    char type;
  };

  using size_type = std::size_t;

  QwRootTreeBranchVector() = default;

  void reserve(size_type count) {
    m_entries.reserve(count);
    m_buffer.reserve(sizeof(double)*count);
  }

  void shrink_to_fit() {
    m_entries.shrink_to_fit();
    m_buffer.shrink_to_fit();
  }

  void clear() {
    m_entries.clear();
    m_buffer.clear();
  }

  size_type size() const noexcept { return m_entries.size(); }
  bool empty() const noexcept { return m_entries.empty(); }

  template <typename T = uint8_t>
  const T& operator[](size_type index) const {
    return value<T>(index);
  }

  template <typename T = uint8_t>
  T& operator[](size_type index) {
    return value<T>(index);
  }

  template <typename T>
  T& value(size_type index) {
    auto& entry = m_entries.at(index);
    return *reinterpret_cast<T*>(m_buffer.data() + entry.offset);
  }

  template <typename T>
  const T& value(size_type index) const {
    const auto& entry = m_entries.at(index);
    return *reinterpret_cast<const T*>(m_buffer.data() + entry.offset);
  }

  // Explicit SetValue overloads with type checking to prevent automatic conversions
  // (presence of multiple overloads ensures a compilation error if the types do not match)
  void SetValue(size_type index, Double_t val) {
    const auto& entry = m_entries.at(index);
    if (entry.type != 'D') {
      throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store double value '" + entry.name + "'");
    }
    this->value<Double_t>(index) = val;
  }

  void SetValue(size_type index, Float_t val) {
    const auto& entry = m_entries.at(index);
    if (entry.type != 'F') {
      throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store float value '" + entry.name + "'");
    }
    this->value<Float_t>(index) = val;
  }

  void SetValue(size_type index, Int_t val) {
    const auto& entry = m_entries.at(index);
    if (entry.type != 'I') {
      throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store int value '" + entry.name + "'");
    }
    this->value<Int_t>(index) = val;
  }

  void SetValue(size_type index, Long64_t val) {
    const auto& entry = m_entries.at(index);
    if (entry.type != 'L') {
      throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store long long value '" + entry.name + "'");
    }
    this->value<Long64_t>(index) = val;
  }

  void SetValue(size_type index, Short_t val) {
    const auto& entry = m_entries.at(index);
    if (entry.type != 'S') {
      throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store short value '" + entry.name + "'");
    }
    this->value<Short_t>(index) = val;
  }

  // Unsigned type overloads
  void SetValue(size_type index, UShort_t val) {
    const auto& entry = m_entries.at(index);
    if (entry.type != 's') {
      throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store short value '" + entry.name + "'");
    }
    this->value<UShort_t>(index) = val;
  }

  void SetValue(size_type index, UInt_t val) {
    const auto& entry = m_entries.at(index);
    if (entry.type != 'i') {
      throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store unsigned int value '" + entry.name + "'");
    }
    this->value<UInt_t>(index) = val;
  }

  void SetValue(size_type index, ULong64_t val) {
    const auto& entry = m_entries.at(index);
    if (entry.type != 'l') {
      throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store long long value '" + entry.name + "'");
    }
    this->value<ULong64_t>(index) = val;
  }

  void* data() noexcept { return m_buffer.data(); }
  const void* data() const noexcept { return m_buffer.data(); }
  size_type data_size() const noexcept { return m_buffer.size(); }

  template <typename T>
  T& back() {
    if (m_entries.empty()) {
      throw std::out_of_range("QwRootTreeBranchVector::back() called on empty container");
    }
    const auto& last_entry = m_entries.back();
    return *reinterpret_cast<T*>(m_buffer.data() + last_entry.offset);
  }

  template <typename T>
  const T& back() const {
    if (m_entries.empty()) {
      throw std::out_of_range("QwRootTreeBranchVector::back() called on empty container");
    }
    const auto& last_entry = m_entries.back();
    return *reinterpret_cast<const T*>(m_buffer.data() + last_entry.offset);
  }

  void push_back(const std::string& name, const char type = 'D') {
    const std::size_t entry_size = GetTypeSize(type);
    const std::size_t offset = AlignOffset(m_buffer.size());

    if (offset > m_buffer.capacity()) {
      throw std::out_of_range("QwRootTreeBranchVector::push_back() requires buffer resize beyond reserved capacity");
    }
    if (offset > m_buffer.size()) {
      m_buffer.resize(offset, 0u);
    }

    Entry entry{name, offset, entry_size, type};
    m_entries.push_back(entry);

    const std::size_t required = offset + entry_size;
    if (required > m_buffer.capacity()) {
      throw std::out_of_range("QwRootTreeBranchVector::push_back() requires buffer resize beyond reserved capacity");
    }
    if (required > m_buffer.size()) {
      m_buffer.resize(required, 0u);
    }
  }

  // Overload for TString (ROOT's string class)
  void push_back(const TString& name, const char type = 'D') {
    push_back(std::string(name.Data()), type);
  }

  // Overload for const char* (string literals)
  void push_back(const char* name, const char type = 'D') {
    push_back(std::string(name), type);
  }

  std::string LeafList(size_type start_index = 0) const {
    static const std::string separator = ":";
    std::ostringstream stream;
    bool first = true;
    for (size_type index = start_index; index < m_entries.size(); ++index) {
      const auto& entry = m_entries[index];
      if (!first) {
        stream << separator;
      }
      stream << entry.name << "/" << entry.type;
      first = false;
    }
    return stream.str();
  }

  std::string Dump(size_type start_index = 0, size_type end_index = 0) const {
    std::ostringstream stream;
    stream << "QwRootTreeBranchVector: " << m_entries.size() << " entries, "
           << m_buffer.size() << " bytes\n";
    size_t end_offset = (end_index == 0 || end_index > m_entries.size()) ?
      m_buffer.size()
      : m_entries[end_index - 1].offset + m_entries[end_index - 1].size;
    stream << "QwRootTreeBranchVector: buffer at 0x" << std::hex << (void*) &m_buffer[0] << '\n';
    stream << "QwRootTreeBranchVector: entries at 0x" << std::hex << (void*) &m_entries[0] << '\n';
    for (size_t offset = m_entries[start_index].offset; offset < end_offset; offset += 4) {
      stream << std::dec
             << "  [" << offset << "] "
             << std::hex
             << " offset=0x" << offset
             << " (0x" << std::setw(4) << std::setfill('0')
                       << offset - m_entries[start_index].offset << ")"
             << " buff=";
      // Little-endian
      for (std::size_t byte = 0; byte < 4; ++byte) {
        stream << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<unsigned int>(m_buffer[offset + byte])
               << " ";
      }
      stream << '\n';
    }
    end_index = (end_index == 0 || end_index > m_entries.size()) ?
      m_entries.size()
      : end_index;
    for (size_type index = start_index; index < end_index; ++index) {
      const auto& entry = m_entries[index];
      stream << std::dec
             << "  [" << index << "] "
             << std::hex
             << " offset=0x" << entry.offset
             << " (0x" << std::setw(4) << std::setfill('0')
                       << entry.offset - m_entries[start_index].offset << ")"
             << " size=0x" << entry.size
             << " buff=0x";
      // Little-endian
      for (std::size_t byte = GetTypeSize(entry.type); byte > 0; --byte) {
        stream << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<unsigned int>((m_buffer.data() + entry.offset)[byte - 1]);
      }
      stream << std::dec
             << " name=" << entry.name << "/" << entry.type
             << " value=" << FormatValue(entry, index);
      stream << '\n';
    }
    return stream.str();
  }

private:
  static std::size_t GetTypeSize(char type) {
    switch (type) {
      case 'D':
        return sizeof(double);
      case 'F':
        return sizeof(float);
      case 'L':
        return sizeof(long long);
      case 'l':
        return sizeof(unsigned long long);
      case 'I':
        return sizeof(int);
      case 'i':
        return sizeof(unsigned int);
      case 'S':
        return sizeof(short);
      case 's':
        return sizeof(unsigned short);
      default:
        throw std::invalid_argument("Unsupported branch type code: " + std::string(1, type));
    }
  }

  static std::size_t AlignOffset(std::size_t offset) {
    const std::size_t alignment = 4u;
    return (offset + (alignment - 1u)) & ~(alignment - 1u);
  }

  std::vector<Entry> m_entries;
  std::vector<std::uint8_t> m_buffer;

  std::string FormatValue(const Entry& entry, size_type index) const {
    switch (entry.type) {
      case 'D':
        return FormatNumeric(value<double>(index));
      case 'F':
        return FormatNumeric(value<float>(index));
      case 'L':
        return FormatNumeric(value<long long>(index));
      case 'l':
        return FormatNumeric(value<unsigned long long>(index));
      case 'I':
        return FormatNumeric(value<int>(index));
      case 'i':
        return FormatNumeric(value<unsigned int>(index));
      case 'S':
        return FormatNumeric(value<short>(index));
      case 's':
        return FormatNumeric(value<unsigned short>(index));
      default:
        return "<unknown>";
    }
  }

  template <typename T>
  static std::string FormatNumeric(T input) {
    std::ostringstream stream;
    stream << input;
    return stream.str();
  }
};

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
class QwRootTree {

  public:

    /// Constructor with name, and description
    QwRootTree(const std::string& name, const std::string& desc, const std::string& prefix = "")
    : fName(name),fDesc(desc),fPrefix(prefix),fType("type undefined"),
      fCurrentEvent(0),fNumEventsCycle(0),fNumEventsToSave(0),fNumEventsToSkip(0) {
      // Construct tree
      ConstructNewTree();
    }

    /// Constructor with existing tree
    QwRootTree(const QwRootTree* tree, const std::string& prefix = "")
    : fName(tree->GetName()),fDesc(tree->GetDesc()),fPrefix(prefix),fType("type undefined"),
      fCurrentEvent(0),fNumEventsCycle(0),fNumEventsToSave(0),fNumEventsToSkip(0) {
      QwMessage << "Existing tree: " << tree->GetName() << ", " << tree->GetDesc() << QwLog::endl;
      fTree = tree->fTree;
    }

    /// Constructor with name, description, and object
    template < class T >
    QwRootTree(const std::string& name, const std::string& desc, T& object, const std::string& prefix = "")
    : fName(name),fDesc(desc),fPrefix(prefix),fType("type undefined"),
      fCurrentEvent(0),fNumEventsCycle(0),fNumEventsToSave(0),fNumEventsToSkip(0) {
      // Construct tree
      ConstructNewTree();

      // Construct branch
      ConstructUnitsBranch();

      // Construct branches and vector
      ConstructBranchAndVector(object);
    }

    /// Constructor with existing tree, and object
    template < class T >
    QwRootTree(const QwRootTree* tree, T& object, const std::string& prefix = "")
    : fName(tree->GetName()),fDesc(tree->GetDesc()),fPrefix(prefix),fType("type undefined"),
      fCurrentEvent(0),fNumEventsCycle(0),fNumEventsToSave(0),fNumEventsToSkip(0) {
      QwMessage << "Existing tree: " << tree->GetName() << ", " << tree->GetDesc() << QwLog::endl;
      fTree = tree->fTree;

      // Construct branches and vector
      ConstructBranchAndVector(object);
    }

    /// Destructor
    virtual ~QwRootTree() { }


  private:

    static const TString kUnitsName;
    static Double_t kUnitsValue[];

    /// Construct the tree
    void ConstructNewTree() {
      QwMessage << "New tree: " << fName << ", " << fDesc << QwLog::endl;

      fTree = new TTree(fName.c_str(), fDesc.c_str());

      // Ensure tree is in the current directory
      if (gDirectory) {
        fTree->SetDirectory(gDirectory);

      } else {

      }
    }

    void ConstructUnitsBranch() {
      std::string name = "units";
      fTree->Branch(name.c_str(), &kUnitsValue, kUnitsName);
    }

    /// Construct index from this tree to another tree
    void ConstructIndexTo(QwRootTree* to) {
      std::string name = "previous_entry_in_" + to->fName;
      fTree->Branch(name.c_str(), &(to->fCurrentEvent));
    }

    /// Construct the branches and vector for generic objects
    template < class T >
    void ConstructBranchAndVector(T& object) {
      // Reserve space for the branch vector
      fVector.reserve(BRANCH_VECTOR_MAX_SIZE);
      // Associate branches with vector
      TString prefix = Form("%s",fPrefix.c_str());
      object.ConstructBranchAndVector(fTree, prefix, fVector);

      // Store the type of object
      fType = typeid(object).name();

      // Check memory reservation
      if (fVector.size() > BRANCH_VECTOR_MAX_SIZE) {
        QwError << "The branch vector is too large: " << fVector.size() << " leaves!  "
                << "The maximum size is " << BRANCH_VECTOR_MAX_SIZE << "."
                << QwLog::endl;
        exit(-1);
      }
    }


  public:

    /// Fill the branches for generic objects
    template < class T >
    void FillTreeBranches(const T& object) {
      if (typeid(object).name() == fType) {
        // Fill the branch vector
        object.FillTreeVector(fVector);
      } else {
        QwError << "Attempting to fill tree vector for type " << fType << " with "
                << "object of type " << typeid(object).name() << QwLog::endl;
        exit(-1);
      }
    }

    Long64_t AutoSave(Option_t *option){
      return fTree->AutoSave(option);
    }

    /// Fill the tree
    Int_t Fill() {
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


    /// Print the tree name and description
    void Print() const {
      QwMessage << GetName() << ", " << GetType();
      if (fPrefix != "")
        QwMessage << " (prefix " << GetPrefix() << ")";
      QwMessage << QwLog::endl;
    }

    /// Get the tree pointer for low level operations
    TTree* GetTree() const { return fTree; };


  friend class QwRootFile;

  private:

    /// Tree pointer
    TTree* fTree;
    /// Vector of leaves
    QwRootTreeBranchVector fVector;


    /// Name, description
    const std::string fName;
    const std::string fDesc;
    const std::string fPrefix;

    /// Get the name of the tree
    const std::string& GetName() const { return fName; };
    /// Get the description of the tree
    const std::string& GetDesc() const { return fDesc; };
    /// Get the description of the tree
    const std::string& GetPrefix() const { return fPrefix; };


    /// Object type
    std::string fType;

    /// Get the object type
    std::string GetType() const { return fType; };


    /// Tree prescaling parameters
    UInt_t fCurrentEvent;
    UInt_t fNumEventsCycle;
    UInt_t fNumEventsToSave;
    UInt_t fNumEventsToSkip;

    /// Set tree prescaling parameters
    void SetPrescaling(UInt_t num_to_save, UInt_t num_to_skip) {
      fNumEventsToSave = num_to_save;
      fNumEventsToSkip = num_to_skip;
      fNumEventsCycle = fNumEventsToSave + fNumEventsToSkip;
    }


    /// Maximum tree size, autoflush and autosave
    Long64_t fMaxTreeSize;
    Long64_t fAutoFlush;
    Long64_t fAutoSave;
    Int_t fBasketSize;

    /// Set maximum tree size
    void SetMaxTreeSize(Long64_t maxsize = 1900000000) {
      fMaxTreeSize = maxsize;
      if (fTree) fTree->SetMaxTreeSize(maxsize);
    }

    /// Set autoflush size
    void SetAutoFlush(Long64_t autoflush = 30000000) {
      fAutoFlush = autoflush;
      #if ROOT_VERSION_CODE >= ROOT_VERSION(5,26,00)
        if (fTree) fTree->SetAutoFlush(autoflush);
      #endif
    }

    /// Set autosave size
    void SetAutoSave(Long64_t autosave = 300000000) {
      fAutoSave = autosave;
      if (fTree) fTree->SetAutoSave(autosave);
    }

    /// Set basket size
    void SetBasketSize(Int_t basketsize = 16000) {
      fBasketSize = basketsize;
      if (fTree) fTree->SetBasketSize("*",basketsize);
    }

    //Set circular buffer size for the memory resident tree
    void SetCircular(Long64_t buff = 100000) {
      if (fTree) fTree->SetCircular(buff);
    }
};

#ifdef HAS_RNTUPLE_SUPPORT
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
    QwRootNTuple(const std::string& name, const std::string& desc, const std::string& prefix = "")
    : fName(name), fDesc(desc), fPrefix(prefix), fType("type undefined"),
      fCurrentEvent(0), fNumEventsCycle(0), fNumEventsToSave(0), fNumEventsToSkip(0) {
      // Create RNTuple model
      fModel = ROOT::RNTupleModel::Create();
    }

    /// Constructor with name, description, and object
    template < class T >
    QwRootNTuple(const std::string& name, const std::string& desc, T& object, const std::string& prefix = "")
    : fName(name), fDesc(desc), fPrefix(prefix), fType("type undefined"),
      fCurrentEvent(0), fNumEventsCycle(0), fNumEventsToSave(0), fNumEventsToSkip(0) {
      // Create RNTuple model
      fModel = ROOT::RNTupleModel::Create();

      // Construct fields and vector
      ConstructFieldsAndVector(object);
    }

    /// Destructor
    virtual ~QwRootNTuple() {
      Close();
    }

    /// Close and finalize the RNTuple writer
    void Close() {
      if (fWriter) {
        // Explicitly commit any remaining data and close the writer
        // This ensures all data is written to the file before destruction
        fWriter.reset();  // This calls the destructor which should finalize the RNTuple
      }
    }

  private:

    /// Construct the fields and vector for generic objects
    template < class T >
    void ConstructFieldsAndVector(T& object) {
      // Reserve space for the field vector
      fVector.reserve(BRANCH_VECTOR_MAX_SIZE);

      // Associate fields with vector - now using shared field pointers
      TString prefix = Form("%s", fPrefix.c_str());
      object.ConstructNTupleAndVector(fModel, prefix, fVector, fFieldPtrs);

      // Store the type of object
      fType = typeid(object).name();

      // Check memory reservation
      if (fVector.size() > BRANCH_VECTOR_MAX_SIZE) {
        QwError << "The field vector is too large: " << fVector.size() << " fields!  "
                << "The maximum size is " << BRANCH_VECTOR_MAX_SIZE << "."
                << QwLog::endl;
        exit(-1);
      }

      // Shrink memory reservation
      fVector.shrink_to_fit();
    }

  public:

    /// Initialize the RNTuple writer with a file
    void InitializeWriter(TFile* file) {
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

    /// Fill the fields for generic objects
    template < class T >
    void FillNTupleFields(const T& object) {
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

    /// Fill the RNTuple (called by FillTree wrapper methods)
    void Fill() {
      // This method is now called indirectly - the actual filling happens in FillNTupleFields
      // Just here for compatibility with the tree interface
    }

    /// Get the name of the RNTuple
    const std::string& GetName() const { return fName; }
    /// Get the description of the RNTuple
    const std::string& GetDesc() const { return fDesc; }
    /// Get the prefix of the RNTuple
    const std::string& GetPrefix() const { return fPrefix; }
    /// Get the object type
    std::string GetType() const { return fType; }

    /// Set prescaling parameters
    void SetPrescaling(UInt_t num_to_save, UInt_t num_to_skip) {
      fNumEventsToSave = num_to_save;
      fNumEventsToSkip = num_to_skip;
      fNumEventsCycle = fNumEventsToSave + fNumEventsToSkip;
    }

    /// Print the RNTuple name and description
    void Print() const {
      QwMessage << GetName() << ", " << GetType();
      if (fPrefix != "")
        QwMessage << " (prefix " << GetPrefix() << ")";
      QwMessage << QwLog::endl;
    }

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

  friend class QwRootFile;
};
#endif // HAS_RNTUPLE_SUPPORT

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
    void FillHistograms(T& object) {
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


    /// Create a new tree with name and description
    void NewTree(const std::string& name, const std::string& desc) {
      if (IsTreeDisabled(name)) return;
      this->cd();
      QwRootTree *tree = 0;
      if (! HasTreeByName(name)) {
        tree = new QwRootTree(name,desc);
      } else {
        tree = new QwRootTree(fTreeByName[name].front());
      }
      fTreeByName[name].push_back(tree);
    }

#ifdef HAS_RNTUPLE_SUPPORT
    /// Create a new RNTuple with name and description
    void NewNTuple(const std::string& name, const std::string& desc) {
      if (IsTreeDisabled(name) || !fEnableRNTuples) return;
      QwRootNTuple *ntuple = 0;
      if (! HasNTupleByName(name)) {
        ntuple = new QwRootNTuple(name, desc);
        // Initialize the writer with our file
        ntuple->InitializeWriter(fRootFile);
      } else {
        // For simplicity, don't support copying existing RNTuples yet
        QwError << "Cannot create duplicate RNTuple: " << name << QwLog::endl;
        return;
      }
      fNTupleByName[name].push_back(ntuple);
    }
#endif // HAS_RNTUPLE_SUPPORT

    /// Get the tree with name
    TTree* GetTree(const std::string& name) {
      if (! HasTreeByName(name)) return 0;
      else return fTreeByName[name].front()->GetTree();
    }

    /// Fill the tree with name
    Int_t FillTree(const std::string& name) {
      if (! HasTreeByName(name)) return 0;
      else return fTreeByName[name].front()->Fill();
    }

    /// Fill all registered trees
    Int_t FillTrees() {
      // Loop over all registered tree names
      Int_t retval = 0;
      std::map< const std::string, std::vector<QwRootTree*> >::iterator iter;
      for (iter = fTreeByName.begin(); iter != fTreeByName.end(); iter++) {
        retval += iter->second.front()->Fill();
      }
      return retval;
    }

#ifdef HAS_RNTUPLE_SUPPORT
    /// Fill the RNTuple with name
    void FillNTuple(const std::string& name) {
      if (HasNTupleByName(name)) {
        fNTupleByName[name].front()->Fill();
      }
    }
#endif // HAS_RNTUPLE_SUPPORT

#ifdef HAS_RNTUPLE_SUPPORT
    /// Fill all registered RNTuples
    void FillNTuples() {
      // Loop over all registered RNTuple names
      std::map< const std::string, std::vector<QwRootNTuple*> >::iterator iter;
      for (iter = fNTupleByName.begin(); iter != fNTupleByName.end(); iter++) {
        iter->second.front()->Fill();
      }
    }
#endif // HAS_RNTUPLE_SUPPORT

    /// Print registered trees
    void PrintTrees() const {
      QwMessage << "Trees: " << QwLog::endl;
      // Loop over all registered tree names
      std::map< const std::string, std::vector<QwRootTree*> >::const_iterator iter;
      for (iter = fTreeByName.begin(); iter != fTreeByName.end(); iter++) {
        QwMessage << iter->first << ": " << iter->second.size()
                  << " objects registered" << QwLog::endl;
        // Loop over all registered objects for this tree
        std::vector<QwRootTree*>::const_iterator tree;
        for (tree = iter->second.begin(); tree != iter->second.end(); tree++) {
          (*tree)->Print();
        }
      }
    }
    /// Print registered histogram directories
    void PrintDirs() const {
      QwMessage << "Dirs: " << QwLog::endl;
      // Loop ove rall registered directories
      std::map< const std::string, TDirectory* >::const_iterator iter;
      for (iter = fDirsByName.begin(); iter != fDirsByName.end(); iter++) {
        QwMessage << iter->first << QwLog::endl;
      }
    }


    /// Write any object to the ROOT file (only valid for TFile)
    template < class T >
    Int_t WriteObject(const T* obj, const char* name, Option_t* option = "", Int_t bufsize = 0) {
      Int_t retval = 0;
      // TMapFile has no support for WriteObject
      if (fRootFile) retval = fRootFile->WriteObject(obj,name,option,bufsize);
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
      }else{
	// this option will allow for reading the tree during write
	Long64_t nBytes(0);
	for (auto iter = fTreeByName.begin(); iter != fTreeByName.end(); iter++)
	  nBytes += iter->second.front()->AutoSave("SaveSelf");

	QwMessage << "TFile saved: "
                  << nBytes/1000000 << "MB (inaccurate number)" //FIXME this calculation is inaccurate
                  << QwLog::endl;
      }
    }
    void Print()  { if (fMapFile) fMapFile->Print();  if (fRootFile) fRootFile->Print(); }
    void ls()     { if (fMapFile) fMapFile->ls();     if (fRootFile) fRootFile->ls(); }
    void Map()    { if (fRootFile) fRootFile->Map(); }

  private:
    /// Recursively clear in-memory objects from a directory tree.
    /// This removes objects from the TDirectory's in-memory list without deleting
    /// their on-disk representation (keys). Used to prevent RNTupleWriter::Close()
    /// from creating duplicate histogram cycles when it internally calls TFile::Write().
    void ClearInMemoryObjects(TDirectory* dir) {
      if (!dir) return;
      
      // First, collect subdirectory names
      std::vector<TString> subdirs;
      TIter next(dir->GetListOfKeys());
      TKey* key;
      while ((key = (TKey*)next())) {
        if (TString(key->GetClassName()) == "TDirectoryFile") {
          subdirs.push_back(key->GetName());
        }
      }
      
      // Recursively clear subdirectories
      for (const auto& name : subdirs) {
        TDirectory* sub = dynamic_cast<TDirectory*>(dir->Get(name));
        if (sub) ClearInMemoryObjects(sub);
      }
      
      // Clear this directory's in-memory object list
      // "nodelete" option: remove from list but don't delete the TKey entries
      TList* list = dir->GetList();
      if (list) {
        list->Clear("nodelete");
      }
    }

  public:
    void Close()  {

      // Check if we should make the file permanent - restore original logic
      if (!fMakePermanent) fMakePermanent = HasAnyFilled();

      if (fRootFile) {
        // Step 1: Write all trees explicitly
        for (auto iter = fTreeByName.begin(); iter != fTreeByName.end(); iter++) {
          if (!iter->second.empty() && iter->second.front()) {
            TTree* tree = iter->second.front()->GetTree();
            if (tree && tree->GetEntries() > 0) {
              tree->Write();
            }
          }
        }

        // Step 2: Write all in-memory objects (histograms, etc.) to disk
        // Use kOverwrite to avoid creating duplicate cycles
        fRootFile->Write(0, TObject::kOverwrite);

        // Step 3: CRITICAL FIX for RNTuple histogram duplication
        // Clear all in-memory objects from the TFile's directory structure.
        // This prevents RNTupleWriter::Close() from re-writing histograms,
        // which would create duplicate cycles. The histograms are already
        // safely written to disk in Step 2.
#ifdef HAS_RNTUPLE_SUPPORT
        if (!fNTupleByName.empty()) {
          ClearInMemoryObjects(fRootFile);
        }
#endif // HAS_RNTUPLE_SUPPORT
      }

#ifdef HAS_RNTUPLE_SUPPORT
      // Step 4: Close all RNTuples
      // Now that in-memory histograms are cleared, the RNTupleWriter destructor
      // won't create duplicate histogram cycles when it internally writes to TFile
      for (auto& pair : fNTupleByName) {
        for (auto& ntuple : pair.second) {
          if (ntuple) ntuple->Close();
        }
      }
#endif // HAS_RNTUPLE_SUPPORT

      // Step 5: Close the file
      if (fRootFile) {
        fRootFile->Close();
      }

      if (fMapFile) fMapFile->Close();
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
    void DisableTree(const TString& regexp) {
      fDisabledTrees.push_back(regexp);
    }
    /// Does this tree name match a disabled tree name?
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
    bool HasTreeByName(const std::string& name) {
      if (fTreeByName.count(name) == 0) return false;
      else return true;
    }
    /// Is a tree registered for this type
    template < class T >
    bool HasTreeByType(const T& object) {
      const std::type_index type = typeid(object);
      if (fTreeByType.count(type) == 0) return false;
      else return true;
    }
    /// Is a tree registered for this object
    template < class T >
    bool HasTreeByAddr(const T& object) {
      const void* addr = static_cast<const void*>(&object);
      if (fTreeByAddr.count(addr) == 0) return false;
      else return true;
    }

#ifdef HAS_RNTUPLE_SUPPORT
    /// Is an RNTuple registered for this name
    bool HasNTupleByName(const std::string& name) {
      if (fNTupleByName.count(name) == 0) return false;
      else return true;
    }
    /// Is an RNTuple registered for this type
    template < class T >
    bool HasNTupleByType(const T& object) {
      const std::type_index type = typeid(object);
      if (fNTupleByType.count(type) == 0) return false;
      else return true;
    }
    /// Is an RNTuple registered for this object
    template < class T >
    bool HasNTupleByAddr(const T& object) {
      const void* addr = static_cast<const void*>(&object);
      if (fNTupleByAddr.count(addr) == 0) return false;
      else return true;
    }
#endif // HAS_RNTUPLE_SUPPORT

    /// Directories
    std::map< const std::string, TDirectory* > fDirsByName;
    std::map< const std::string, std::vector<std::string> > fDirsByType;

    /// Is a tree registered for this name
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
