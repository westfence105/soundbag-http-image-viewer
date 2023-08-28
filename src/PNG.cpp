#include <iostream>
#include <cstring>

#include "PNG.hpp"

PNG::Chunk::Chunk(std::ifstream& ifs, std::list<const char*> target_chunks) {
  unsigned char size_buf[4];
  ifs.read((char*)size_buf, 4);
  m_size = (size_buf[0] << 24) | (size_buf[1] << 16) | (size_buf[2] << 8) | size_buf[3];

  ifs.read(m_name, 4);
  // std::cout << "[Chunk] " << m_name << " (" << m_size << " bytes)" << std::endl;

  if (target_chunks.size()) {
    bool found = false;
    for (const char* name : target_chunks) {
      if (memcmp(m_name, name, 4) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      // Not in target
      ifs.seekg(m_size + 4, std::ios::cur);
      m_data = NULL;
      return;
    }
  }

  if (m_size) {
    m_data = new char[m_size + 1];
    ifs.read(m_data, m_size);
  }

  ifs.read(m_crc, 4);
}

PNG::Chunk::~Chunk() {
  if (m_size && m_data) {
    delete [] m_data;
  }
}

PNG::File::File(std::ifstream& ifs, std::list<const char*> target_chunks) {
  char header_buf[8];
  ifs.read(header_buf, 8);

  while (ifs.peek() != std::ios::traits_type::eof()) {
    auto chunk = new Chunk(ifs, target_chunks);
    if (memcmp(chunk->name(), "IEND", 4) == 0) {
      m_chunks.push_back(chunk);
      break;
    }
    else if (target_chunks.size()) {
      bool found = false;
      for (const auto& name : target_chunks) {
        if (memcmp(chunk->name(), name, 4) == 0) {
          found = true;
          break;
        }
      }
      if (found) {
        m_chunks.push_back(chunk);

        bool remain = false;
        for (const auto& name : target_chunks) {
          if (this->chunk(name) == NULL) {
            remain = true;
            break;
          }
        }
        if (remain) {
          break;
        }
      }
      else {
        delete chunk;
      }
    }
    else {
      m_chunks.push_back(chunk);
    }
  }
}

PNG::File::~File() {
  for (auto& chunk : m_chunks) {
    delete chunk;
  }
}

const char* PNG::File::exif() const {
  for (auto& chunk : m_chunks) {
    if (memcmp(chunk->name(), "eXIf", 4) == 0) {
      return chunk->data();
    }
  }
  return NULL;
}

const PNG::Chunk* PNG::File::chunk(const char* name) const {
  for (auto& chunk : m_chunks) {
    if (memcmp(chunk->name(), name, 4) == 0) {
      return chunk;
    }
  }
  return NULL;
}
