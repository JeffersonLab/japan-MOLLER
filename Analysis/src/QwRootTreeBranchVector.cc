#include "QwRootTreeBranchVector.h"


void QwRootTreeBranchVector::reserve(size_type count)
{
	m_entries.reserve(count);
	m_buffer.reserve(sizeof(double)*count);
}

void QwRootTreeBranchVector::shrink_to_fit()
{
	m_entries.shrink_to_fit();
	m_buffer.shrink_to_fit();
}
void QwRootTreeBranchVector::clear()
{
    m_entries.clear();
    m_buffer.clear();
}

size_type QwRootTreeBranchVector::size()  const noexcept { return m_entries.size() ; }
bool      QwRootTreeBranchVector::empty() const noexcept { return m_entries.empty(); }

void QwRootTreeBranchVector::SetValue(size_type index, Double_t val)
{
  const auto& entry = m_entries.at(index);
  if (entry.type != 'D') {
    throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store double value '" + entry.name + "'");
  }
  this->value<Double_t>(index) = val;
}

void QwRootTreeBranchVector::SetValue(size_type index, Float_t val)
{
  const auto& entry = m_entries.at(index);
  if (entry.type != 'F') {
    throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store float value '" + entry.name + "'");
  }
  this->value<Float_t>(index) = val;
}

void QwRootTreeBranchVector::SetValue(size_type index, Int_t val)
{
  const auto& entry = m_entries.at(index);
  if (entry.type != 'I') {
    throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store int value '" + entry.name + "'");
  }
  this->value<Int_t>(index) = val;
}

void QwRootTreeBranchVector::SetValue(size_type index, Long64_t val)
{
  const auto& entry = m_entries.at(index);
  if (entry.type != 'L') {
    throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store long long value '" + entry.name + "'");
  }
  this->value<Long64_t>(index) = val;
}

void QwRootTreeBranchVector::SetValue(size_type index, Short_t val)
{
  const auto& entry = m_entries.at(index);
  if (entry.type != 'S') {
    throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store short value '" + entry.name + "'");
  }
  this->value<Short_t>(index) = val;
}

void QwRootTreeBranchVector::SetValue(size_type index, UShort_t val)
{
  const auto& entry = m_entries.at(index);
  if (entry.type != 's') {
    throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store short value '" + entry.name + "'");
  }
  this->value<UShort_t>(index) = val;
}

void QwRootTreeBranchVector::SetValue(size_type index, UInt_t val)
{
  const auto& entry = m_entries.at(index);
  if (entry.type != 'i') {
    throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store unsigned int value '" + entry.name + "'");
  }
  this->value<UInt_t>(index) = val;
}

void QwRootTreeBranchVector::SetValue(size_type index, ULong64_t val)
{
  const auto& entry = m_entries.at(index);
  if (entry.type != 'l') {
    throw std::invalid_argument("Type mismatch: entry type '" + std::string(1, entry.type) + "' cannot store long long value '" + entry.name + "'");
  }
  this->value<ULong64_t>(index) = val;
}

void*       QwRootTreeBranchVector::data()           noexcept { return m_buffer.data(); }
const void* QwRootTreeBranchVector::data()     const noexcept { return m_buffer.data(); }
size_type   QwRootTreeBranchVector::data_size()const noexcept { return m_buffer.size(); }

void QwRootTreeBranchVector::push_back(const std::string &name, const char type)
{
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
void QwRootTreeBranchVector::push_back(const TString& name, const char type)
{
  push_back(std::string(name.Data()), type);
}

void QwRootTreeBranchVector::push_back(const char* name, const char type)
{
	push_back(std::string(name), type);
}

std::string QwRootTreeBranchVector::LeafList(size_type start_index) const
{
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

std::string QwRootTreeBranchVector::Dump(size_type start_index, size_type end_index) const
{
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

std::size_t QwRootTreeBranchVector::GetTypeSize(char type)
{
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
std::size_t QwRootTreeBranchVector::AlignOffset(std::size_t offset)
{
  const std::size_t alignment = 4u;
  return (offset + (alignment - 1u)) & ~(alignment - 1u);
}

std::string QwRootTreeBranchVector::FormatValue(const Entry& entry, size_type index) const
{
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

