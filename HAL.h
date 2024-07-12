#ifndef HAL_H
#define HAL_H

#include <stdint.h>

#define SECTOR_SIZE 512  // Define the sector size

// Reads a sector at the given index into the buffer.
// Returns the number of bytes read, or -1 on error.
int32_t kmc_read_sector(uint32_t index, uint8_t *buff);

// Reads multiple sectors starting from the given index into the buffer.
// Returns the number of bytes read, or -1 on error.
int32_t kmc_read_multi_sector(uint32_t index, uint32_t num, uint8_t *buff);

#endif // HAL_H
