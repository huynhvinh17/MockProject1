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

    if (kmc_init(image_path) != 0)
    {
        fprintf(stderr, "Failed to open image file\n");
        return -1;
    }

    if (kmc_read_sector(0, bootSector) != SECTOR_SIZE) /** Read boot sector */
    {
        fprintf(stderr, "Error: Failed to read boot sector\n");
        return -1;
    }

    memcpy(&s_FAT12Info, bootSector, sizeof(s_FAT12Info)); /** Copy FAT12 information from boot sector */

    if (kmc_update_sector_size(s_FAT12Info.bytes_per_sector) != 0) /** Update sector size */
    {
        fprintf(stderr, "Error: Failed to update sector size\n");
        return -1;
    }

    // Calculate FAT table size in sectors
    uint32_t fat_table_start_sector = s_FAT12Info.reserved_sectors;
    uint32_t fat_table_size = s_FAT12Info.fat_size_16 * s_FAT12Info.bytes_per_sector;
    s_fat_table = (uint8_t *)malloc(fat_table_size);
    if (!s_fat_table)
    {
        fprintf(stderr, "Error: Failed to allocate memory for FAT table\n");
        return -1;
    }
	uint32_t i = 0;
    // Read FAT table sectors
    for (i = 0; i < s_FAT12Info.fat_size_16; ++i)
    {
        if (kmc_read_sector(fat_table_start_sector + i, s_fat_table + (i * SECTOR_SIZE)) != SECTOR_SIZE)
        {
            fprintf(stderr, "Error: Failed to read FAT table sector %d\n", fat_table_start_sector + i);
            free(s_fat_table);
            s_fat_table = NULL;
            return -1;
        }
    }

    printf("FAT table read successfully\n");
    return 0;
}

void fatfs_deinit(void)
{
    if (s_fat_table)
    {
        free(s_fat_table);
        s_fat_table = NULL;
    }
    kmc_deinit();
}

void read_dir(uint32_t start_cluster, DirEntry **head, DirEntry **tail)
{
    uint8_t sector[SECTOR_SIZE];
    uint32_t root_dir_sector = s_FAT12Info.reserved_sectors + (s_FAT12Info.fat_count * s_FAT12Info.fat_size_16);
    uint32_t root_dir_size = (s_FAT12Info.root_entry_count * sizeof(fatfs_dir_entry_t) + (SECTOR_SIZE - 1)) / SECTOR_SIZE;
    uint32_t i, j;

    if (start_cluster == 0)
    {
        // Read the root directory
        for (i = 0; i < root_dir_size; i++)
        {
            if (kmc_read_sector(root_dir_sector + i, sector) != SECTOR_SIZE)
            {
                fprintf(stderr, "Error: Failed to read root directory sector %d\n", i);
                return;
            }

            fatfs_dir_entry_t *dir = (fatfs_dir_entry_t *)sector;
            for (j = 0; j < SECTOR_SIZE / sizeof(fatfs_dir_entry_t); ++j)
            {
                if (dir[j].name[0] == 0x00)
                    return; // No more entries
                if (dir[j].name[0] == 0xE5)
                    continue; // Deleted entry

                DirEntry *entry = (DirEntry *)malloc(sizeof(DirEntry));
                memcpy(entry->name, dir[j].name, 11);
                entry->name[11] = '\0'; // Null-terminate the name
                entry->size = dir[j].file_size;
                entry->is_dir = (dir[j].attr & 0x10) != 0; // Check if directory
                entry->first_cluster = dir[j].first_cluster_low;
                entry->next = NULL;

                if (*head == NULL)
                {
                    *head = entry;
                    *tail = entry;
                }
                else
                {
                    (*tail)->next = entry;
                    *tail = entry;
                }
            }
        }
    }
    else
    {
        // Read subdirectory
        uint32_t cluster = start_cluster;
        while (cluster < 0xFF8)
        {
            uint32_t cluster_sector = root_dir_sector + root_dir_size + (cluster - 2) * s_FAT12Info.sectors_per_cluster;
            for (i = 0; i < s_FAT12Info.sectors_per_cluster; ++i)
            {
                if (kmc_read_sector(cluster_sector + i, sector) != SECTOR_SIZE)
                {
                    fprintf(stderr, "Error: Failed to read sector %d of subdirectory\n", cluster_sector + i);
                    return;
                }

                fatfs_dir_entry_t *dir = (fatfs_dir_entry_t *)sector;
                for (j = 0; j < SECTOR_SIZE / sizeof(fatfs_dir_entry_t); ++j)
                {
                    if (dir[j].name[0] == 0x00)
                        return; // No more entries
                    if (dir[j].name[0] == 0xE5)
                        continue; // Deleted entry

                    DirEntry *entry = (DirEntry *)malloc(sizeof(DirEntry));
                    memcpy(entry->name, dir[j].name, 11);
                    entry->name[11] = '\0'; // Null-terminate the name
                    entry->size = dir[j].file_size;
                    entry->is_dir = (dir[j].attr & 0x10) != 0; // Check if directory
                    entry->first_cluster = dir[j].first_cluster_low;
                    entry->next = NULL;

                    if (*head == NULL)
                    {
                        *head = entry;
                        *tail = entry;
                    }
                    else
                    {
                        (*tail)->next = entry;
                        *tail = entry;
                    }
                }
            }

            // Read next cluster from FAT table
            uint16_t fat_entry = (cluster * 3) / 2;
            if (cluster % 2 == 0)
            {
                cluster = *(uint16_t *)&s_fat_table[fat_entry] & 0x0FFF;
            }
            else
            {
                cluster = (*(uint16_t *)&s_fat_table[fat_entry] >> 4) & 0x0FFF;
            }
        }
    }
}

