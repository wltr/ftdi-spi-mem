//------------------------------------------------------------------------------
// MIT License, Copyright (c) 2013 Johannes Walter <johannes@wltr.io>
//
// Description:
// SPI Flash memory programmer using FTDI chips.
//------------------------------------------------------------------------------

#ifndef FTDI_SPI_MEM_HPP
#define FTDI_SPI_MEM_HPP

#include <ftd2xx.h>
#include <libMPSSE_spi.h>

class ftdi_spi_mem {
public:
  ftdi_spi_mem();
  virtual ~ftdi_spi_mem();

  // USB channel functions
  uint32 get_channel_count();
  void print_channel_info(const uint32 channel_num);
  bool open_channel(const uint32 channel_num);
  bool close_channel();

  // Memory functions
  void print_memory_id();
  void print_memory_status();
  uint32 get_memory_size();
  bool get_memory_status(uint8& status);
  bool memory_is_empty();
  bool memory_bulk_erase();
  bool memory_read(uint8* values, const uint32 size, bool bit_swap = false);
  bool memory_write(uint8* values, const uint32 size, bool bit_swap = false);

  // GPIO functions
  bool gpio_read(uint8& value);
  bool gpio_write(uint8 value);

  // Helper functions
  uint8 reverse_bits(uint8 value);

protected:
  ftdi_spi_mem(const ftdi_spi_mem& f);

private:
  // Memory functions
  bool memory_enable_write();
  bool memory_disable_write();

  // USB status
  FT_STATUS _status;
  FT_HANDLE _handle;

  // USB buffer
  uint8* _spi_buf;
  uint32 _spi_len;

  // Constants
  static const size_t SPI_CLOCK_RATE = 10000000;
  static const size_t SPI_LATENCY_TIMER = 16;
  static const size_t SPI_BUF_LEN = 260;
  static const size_t SPI_PACKET_LEN = 256;
  static const size_t MEM_ID_LEN = 3;
  static const size_t MEM_SIZE = 4194304;
  static const size_t WAIT_PP_MS = 1;
  static const size_t WAIT_BE_MS = 500;
};

#endif
