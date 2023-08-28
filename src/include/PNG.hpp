#pragma once

#include <list>
#include <fstream>

namespace PNG {
  class Chunk {
      Chunk();

      unsigned long  m_size;
      char  m_name[4];
      char* m_data;
      char  m_crc[4];

    public:
      Chunk(std::ifstream& ifs, std::list<const char*> target_chunks = {});
      ~Chunk();

      inline long size() const { return m_size; }
      inline const char* name() const { return m_name; }
      inline const char* data() const { return m_data; }
      inline const char* crc()  const { return m_crc; }
  };

  class File {
      File();

      std::list<Chunk*> m_chunks;

    public:
      File(std::ifstream& ifs, std::list<const char*> target_chunks = {});
      ~File();

      const PNG::Chunk* chunk(const char* name) const;
      const char* exif() const;
      inline const std::list<Chunk*> chunks() const { return m_chunks; }
  };
}