void display_entries(DirEntry *head)
{
    DirEntry *current = head;
    int index = 0;

    printf("Index\tName\t\tSize\tType\n");
    while (current)
    {
        printf("%d\t%s\t\t%u\t%s\n", index++, current->name, current->size, current->is_dir ? "DIR" : "FILE");
        current = current->next;
    }
}

int count_entries(DirEntry *head)
{
    int count = 0;
    DirEntry *current = head;
    while (current)
    {
        count++;
        current = current->next;
    }
    return count;
}

DirEntry *get_entry_by_index(DirEntry *head, int index)
{
    int count = 0;
    DirEntry *current = head;
    while (current)
    {
        if (count == index)
        {
            return current;
        }
        count++;
        current = current->next;
    }
    return NULL;
}

void free_entries(DirEntry *head)
{
    DirEntry *current = head;
    while (current)
    {
        DirEntry *next = current->next;
        free(current);
        current = next;
    }
}

void fatfs_read_file(const char *filepath, uint32_t start_cluster)
{
    uint32_t cluster = start_cluster;
    uint8_t sector[SECTOR_SIZE];
    uint32_t i;

    while (cluster < 0xFF8)
    {
        uint32_t cluster_sector = s_FAT12Info.reserved_sectors + (s_FAT12Info.fat_count * s_FAT12Info.fat_size_16) + s_FAT12Info.root_entry_count / (SECTOR_SIZE / sizeof(fatfs_dir_entry_t)) + (cluster - 2) * s_FAT12Info.sectors_per_cluster;
        for (i = 0; i < s_FAT12Info.sectors_per_cluster; ++i)
        {
            if (kmc_read_sector(cluster_sector + i, sector) != SECTOR_SIZE)
            {
                fprintf(stderr, "Error: Failed to read sector %d of file\n", cluster_sector + i);
                return;
            }

            fwrite(sector, 1, SECTOR_SIZE, stdout);
        }

        // Read next cluster from FAT table
        uint16_t fat_entry = (cluster * 3) / 2;
        if (cluster % 2 == 0)
        {
            cluster = *(uint16_t *)&s_fat_table[fat_entry] & 0x0FFF;
        }
        else
        {
            cluster = (*(uint16_t *)&s_fat_table[fat_entry] >> 4) & 0x0FFF;
        }
    }
}
