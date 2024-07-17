#include "FATfs.h"
#include "HAL.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE 512

static fatfs_bootsector_struct_t s_FAT12Info;
static uint8_t *s_fat_table = NULL;

int fatfs_init(const char *image_path)
{
    int result = 0;
    uint8_t bootSector[SECTOR_SIZE];

    if (hal_init(image_path) != 0)
    {
        fprintf(stderr, "Failed to open image file\n");
        result = -1;
    }
    else if (kmc_read_sector(0, bootSector) != SECTOR_SIZE) /** Read boot sector */
    {
        fprintf(stderr, "Error: Failed to read boot sector\n");
        result = -1;
    }
    else
    {
        memcpy(&s_FAT12Info, bootSector, sizeof(s_FAT12Info));          /** Copy FAT12 information from boot sector */
        printf("Bytes per sector: %d\n", s_FAT12Info.bytes_per_sector);
        printf("Reserved sectors: %d\n", s_FAT12Info.reserved_sectors);
        printf("FAT count: %d\n", s_FAT12Info.fat_count);
        printf("FAT size: %d\n", s_FAT12Info.fat_size_16);

        if (kmc_update_sector_size(s_FAT12Info.bytes_per_sector) != 0) /** Update sector size */
        {
            fprintf(stderr, "Error: Failed to update sector size\n");
            result = -1;
        }
        else
        {
            s_fat_table = malloc(s_FAT12Info.fat_size_16 * s_FAT12Info.bytes_per_sector); /** Allocate memory for FAT table */
            if (!s_fat_table)
            {
                fprintf(stderr, "Error: Failed to allocate memory for FAT table\n");
                result = -1;
            }
            else
                printf("\nReading FAT table: %d sectors\n", s_FAT12Info.fat_size_16); /** Read FAT table */
                printf("FAT table read successfully\n");
        }
    }

    return result;
}

void fatfs_deinit(void)
{
    if (s_fat_table)
    {
        free(s_fat_table);
        s_fat_table = NULL;
    }
}

void fatfs_list_directory(const char *path)
{
}

void fatfs_display_file(const char *filepath)
{
}
