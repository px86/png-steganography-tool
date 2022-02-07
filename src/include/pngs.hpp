#pragma once

#include <cstdint>
#include <utility>  // std::exchange
#include <optional> // std::optional
#include <fstream>
#include <cstring>  // memcmp

typedef unsigned char byte;
constexpr byte png_signature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };

class Chunk;

void verify_png_signature(std::ifstream &file, const char *filepath);
auto read_chunk(std::ifstream &pngfile) -> std::optional<Chunk>;
void write_chunk(std::ofstream&, Chunk&);
void print_chunk(Chunk &chunk, int chunk_num=-1);
void inspect_pngfile(std::ifstream &pngfile, const char *filepath);

Chunk make_chunk_of_file(const char* outfile);
void extract_chunk_type(const char *chunk_type, std::ifstream &pngfile,
                        const char *filepath);

class Chunk {
public:
  uint32_t length = 0;
  uint32_t type = 0;
  uint8_t *data = nullptr;
  uint32_t crc = 0;

  // default constructor
  Chunk() = default;

  // move constructor
  Chunk(Chunk &&other) noexcept
      : length(std::exchange(other.length, 0)),
        type(std::exchange(other.type, 0)),
        data(std::exchange(other.data, nullptr)),
	crc(std::exchange(other.crc, 0)) {}

  ~Chunk() {
    if (data != nullptr) delete [] data;
  }

  // move assignment operator
  Chunk& operator=(Chunk &&other) {
   length = std::exchange(other.length, 0);
   type = std::exchange(other.type, 0);
   data = std::exchange(other.data, nullptr);
   crc = std::exchange(other.crc, 0);
   return *this;
  }
};
