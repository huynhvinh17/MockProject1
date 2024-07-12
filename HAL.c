#include "HAL.h"
#include <stdio.h>
#include <stdlib.h>



// File pointer for the image file
static FILE *image_file = NULL;

// Opens the image file
int hal_init(const char *image_path) {
    image_file = fopen(image_path, "rb");
    return image_file ? 0 : -1;
}

// Closes the image file
void hal_cleanup() {
    if (image_file) {
        fclose(image_file);
        image_file = NULL;
    }
}

int32_t kmc_read_sector(uint32_t index, uint8_t *buff) {
    if (!image_file || !buff) {
        return -1;
    }

    if (fseek(image_file, index * SECTOR_SIZE, SEEK_SET) != 0) {
        return -1;
    }

    return fread(buff, 1, SECTOR_SIZE, image_file);
}

int32_t kmc_read_multi_sector(uint32_t index, uint32_t num, uint8_t *buff) {
    if (!image_file || !buff) {
        return -1;
    }

    if (fseek(image_file, index * SECTOR_SIZE, SEEK_SET) != 0) {
        return -1;
    }

    return fread(buff, 1, num * SECTOR_SIZE, image_file);
}
