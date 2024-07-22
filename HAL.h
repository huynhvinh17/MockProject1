#ifndef _HAL_H_
#define _HAL_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define DEFAULT_SECTOR_SIZE 512

/**  Define an enumeration to represent status codes */
typedef enum
{
    KMC_ERROR = -1, /**  Status code indicating an error */
    KMC_OK = 0      /**  Status code indicating success */
} kmc_status_t;     /**  Define the type name for the enumeration */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/**
 * @brief Function to initialize the image file for reading
 *
 * @param imagePath The path to the image file to be opened.
 * @return int Returns 0 if the file is successfully opened, or -1 if there is an error.
 */
int kmc_init(const char *imagePath);

/**
 * @brief Updates the sector size used by the system.
 *
 * @param sectorSize The new sector size to be set. Must be a non-zero value.
 * @return int Returns 0 if the sector size is successfully updated, or -1 if the provided sector size is zero.
 */
int kmc_update_sector_size(uint16_t sectorSize);

/**
 * @brief Reads data from a specified sector in the system.
 *
 * @param index The index of the sector to read from
 * @param buff Pointer to a buffer where the read data will be stored
 * @return int32_t return the number of bytes read on success, or a negative value to indicate an error:
 */
int32_t kmc_read_sector(uint32_t index, uint8_t *buff);

/**
 * @brief Reads data from multiple consecutive sectors starting from a specified index.
 *
 * @param index The starting index of the first sector to read from.
 * @param num The number of consecutive sectors to read.
 * @param buff Pointer to a buffer where the read data will be stored.
 * @return int32_t return the number of bytes read on success, or a negative value to indicate an error:
 */
int32_t kmc_read_multi_sector(uint32_t index, uint32_t num, uint8_t *buff);

/**
 * @brief Function to deinitialize the image file
 */
void kmc_deinit(void);

#endif /** _HAL_H_ */
