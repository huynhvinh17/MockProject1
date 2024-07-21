/*******************************************************************************
 * Definitions
 ******************************************************************************/

#include "HAL.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/

static FILE *s_imageFile = NULL;            /** File pointer to the image file */
static uint16_t s_sectorSize = DEFAULT_SECTOR_SIZE; /** Initializedto DEFAULT_SECTOR_SIZE */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

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

void kmc_deinit(void)
{
    if (s_imageFile != NULL) /** Check if the file is open */
    {
        fclose(s_imageFile); /** Close the file */
        s_imageFile = NULL;  /** Set the file pointer to NULL */
    }
}

int kmc_update_sector_size(uint16_t sectorSize)
{
    kmc_status_t status = KMC_OK; /** Initialize the status to KMC_OK, indicating success */

    if ((sectorSize == 0) || (sectorSize % 512 != 0)) /** Check if the provided sector size is 0 or mutiple of 512 */
    {
        status = KMC_ERROR; /** Set status to KMC_ERROR to indicate the error */
    }
    s_sectorSize = sectorSize; /** Update the sector size */

    return status; /** Return the status */
}

int32_t kmc_read_sector(uint32_t index, uint8_t *buff)
{
    int byteRead = 0; /** ByteRead variable to return the number of bytes read or failure */

    if (fseek(s_imageFile, index * s_sectorSize, SEEK_SET) != 0) /** Move file pointer to the desire sector */
    {
        byteRead = -1; /** Set byteRead to indicate failure */
    }
    else
    {
        byteRead = fread(buff, 1, s_sectorSize, s_imageFile); /** Read sector into the buffer */
    }

    return byteRead; /** Return the byteRead */
}

int32_t kmc_read_multi_sector(uint32_t index, uint32_t num, uint8_t *buff)
{
    int byteRead = 0; /** ByteRead variable to return the number of bytes read or failure */

    if (fseek(s_imageFile, index * s_sectorSize, SEEK_SET) != 0) /** Move file pointer to the desire sector */
    {
        byteRead = -1;
    }
    else
    {
        byteRead = fread(buff, 1, s_sectorSize * num, s_imageFile); /** Read mutiple sector into the buffer */
    }

    return byteRead; /** Return the byteRead */
}
