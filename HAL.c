#include "HAL.h"
#include <stdio.h>
#include <stdlib.h>

static FILE *s_imageFile = NULL;
static uint16_t s_sectorSize = SECTOR_SIZE;

int hal_init(const char *imagePath)
{
    int status = 0;

    s_imageFile = fopen(imagePath, "rb");
    if (!s_imageFile)
    {
        fprintf(stderr, "Error: Failed to open image file\n");
        status = -1;
    }

    return status;
}

void hal_cleanup(void)
{
    if (s_imageFile != NULL)
    {
        fclose(s_imageFile);
        s_imageFile = NULL;
    }
}

int kmc_update_sector_size(uint16_t sectorSize)
{
    int status = 0;
    if (sectorSize == 0)
    {
        status = -1;
    }
    s_sectorSize = sectorSize;

    return status;
}

int32_t kmc_read_sector(uint32_t index, uint8_t *buff)
{
    int status = 0;

    if (fseek(s_imageFile, index * s_sectorSize, SEEK_SET) != 0)
    {
        status = -1;
    }
    else
    {
        status = fread(buff, 1, s_sectorSize, s_imageFile);
    }
    return status;
}

int32_t kmc_read_multi_sector(uint32_t index, uint32_t num, uint8_t *buff)
{
    int status = 0;

    if (fseek(s_imageFile, index * s_sectorSize, SEEK_SET) != 0)
    {
        status = -1;
    }
    else
    {
        status = fread(buff, 1, s_sectorSize * num, s_imageFile);
    }

    return status;
}

