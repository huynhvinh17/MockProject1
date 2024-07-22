/*******************************************************************************
 * Definitions
 ******************************************************************************/

#include "HAL.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/

static FILE *s_imageFile = NULL;                    /** File pointer to the image file */
static uint16_t s_sectorSize = DEFAULT_SECTOR_SIZE; /** Initializedto DEFAULT_SECTOR_SIZE */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/**
 * @brief Function to initialize the image file for reading
 *
 * @param imagePath The path to the image file to be opened.
 * @return int Returns 0 if the file is successfully opened, or -1 if there is an error.
 */
int kmc_init(const char *imagePath)
{
    kmc_status_t status = KMC_OK; /** Initialize status to KMC_OK, indicate success */

    s_imageFile = fopen(imagePath, "rb"); /** Open the image file in read */
    if (!s_imageFile)                     /** Check if the file failed to open */
    {
        fprintf(stderr, "Error: Failed to open image file\n"); /** Print error message */
        status = KMC_ERROR;                                    /** Set status to indicate failure */
    }

    return status; /** Return the status */
}

/**
 * @brief Updates the sector size used by the system.
 *
 * @param sectorSize The new sector size to be set. Must be a non-zero value.
 * @return int Returns 0 if the sector size is successfully updated, or -1 if the provided sector size is zero.
 */
int kmc_update_sector_size(uint16_t sectorSize)
{
    kmc_status_t status = KMC_OK; /** Initialize the status to KMC_OK, indicating success */

    if ((0 == sectorSize) || (sectorSize % 512 != 0)) /** Check if the provided sector size is 0 or mutiple of 512 */
    {
        status = KMC_ERROR; /** Set status to KMC_ERROR to indicate the error */
    }
    s_sectorSize = sectorSize; /** Update the sector size */

    return status; /** Return the status */
}

/**
 * @brief Reads data from a specified sector in the system.
 *
 * @param index The index of the sector to read from
 * @param buff Pointer to a buffer where the read data will be stored
 * @return int32_t return the number of bytes read on success, or a negative value to indicate an error:
 */
int32_t kmc_read_sector(uint32_t index, uint8_t *buff)
{
    int byteRead = (int)KMC_OK; /** ByteRead variable to return the number of bytes read or failure */

    if (fseek(s_imageFile, index * s_sectorSize, SEEK_SET) != 0) /** Move file pointer to the desire sector */
    {
        byteRead = KMC_ERROR; /** Set byteRead to indicate failure */
    }
    else
    {
        byteRead = fread(buff, 1, s_sectorSize, s_imageFile); /** Read sector into the buffer */
    }
    return byteRead; /** Return the byteRead */
}

/**
 * @brief Reads data from multiple consecutive sectors starting from a specified index.
 *
 * @param index The starting index of the first sector to read from.
 * @param num The number of consecutive sectors to read.
 * @param buff Pointer to a buffer where the read data will be stored.
 * @return int32_t return the number of bytes read on success, or a negative value to indicate an error:
 */
int32_t kmc_read_multi_sector(uint32_t index, uint32_t num, uint8_t *buff)
{
    int byteRead = (int)KMC_OK; /** ByteRead variable to return the number of bytes read or failure */

    if (fseek(s_imageFile, index * s_sectorSize, SEEK_SET) != 0) /** Move file pointer to the desire sector */
    {
        byteRead = KMC_ERROR;
    }
    else
    {
        byteRead = fread(buff, 1, s_sectorSize * num, s_imageFile); /** Read mutiple sector into the buffer */
    }

    return byteRead; /** Return the byteRead */
}

/**
 * @brief Function to deinitialize the image file
 */
void kmc_deinit(void)
{
    if (s_imageFile != NULL) /** Check if the file is open */
    {
        fclose(s_imageFile); /** Close the file */
        s_imageFile = NULL;  /** Set the file pointer to NULL */
    }
}
