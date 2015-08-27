//------------------------------------------------------------------------------
// MIT License, Copyright (c) 2013 Johannes Walter <johannes@wltr.io>
//
// Description:
// SPI Flash memory programmer using FTDI chips.
//------------------------------------------------------------------------------

#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <cstring>

#include "ftdi_spi_mem.hpp"

ftdi_spi_mem::ftdi_spi_mem() :
  _status(FT_INVALID_HANDLE),
  _handle(0),
  _spi_buf(0),
  _spi_len(0)
{
  // Allocate memory for SPI buffer
  _spi_buf = new uint8[SPI_BUF_LEN];

  // Initialize library
  Init_libMPSSE();
}

ftdi_spi_mem::~ftdi_spi_mem()
{
  // Close channel
  close_channel();

  // Clean-up library
  Cleanup_libMPSSE();

  // Free memory for SPI buffer
  delete[] _spi_buf;
  _spi_buf = 0;
}

ftdi_spi_mem::ftdi_spi_mem(const ftdi_spi_mem& f)
{}

uint32 ftdi_spi_mem::get_memory_size()
{
  return MEM_SIZE;
}

uint32 ftdi_spi_mem::get_channel_count()
{
  uint32 channel_cnt = 0;

  // Get number of channels
  _status = SPI_GetNumChannels(&channel_cnt);
  if(_status == FT_OK) {
    return channel_cnt;
  } else {
    std::cout << "Error: Could not get channel count." << std::endl;
    return 0;
  }
}

void ftdi_spi_mem::print_channel_info(const uint32 channel_num)
{
  FT_DEVICE_LIST_INFO_NODE info;

  // Get channel info
  _status = SPI_GetChannelInfo(channel_num, &info);
  if(_status == FT_OK) {
    std::cout << "Channel " << channel_num << ":" << std::endl;
    std::cout << "  Description: " << info.Description << std::endl;
    std::cout << "  Flags: " << info.Flags << std::endl;
    std::cout << "  ftHandle: " << info.ftHandle << std::endl;
    std::cout << "  ID: " << info.ID << std::endl;
    std::cout << "  LocId: " << info.LocId << std::endl;
    std::cout << "  SerialNumber: " << info.SerialNumber << std::endl;
    std::cout << "  Type: " << info.Type << std::endl;
  } else {
    std::cout << "Error: Could not get info for channel " << channel_num << "." << std::endl;
  }
}

bool ftdi_spi_mem::open_channel(const uint32 channel_num)
{
  // Open channel
  _status = SPI_OpenChannel(channel_num, &_handle);
  if(_status != FT_OK) {
    std::cout << "Error: Could not open channel." << std::endl;
    return false;
  }

  // Set channel parameters
  ChannelConfig config;
  config.ClockRate = SPI_CLOCK_RATE;
  config.LatencyTimer = SPI_LATENCY_TIMER;
  config.configOptions = SPI_CONFIG_OPTION_MODE0 | SPI_CONFIG_OPTION_CS_DBUS3 | SPI_CONFIG_OPTION_CS_ACTIVELOW;
  config.Pin = 0x3B | (0x2C << 8) | (0x00 << 16) | (0xFF << 24);

  // Initialize channel
  if(_handle == 0) {
    std::cout << "Error: Invalid handle." << std::endl;
    return false;
  }

  _status = SPI_InitChannel(_handle, &config);
  if(_status != FT_OK) {
    std::cout << "Error: Could not initialize channel." << std::endl;
    return false;
  }

  return true;
}

bool ftdi_spi_mem::close_channel()
{
  if(_handle == 0) {
    return true;
  }

  // Close channel
  _status = SPI_CloseChannel(_handle);
  if(_status != FT_OK) {
    std::cout << "Error: Could not close channel." << std::endl;
    return false;
  }

  _handle = 0;
  return true;
}

