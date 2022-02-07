#include "pngs.hpp"
#include "crc.hpp"

#include <bits/stdint-uintn.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <arpa/inet.h>
#include <iomanip>

void verify_png_signature(std::ifstream &file, const char *filepath) {
  // Note: file postion must be at the beginning.
  file.seekg(0, std::ifstream::beg);
  byte buf[8] = {0};
  file.read((char *)buf, 8);
  if (memcmp(png_signature, buf, 8)) {
    std::cerr << "Error: file " << filepath << " is not a valid PNG file" << std::endl;
    exit(1);
  }
}

auto read_chunk(std::ifstream &pngfile) -> std::optional<Chunk> {
  Chunk chunk;
  pngfile.read((char *)&chunk.length, 4);
  // network byte order to host byte order.
  chunk.length = ntohl(chunk.length);

  pngfile.read((char *)&chunk.type, 4);

  chunk.data = new uint8_t[chunk.length];
  if (chunk.data) {
    pngfile.read((char *)chunk.data, chunk.length);
  } else return {};

  pngfile.read((char *)&chunk.crc, 4);

  if (pngfile.eof() || pngfile.fail()) return {};
  else return chunk;
}

void write_chunk(std::ofstream &outfile, Chunk &chunk) {
  uint32_t nw_length = htonl(chunk.length);

  outfile.write((char *) &nw_length, 4);
  outfile.write((char *) &chunk.type, 4);
  outfile.write((char *) chunk.data, chunk.length);
  outfile.write((char *) &chunk.crc, 4);
}

void print_chunk(Chunk &chunk, int chunk_num) {
  // Convert chunk.type into a C string, for easier printing.
  char type_cstr[5];
  memcpy(type_cstr, &chunk.type, 4);
  type_cstr[4] = '\0';

#define HEX_START "\x1b[33m" "0x" << std::hex << std::setw(8) << std::setfill('0')
#define HEX_END    std::dec << std::setw(0) << "\x1b[m"

  if (chunk_num > 0)
    std::cout << "  Chunk    " << chunk_num << '\n';
  std::cout << "  Type     " << "\x1b[32m" << type_cstr << "\x1b[m" << '\n'
	    << "  Length   " << chunk.length << '\n'
	    << "  CRC      " << HEX_START << chunk.crc << HEX_END << '\n'
	    << "\x1b[30m"  "--------------------------" "\x1b[m" << '\n';

#undef HEX_START
#undef HEX_END
}

void inspect_pngfile(std::ifstream &pngfile, const char *filepath) {

  std::cout << "\nfile: " "\x1b[4m" << filepath << "\x1b[m" "\n\n";

  int chunk_num = 1;
  auto opt_chunk = read_chunk(pngfile);
  while (opt_chunk) {
    print_chunk(opt_chunk.value(), chunk_num++);
    opt_chunk = read_chunk(pngfile);
  }
}

Chunk make_chunk_of_file(const char* filepath)  {
  // Open the file at the END position.
  auto datafile = std::ifstream(filepath, std::ifstream::binary | std::ifstream::ate);
  if (!datafile) {
    std::cerr << "Error: file " << filepath << " can not be opened" << std::endl;
    exit(1);
  }
  size_t filesize = datafile.tellg();
  auto buff = new uint8_t[filesize];
  if (!buff) {
    std::cerr << "Error: can not allocate enough memory for reading file " << filepath << std::endl;
    datafile.close();
    exit(1);
  }

  datafile.seekg(0, std::ifstream::beg);  // move to the beginning
  datafile.read((char *) buff, filesize);

  if (!datafile) {
    std::cerr << "Error: unexpected error while reading file " << filepath << std::endl;
    datafile.close();
    exit(1);
  }

  Chunk chunk;
  chunk.length = filesize;
  memcpy(&chunk.type, "scrt", 4);
  chunk.data = buff;
  chunk.crc = htonl(crc(buff, filesize));

  return chunk;
}

void extract_chunk_type(const char *chunk_type, std::ifstream &pngfile, const char *filepath) {
  pngfile.seekg(8, std::ifstream::beg); // postion to the beginning of first chunk.

  auto file = std::ofstream(filepath, std::ofstream::binary);
  if (!file) {
    std::cerr << "Error: can not open file " << filepath << " for writing." << std::endl;
    exit(1);
  }

  auto opt_chunk = read_chunk(pngfile);
  while (opt_chunk) {
    if (!memcmp(&opt_chunk.value().type, chunk_type, 4)) {
      file.write((char *)opt_chunk.value().data, opt_chunk.value().length);
      break;
    }
    else opt_chunk = read_chunk(pngfile);
  }

  file.close();
}
