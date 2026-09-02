#include "csv-writer.h"
#include <iostream>

namespace ns3 {

CsvWriter::CsvWriter (std::string filename)
{
  m_file.open (filename);
  if (!m_file.is_open ())
    {
      std::cerr << "Failed to open " << filename << " for writing." << std::endl;
    }
}

CsvWriter::~CsvWriter ()
{
  if (m_file.is_open ())
    {
      m_file.close ();
    }
}

void
CsvWriter::WriteHeader (const std::vector<std::string>& columns)
{
  WriteRow (columns);
}

void
CsvWriter::WriteRow (const std::vector<std::string>& data)
{
  if (!m_file.is_open ())
    return;

  for (size_t i = 0; i < data.size (); ++i)
    {
      m_file << data[i];
      if (i < data.size () - 1)
        {
          m_file << ",";
        }
    }
  m_file << "\n";
}

} // namespace ns3
