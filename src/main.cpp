#include "pngs.hpp"
#include "crc.hpp"

#include <bits/stdint-uintn.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

/*
  pngs -x chunktype pngfile outfile  5
  pngs  pngfile datafile outfile 4
  pngs pngfile 2
 */

int main(int argc, char **argv) {
  char *png_filepath = nullptr;
  char *secret_filepath = nullptr;
  char *out_filepath = nullptr;
  char *chunk_type = nullptr;

  if (argc == 2) {
    png_filepath = argv[1];
    auto pngfile = std::ifstream(png_filepath, std::ifstream::binary);
    verify_png_signature(pngfile, png_filepath);
    inspect_pngfile(pngfile, png_filepath);
    return 0;
  }

  if (argc == 4) {
    png_filepath = argv[1];
    secret_filepath = argv[2];
    out_filepath = argv[3];

    auto pngfile = std::ifstream(png_filepath, std::ifstream::binary);
    verify_png_signature(pngfile, png_filepath);

    auto outfile = std::ofstream(out_filepath, std::ofstream::binary);
    outfile.write((char *)png_signature, 8);

    Chunk secret_chunk = make_chunk_of_file(secret_filepath);

    int chunk_number = 1;

    auto opt_chunk = read_chunk(pngfile);
    while (opt_chunk) {
      write_chunk(outfile, opt_chunk.value());
      chunk_number++;
      // TODO: find a better postion to insert the secret chunk.
      if (chunk_number == 3)
	write_chunk(outfile, secret_chunk);
      opt_chunk = read_chunk(pngfile);
    }

    return 0;
  }

  // Extract secret chunk from the PNG file.
  if (argc == 5 && !strcmp(argv[1], "-x")) {
    chunk_type = argv[2];
    png_filepath = argv[3];
    out_filepath = argv[4];

    auto pngfile = std::ifstream(png_filepath, std::ifstream::binary);
    verify_png_signature(pngfile, png_filepath);

    extract_chunk_type(chunk_type, pngfile, out_filepath);

    return 0;
  }
  else {
    std::cout << R"(Usage:
pngs PNGFILE
pngs -x CHUNKTYPE PNGFILE OUTFILE
pngs PNGFILE SECRETFILE OUTFILE
)";
  }
  return 0;
}
