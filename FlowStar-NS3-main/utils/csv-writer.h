#ifndef CSV_WRITER_H
#define CSV_WRITER_H

#include <string>
#include <fstream>
#include <vector>

namespace ns3 {

class CsvWriter {
public:
  CsvWriter (std::string filename);
  ~CsvWriter ();

  void WriteHeader (const std::vector<std::string>& columns);
  void WriteRow (const std::vector<std::string>& data);

private:
  std::ofstream m_file;
};

} // namespace ns3

#endif /* CSV_WRITER_H */
