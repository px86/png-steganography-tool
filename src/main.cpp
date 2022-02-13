#include "include/pngs.hpp"
#include "include/crc.hpp"
#include "include/argparser.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
  const char *png_filepath = nullptr;
  const char *secret_filepath = nullptr;
  const char *out_filepath = nullptr;
  const char *chunk_type = nullptr;

  auto ap = pr::ArgParser("pngs");
  ap.add_option(chunk_type, "chunk type to be extracted", "extract", 'x');
  ap.add_option(secret_filepath, "path of file which is to be hidden", "secret", 's');

  ap.add_argument(png_filepath, "PNG file path", "<pngfile>");
  ap.add_argument(out_filepath, "output file path", "<outfile>");

  ap.parse(argc, argv);

  if (!png_filepath) {
    std::cerr << "Error: PNG file path not provided" << std::endl;
    exit(EXIT_FAILURE);
  }

  // Extract data
  if (chunk_type) {
    if (!out_filepath) {
      std::cerr << "Error: Output file path not provided" << std::endl;
      exit(EXIT_FAILURE);
    }

    auto pngfile = std::ifstream(png_filepath, std::ifstream::binary);
    verify_png_signature(pngfile, png_filepath);

    extract_chunk_type(chunk_type, pngfile, out_filepath);
  }
  // Hide data
  else if (secret_filepath) {
    if (!out_filepath) {
      std::cerr << "Error: Output file path not provided" << std::endl;
      exit(EXIT_FAILURE);
    }

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
  }
  // Inspect pngfile
  else {
    auto pngfile = std::ifstream(png_filepath, std::ifstream::binary);
    verify_png_signature(pngfile, png_filepath);
    inspect_pngfile(pngfile, png_filepath);
  }

  return EXIT_SUCCESS;
}
