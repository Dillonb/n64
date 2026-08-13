#include "memory_logger.h"

#include <vector>

bool should_log = false;

std::vector<memory_access_t> byte_reads;
std::vector<memory_access_t> byte_writes;
std::vector<memory_access_t> half_reads;
std::vector<memory_access_t> half_writes;
std::vector<memory_access_t> word_reads;
std::vector<memory_access_t> word_writes;
std::vector<memory_access_t> dword_reads;
std::vector<memory_access_t> dword_writes;

void init_memory_logger() {
  clear_memory_logger();
  should_log = true;
}

void log_memory_read(u32 paddr, memory_access_size_t size, u64 value) {
  if (should_log) {
    switch (size) {
    case MEMORY_ACCESS_SIZE_BYTE: {
      memory_access_t access = {paddr, BUS_LOAD, size, value & 0xFF};
      byte_reads.push_back(access);
      break;
    }
    case MEMORY_ACCESS_SIZE_HALF: {
      memory_access_t access = {paddr, BUS_LOAD, size, value & 0xFFFF};
      half_reads.push_back(access);
      break;
    }
    case MEMORY_ACCESS_SIZE_WORD: {
      memory_access_t access = {paddr, BUS_LOAD, size, value & 0xFFFFFFFF};
      word_reads.push_back(access);
      break;
    }
    case MEMORY_ACCESS_SIZE_DWORD: {
      memory_access_t access = {paddr, BUS_LOAD, size, value};
      dword_reads.push_back(access);
      break;
    }
    }
  }
}

void log_memory_write(u32 paddr, memory_access_size_t size, u64 value) {
  if (should_log) {
    switch (size) {
      case MEMORY_ACCESS_SIZE_BYTE: {
        memory_access_t access = {paddr, BUS_STORE, size, value & 0xFFFFFFFF};
        byte_writes.push_back(access);
        break;
      }
      case MEMORY_ACCESS_SIZE_HALF: {
        memory_access_t access = {paddr, BUS_STORE, size, value & 0xFFFFFFFF};
        half_writes.push_back(access);
        break;
      }
      case MEMORY_ACCESS_SIZE_WORD: {
        memory_access_t access = {paddr, BUS_STORE, size, value & 0xFFFFFFFF};
        word_writes.push_back(access);
        break;
      }
      case MEMORY_ACCESS_SIZE_DWORD: {
        memory_access_t access = {paddr, BUS_STORE, size, value};
        dword_writes.push_back(access);
        break;
      }
    }
  }
}

std::vector<memory_access_t>& get_memory_access_vector(memory_access_size_t size, bus_access_t access_type) {
  switch (access_type) {
    case BUS_LOAD:
      switch (size) {
        case MEMORY_ACCESS_SIZE_BYTE:
          return byte_reads;
        case MEMORY_ACCESS_SIZE_HALF:
          return half_reads;
        case MEMORY_ACCESS_SIZE_WORD:
          return word_reads;
        case MEMORY_ACCESS_SIZE_DWORD:
          return dword_reads;
      }
      break;
    case BUS_STORE:
      switch (size) {
        case MEMORY_ACCESS_SIZE_BYTE:
          return byte_writes;
        case MEMORY_ACCESS_SIZE_HALF:
          return half_writes;
        case MEMORY_ACCESS_SIZE_WORD:
          return word_writes;
        case MEMORY_ACCESS_SIZE_DWORD:
          return dword_writes;
      }
      break;
  }
  logfatal("Invalid memory access size (%d) or type (%d) in get_memory_access_vector", size, access_type);
}

bool pop_memory_access(memory_access_t *access, memory_access_size_t size, bus_access_t access_type) {
  std::vector<memory_access_t>& access_vector = get_memory_access_vector(size, access_type);
  if (access_vector.empty()) {
    return false;
  } else {
    *access = access_vector.front();
    access_vector.erase(access_vector.begin());
    return true;
  }
}

void clear_memory_logger() {
  byte_reads.clear();
  byte_writes.clear();
  half_reads.clear();
  half_writes.clear();
  word_reads.clear();
  word_writes.clear();
  dword_reads.clear();
  dword_writes.clear();
}
