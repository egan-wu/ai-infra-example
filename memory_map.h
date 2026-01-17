#ifndef MEMORY_MAP_H
#define MEMORY_MAP_H

#include <cstdint>

// Memory Map
const uint64_t RAM_BASE_ADDR = 0x00000000;
const uint64_t RAM_SIZE      = 0x10000000; // 256 MB
const uint64_t TPU_BASE_ADDR = 0x10000000;
const uint64_t TPU_SIZE      = 0x00000100; // 256 Bytes

// Register Offsets
const uint64_t REG_ADDR_SRC_A = 0x00;
const uint64_t REG_ADDR_SRC_B = 0x04;
const uint64_t REG_ADDR_DST_C = 0x08;
const uint64_t REG_MATRIX_DIM = 0x0C;
const uint64_t REG_CMD_START  = 0x10;
const uint64_t REG_STATUS     = 0x14;

// Status Codes
const uint32_t STATUS_IDLE = 0;
const uint32_t STATUS_BUSY = 1;
const uint32_t STATUS_DONE = 2;

// Matrix Config
const int TEST_DIM = 32;

#endif // MEMORY_MAP_H
