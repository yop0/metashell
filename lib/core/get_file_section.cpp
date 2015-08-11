#include <metashell/get_file_section.hpp>

#include <fstream>
#include <sstream>

namespace metashell {

file_section get_file_section(
  std::istream& stream, int middle_line, int offset)
{
  file_section result;

  std::string line;
  int i = 1;
  for (; std::getline(stream, line); ++i) {
    if (i > middle_line + offset) {
      break;
    }
    if (i < middle_line - offset) {
      continue;
    }
    result.push_back({i, line});
  }
  if (i-1 < middle_line) {
    return {};
  }
  return result;
}

file_section get_file_section_from_file(
  const std::string& file_name, int middle_line, int offset)
{
  std::ifstream in(file_name);
  if (!in) {
    return {};
  }
  return get_file_section(in, middle_line, offset);
}

file_section get_file_section_from_buffer(
  const std::string& buffer, int middle_line, int offset)
{
  std::stringstream ss(buffer);
  return get_file_section(ss, middle_line, offset);
}

} // namespace metashell