bool ftdi_spi_mem::memory_enable_write()
{
  if(_handle == 0) {
    std::cout << "Error: Invalid handle." << std::endl;
    return false;
  }

  // Send command to set the memory's write enable flag
  _spi_buf[0] = 0x06;
  _status = SPI_Write(_handle, _spi_buf, 1, &_spi_len, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
  if(_status != FT_OK) {
    std::cout << "Error: Could not enable write flag." << std::endl;
    return false;
  }

  return true;
}

bool ftdi_spi_mem::memory_disable_write()
{
  if(_handle == 0) {
    std::cout << "Error: Invalid handle." << std::endl;
    return false;
  }

  // Send command to unset the memory's write enable flag
  _spi_buf[0] = 0x04;
  _status = SPI_Write(_handle, _spi_buf, 1, &_spi_len, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
  if(_status != FT_OK) {
    std::cout << "Error: Could not disable write flag." << std::endl;
    return false;
  }

  return true;
}

void ftdi_spi_mem::print_memory_id()
{
  if(_handle == 0) {
    std::cout << "Error: Invalid handle." << std::endl;
    return;
  }

  // Send command to read the memory's ID
  _spi_buf[0] = 0x9F;
  _status = SPI_Write(_handle, _spi_buf, 1, &_spi_len, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
  if(_status != FT_OK) {
    std::cout << "Error: Could not read ID." << std::endl;
    return;
  }

  memset(_spi_buf, 0, SPI_BUF_LEN);

  // Read the memory's ID
  _status = SPI_Read(_handle, _spi_buf, MEM_ID_LEN, &_spi_len, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
  if(_status == FT_OK) {
    std::cout << "Memory ID:";
    for(size_t i = 0; i < _spi_len; ++i) {
      std::cout << " 0x" << std::hex << (int)_spi_buf[i] << std::dec;
    }
    std::cout << std::endl;
  } else {
    std::cout << "Error: Could not read ID." << std::endl;
  }
}

void ftdi_spi_mem::print_memory_status()
{
  if(_handle == 0) {
    std::cout << "Error: Invalid handle." << std::endl;
    return;
  }

  uint8 status = 0;
  if(!get_memory_status(status)) {
    return;
  }

  std::cout << "Memory status: 0x" << std::hex << (int)status << std::dec << std::endl;
}

bool ftdi_spi_mem::get_memory_status(uint8& status)
{
  if(_handle == 0) {
    std::cout << "Error: Invalid handle." << std::endl;
    return false;
  }

  // Send command to read the memory's status
  _spi_buf[0] = 0x05;
  _status = SPI_Write(_handle, _spi_buf, 1, &_spi_len, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
  if(_status != FT_OK) {
    std::cout << "Error: Could not send read status command." << std::endl;
    return false;
  }

  // Read the memory's status
  _status = SPI_Read(_handle, &status, 1, &_spi_len, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
  if(_status != FT_OK) {
    std::cout << "Error: Could not read status." << std::endl;
    return false;
  }

  return true;
}

bool ftdi_spi_mem::memory_bulk_erase()
{
  // Channel has to be open
  if(_handle == 0) {
    std::cout << "Error: Invalid handle." << std::endl;
    return false;
  }

  // Check if memory is ready
  uint8 status = 0;
  if(!get_memory_status(status)) {
    return false;
  }

  if(status & 0x03) {
    std::cout << "Error: Device is busy." << std::endl;
    return false;
  }

  // Set memory's write enable flag
  if(!memory_enable_write()) {
    return false;
  }

  // Check if memory's write enable flag is set
  if(!get_memory_status(status)) {
    return false;
  }

  if(status & 0x02 == 0) {
    std::cout << "Error: Write enable flag not set." << std::endl;
    return false;
  }

  char progr[8] = {'\\', '-', '/', '|', '\\', '-', '/', '|'};
  size_t progr_idx = 0;

  // TODO: Start time counter

  // Send command to bulk erase the memory
  _spi_buf[0] = 0xC7;
  _status = SPI_Write(_handle, _spi_buf, 1, &_spi_len, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
  if(_status != FT_OK) {
    std::cout << "Error: Could not send bulk erase command." << std::endl;
    return false;
  }

  std::cout << "\r";
  std::cout << "Erasing " << progr[progr_idx];
  progr_idx = ++progr_idx % 8;

  usleep(WAIT_BE_MS * 1000);

  // Wait for operation to finish
  bool wait = true;
  do {
    if(!get_memory_status(status)) {
      return false;
    }

    if(status & 0x03) {
      std::cout << "\r";
      std::cout << "Erasing " << progr[progr_idx];
      progr_idx = ++progr_idx % 8;

      usleep(WAIT_BE_MS * 1000);
    } else {
      wait = false;
    }
  } while(wait);

  // TODO: Stop time counter

  std::cout << "\r";
  std::cout << "Erasing done." << std::endl;

  return true;
}

uint8 ftdi_spi_mem::reverse_bits(uint8 value)
{
  // Reverse all bits of a single byte
  uint8 r = value;
  int s = sizeof(value) * 8 - 1;
  for(value >>= 1; value; value >>= 1) {
    r <<= 1;
    r |= value & 1;
    s--;
  }
  r <<= s;
  return r;
}

bool ftdi_spi_mem::memory_write(uint8* values, const uint32 size, bool bit_swap)
{
  // Check size
  if(size > MEM_SIZE) {
    std::cout << "Error: Memory size exceeded." << std::endl;
    return false;
  }

  // Channel has to be open
  if(_handle == 0) {
    std::cout << "Error: Invalid handle." << std::endl;
    return false;
  }

  // Check if memory is ready
  uint8 status = 0;
  if(!get_memory_status(status)) {
    return false;
  }

  if(status & 0x03) {
    std::cout << "Error: Device is busy." << std::endl;
    return false;
  }

  uint32 addr = 0;
  uint32 bytes_left = size;
  uint32 buf_idx = 0;
  int last = 999;

  // TODO: Start time counter

  while(bytes_left > 0) {
    // Set memory's write enable flag
    if(!memory_enable_write()) {
      return false;
    }

    // Check if memory's write enable flag is set
    if(!get_memory_status(status)) {
      return false;
    }

    if(status & 0x02 == 0) {
      std::cout << "Error: Write enable flag not set." << std::endl;
      return false;
    }

    memset(_spi_buf, 0, SPI_BUF_LEN);

    // Set up the write command
    _spi_buf[0] = 0x02;
    _spi_buf[1] = (addr >> 16) & 0xFF;
    _spi_buf[2] = (addr >> 8) & 0xFF;
    _spi_buf[3] = addr & 0xFF;

    // Calculate packet size
    uint32 packet_len = SPI_PACKET_LEN;
    if(bytes_left < packet_len) {
      packet_len = bytes_left;
    }

    memcpy(&_spi_buf[4], &values[buf_idx], packet_len);

    // Bit swapping
    if(bit_swap) {
      for(size_t i = 4; i < SPI_BUF_LEN; ++i) {
        _spi_buf[i] = reverse_bits(_spi_buf[i]);
      }
    }

    // Send command to write data
    _status = SPI_Write(_handle, _spi_buf, (packet_len + 4), &_spi_len, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    if(_status != FT_OK) {
      std::cout << "Error: Could not send page program command." << std::endl;
      return false;
    }

    if((packet_len + 4) != _spi_len) {
      std::cout << "Error: Data length mismatch." << std::endl;
      return false;
    }

    int percent = (100.0 / size * (size - bytes_left));
    if(percent != last) {
      last = percent;
      std::cout << "\r";
      std::cout << "Writing bytes: " << percent << " %";
    }

    usleep(WAIT_PP_MS * 1000);

    // Wait for operation to finish - not necessary
    bool wait = true;
    do {
      if(!get_memory_status(status)) {
        return false;
      }

      if(status & 0x03) {
        usleep(WAIT_PP_MS * 1000);
      } else {
        wait = false;
      }
    } while(wait);

    addr += packet_len;
    buf_idx += packet_len;
    bytes_left -= packet_len;
  }

  // TODO: Stop time counter

  std::cout << "\r";
  std::cout << "Writing done." << std::endl;

  return true;
}

bool ftdi_spi_mem::memory_read(uint8* values, const uint32 size, bool bit_swap)
{
  // Check size
  if(size > MEM_SIZE) {
    std::cout << "Error: Memory size exceeded." << std::endl;
    return false;
  }

  // Channel has to be open
  if(_handle == 0) {
    std::cout << "Error: Invalid handle." << std::endl;
    return false;
  }

  // Check if memory is ready
  uint8 status = 0;
  if(!get_memory_status(status)) {
    return false;
  }

  if(status & 0x03) {
    std::cout << "Error: Device is busy." << std::endl;
    return false;
  }

  uint32 addr = 0;
  uint32 bytes_left = size;
  uint32 buf_idx = 0;
  int last = 999;

  // TODO: Start time counter

  while(bytes_left > 0) {
    // Set up the read command
    _spi_buf[0] = 0x03;
    _spi_buf[1] = (addr >> 16) & 0xFF;
    _spi_buf[2] = (addr >> 8) & 0xFF;
    _spi_buf[3] = addr & 0xFF;

    // Send command to read data
    _status = SPI_Write(_handle, _spi_buf, 4, &_spi_len, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE);
    if(_status != FT_OK) {
      std::cout << "Error: Could not send read command." << std::endl;
      return false;
    }

    // Calculate packet size
    uint32 packet_len = SPI_PACKET_LEN;
    if(bytes_left < packet_len) {
      packet_len = bytes_left;
    }

    memset(_spi_buf, 0, SPI_BUF_LEN);

    // Read data
    _status = SPI_Read(_handle, _spi_buf, packet_len, &_spi_len, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    if(_status != FT_OK) {
      std::cout << "Error: Could not read data." << std::endl;
      return false;
    }

    if(packet_len != _spi_len) {
      std::cout << "Error: Data length mismatch." << std::endl;
      return false;
    }

    // Bit swapping
    if(bit_swap) {
      for(size_t i = 0; i < packet_len; ++i) {
        _spi_buf[i] = reverse_bits(_spi_buf[i]);
      }
    }

    memcpy(&values[buf_idx], &_spi_buf[0], packet_len);

    addr += packet_len;
    buf_idx += packet_len;
    bytes_left -= packet_len;

    int percent = (100.0 / size * (size - bytes_left));
    if(percent != last) {
      last = percent;
      std::cout << "\r";
      std::cout << "Reading bytes: " << percent << " %";
    }
  }

  // TODO: Stop time counter

  std::cout << "\r";
  std::cout << "Reading done." << std::endl;

  return true;
}

bool ftdi_spi_mem::memory_is_empty()
{
  uint8* buf = new uint8[MEM_SIZE];
  memset(buf, 0x00, MEM_SIZE);

  // Read the whole memory
  if(!memory_read(buf, MEM_SIZE)) {
    return false;
  }

  // Check if memory is empty
  for(size_t i = 0; i < MEM_SIZE; ++i) {
    if(buf[i] != 0xFF) {
      return false;
    }
  }

  delete[] buf;
  buf = 0;

  return true;
}

bool ftdi_spi_mem::gpio_read(uint8& value)
{
  // Channel has to be open
  if(_handle == 0) {
    std::cout << "Error: Invalid handle." << std::endl;
    return false;
  }

  // The FT2232D on the ECI has only 4 GPIOs
  uint8 mask = 0x0F;

  // Set GPIO directions to inputs
  uint8 direction = 0x00;

  // Write GPIO directions
  _status = FT_WriteGPIO(_handle, (direction & mask), 0x00);
  if(_status != FT_OK) {
    std::cout << "Error: Could not write GPIO directions." << std::endl;
    return false;
  }

  // Read GPIOs
  _status = FT_ReadGPIO(_handle, &value);
  if(_status != FT_OK) {
    std::cout << "Error: Could not read GPIOs." << std::endl;
    return false;
  }

  value = (value & mask);

  return true;
}

bool ftdi_spi_mem::gpio_write(uint8 value)
{
  // Channel has to be open
  if(_handle == 0) {
    std::cout << "Error: Invalid handle." << std::endl;
    return false;
  }

  // The FT2232D on the ECI has only 4 GPIOs
  uint8 mask = 0x0F;

  // Set GPIO directions to outputs
  uint8 direction = 0xFF;

  // Write GPIOs
  _status = FT_WriteGPIO(_handle, (direction & mask), (value & mask));
  if(_status != FT_OK) {
    std::cout << "Error: Could not write GPIOs." << std::endl;
    return false;
  }

  return true;
}
