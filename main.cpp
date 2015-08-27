//------------------------------------------------------------------------------
// MIT License, Copyright (c) 2013 Johannes Walter <johannes@wltr.io>
//
// Description:
// SPI Flash memory programmer using FTDI chips.
//------------------------------------------------------------------------------

#include <iostream>
#include <string>
#include <fstream>
#include <cstring>

#include "ftdi_spi_mem.hpp"

int main(int argc, char** argv)
{
  if(argc < 2) {
    std::cout << "Usage: " << argv[0] << " FILE" << std::endl;
    return -1;
  }

  std::cout << "Programming..." << std::endl << std::endl;

  std::string filename(argv[1]);

  ftdi_spi_mem dev;
  std::ifstream fls_file;

  // Open input file
  fls_file.open(filename.c_str(), std::ios::binary);

  if(!fls_file.is_open()) {
    std::cout << "Error: Could not open file." << std::endl;
    return -1;
  }

  // Determine file size
  fls_file.seekg(0, std::ios::end);
  size_t fls_buf_len = fls_file.tellg();
  fls_file.seekg(0, std::ios::beg);

  std::cout << "File name: " << filename << std::endl;
  std::cout << "File size: " << fls_buf_len << " bytes" << std::endl;
  std::cout << std::endl;

  // Allocate memory for input file
  char* fls_buf = new char[fls_buf_len];

  // Read input file
  fls_file.read(fls_buf, fls_buf_len);
  fls_file.close();

  // Get channel count
  size_t channel_cnt = dev.get_channel_count();
  if(channel_cnt == 0) {
    std::cout << "Error: No device found." << std::endl;
    return -1;
  }

  // Print info for channel 0
  dev.print_channel_info(0);
  std::cout << std::endl;

  // Open channel 0
  if(!dev.open_channel(0)) {
    return -1;
  }

  // Print memory ID and status
  dev.print_memory_id();
  dev.print_memory_status();
  std::cout << std::endl;

  // Make sure that memory is empty
  if(!dev.memory_bulk_erase()) {
    return -1;
  }

  // Write data to memory
  if(!dev.memory_write((uint8*)fls_buf, fls_buf_len)) {
    return -1;
  }

  // Verify data
  char* buf = new char[fls_buf_len];
  memset(buf, 0, fls_buf_len);

  if(!dev.memory_read((uint8*)buf, fls_buf_len)) {
    return -1;
  }

  if(memcmp(buf, fls_buf, fls_buf_len) == 0) {
    std::cout << std::endl << "SUCCESS" << std::endl;
  } else {
    std::cout << std::endl << "FAILURE" << std::endl;
  }

  // Close channel
  if(!dev.close_channel()) {
    return -1;
  }

  // Free memory for input file
  delete[] fls_buf;
  fls_buf = 0;

  delete[] buf;
  buf = 0;

  std::cout << std::endl;
  std::cout << "Press ENTER to continue ..." << std::endl;
  getchar();

  return 0;
}
