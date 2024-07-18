#include "FATfs.h"
#include "HAL.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE 512

static fatfs_bootsector_struct_t s_FAT12Info;
static uint8_t *s_fat_table = NULL;


void push(DirectoryStack **stack, uint32_t cluster)
{
    DirectoryStack *newNode = (DirectoryStack *)malloc(sizeof(DirectoryStack));
    newNode->cluster = cluster;
    newNode->next = *stack;
    *stack = newNode;
}

uint32_t pop(DirectoryStack **stack)
{
    if (*stack == NULL)
        return 0;

    DirectoryStack *temp = *stack;
    uint32_t cluster = temp->cluster;
    *stack = (*stack)->next;
    free(temp);
    return cluster;
}

int fatfs_init(const char *image_path)
{
    int result = 0;
    uint8_t bootSector[SECTOR_SIZE];

    if (kmc_init(image_path) != 0)
    {
        fprintf(stderr, "Failed to open image file\n");
        result = -1;
    }
    else if (kmc_read_sector(0, bootSector) != SECTOR_SIZE)
    {
        fprintf(stderr, "Error: Failed to read boot sector\n");
        result = -1;
    }
    else
    {
        memcpy(&s_FAT12Info, bootSector, sizeof(s_FAT12Info));
        if (kmc_update_sector_size(s_FAT12Info.bytes_per_sector) != 0)
        {
            fprintf(stderr, "Error: Failed to update sector size\n");
            result = -1;
        }
        else
        {
            s_fat_table = malloc(s_FAT12Info.fat_size_16 * s_FAT12Info.bytes_per_sector);
            if (!s_fat_table)
            {
                fprintf(stderr, "Error: Failed to allocate memory for FAT table\n");
                result = -1;
            }
            else
            {
                if (kmc_read_multi_sector(s_FAT12Info.reserved_sectors, s_FAT12Info.fat_size_16, s_fat_table) != (s_FAT12Info.fat_size_16 * s_FAT12Info.bytes_per_sector))
                {
                    fprintf(stderr, "Error: Failed to read FAT table\n");
                    free(s_fat_table);
                    result = -1;
                }
            }
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

void display_entries(DirEntry *head)
{
    DirEntry *current = head;
    int index = 0;

    printf("Index   Name            Size    Type\n");
    while (current)
    {
        printf("%-7d %-15s %-7u %s\n", index++, current->name, current->size, current->is_dir ? "DIR" : "FILE");
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

void fatfs_read_dir(uint32_t start_cluster, DirEntry **head, DirEntry **tail)
{
    uint8_t sector[SECTOR_SIZE];
    uint32_t root_dir_sector = s_FAT12Info.reserved_sectors + (s_FAT12Info.fat_count * s_FAT12Info.fat_size_16);
    uint32_t root_dir_size = (s_FAT12Info.root_entry_count * sizeof(fatfs_dir_entry_t) + (SECTOR_SIZE - 1)) / SECTOR_SIZE;
    uint32_t i, j;

    if (start_cluster == 0)
    {
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
                    return;
                if (dir[j].name[0] == 0xE5 || (dir[j].attr & 0x08))
                    continue; // Skip deleted or volume label

                DirEntry *entry = (DirEntry *)malloc(sizeof(DirEntry));
                memcpy(entry->name, dir[j].name, 11);
                entry->name[11] = '\0';
                entry->size = dir[j].file_size;
                entry->is_dir = (dir[j].attr & 0x10) != 0;
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
                        return;
                    if (dir[j].name[0] == 0xE5 || (dir[j].attr & 0x08))
                        continue; // Skip deleted or volume label

                    DirEntry *entry = (DirEntry *)malloc(sizeof(DirEntry));
                    memcpy(entry->name, dir[j].name, 11);
                    entry->name[11] = '\0';
                    entry->size = dir[j].file_size;
                    entry->is_dir = (dir[j].attr & 0x10) != 0;
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
