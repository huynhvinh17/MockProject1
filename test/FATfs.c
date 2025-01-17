#include "FATfs.h"
#include "HAL.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE 512
#define MAX_PATH_LEN 256

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
        printf("Bytes per sector: %d\n", s_FAT12Info.bytes_per_sector); /** Print FAT12 information (for debugging) */
        printf("Reserved sectors: %d\n", s_FAT12Info.reserved_sectors);
        printf("FAT count: %d\n", s_FAT12Info.fat_count);
        printf("FAT size: %d\n", s_FAT12Info.fat_size_16);
        printf("FAT root_entry_count: %d\n", s_FAT12Info.root_entry_count);
        printf("FAT size: %d\n", s_FAT12Info.sectors_per_cluster);

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
            {
                printf("\nReading FAT table: %d sectors\n", s_FAT12Info.fat_size_16); /** Read FAT table */
                if (kmc_read_multi_sector(s_FAT12Info.reserved_sectors, s_FAT12Info.fat_size_16, s_fat_table) != (s_FAT12Info.fat_size_16 * s_FAT12Info.bytes_per_sector))
                {
                    fprintf(stderr, "Error: Failed to read FAT table\n");
                    free(s_fat_table);
                    result = -1;
                }
                else
                {
                    printf("FAT table read successfully\n");
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

void get_full_path(const char *base_path, const char *entry_name, char *full_path)
{
    if (strcmp(base_path, ".") == 0)
    {
        snprintf(full_path, MAX_PATH_LEN, "/%s", entry_name);
    }
    else
    {
        snprintf(full_path, MAX_PATH_LEN, "%s/%s", base_path, entry_name);
    }
}

char current_directory[MAX_PATH_LEN] = "/"; // Biến toàn cục lưu đường dẫn thư mục hiện tại

/* void fatfs_list_directory(const char *path)
{
    uint8_t sector[SECTOR_SIZE];
    uint32_t root_dir_sectors = ((s_FAT12Info.root_entry_count * 32) + (SECTOR_SIZE - 1)) / SECTOR_SIZE;
    uint32_t root_dir_start = s_FAT12Info.reserved_sectors + (s_FAT12Info.fat_count * s_FAT12Info.fat_size_16);
    uint32_t i = 0;
    uint32_t j = 0;
    int k = 0;
    int entry_index = 1;

    fatfs_entry_t *head = NULL;
    fatfs_entry_t *current_entry = NULL;

    for (i = 0; i < root_dir_sectors; i++)
    {
        if (kmc_read_sector(root_dir_start + i, sector) != SECTOR_SIZE)
        {
            fprintf(stderr, "Failed to read root directory sector\n");
            return;
        }

        for (j = 0; j < SECTOR_SIZE / sizeof(fatfs_dir_entry_t); j++)
        {
            fatfs_dir_entry_t *entry = (fatfs_dir_entry_t *)(sector + j * sizeof(fatfs_dir_entry_t));

            if (entry->name[0] == 0x00)
            {
                break;
            }

            if ((entry->name[0] != 0xE5) && ((entry->attr & 0x0F) != 0x0F))
            {
                char name[12];
                memcpy(name, entry->name, 11);
                name[11] = '\0';

                for (k = 0; k < 11; k++)
                {
                    if (name[k] == ' ')
                    {
                        name[k] = '\0';
                        break;
                    }
                }

                fatfs_entry_t *new_entry = (fatfs_entry_t *)malloc(sizeof(fatfs_entry_t));
                if (new_entry == NULL)
                {
                    fprintf(stderr, "Error: Memory allocation failed\n");
                    return;
                }
                strncpy(new_entry->name, name, 12);
                new_entry->attributes = entry->attr;
                new_entry->first_cluster = entry->first_cluster_low;
                new_entry->next = NULL;

                if (head == NULL)
                {
                    head = new_entry;
                    current_entry = head;
                }
                else
                {
                    current_entry->next = new_entry;
                    current_entry = new_entry;
                }

                printf("%d. %s\t", entry_index++, new_entry->name);
                if (new_entry->attributes & 0x10)
                {
                    printf("(Directory)\n");
                }
                else
                {
                    printf("(File)\n");
                }
            }
        }
    }

    printf("\nPlease enter selection number or '..' to go back: ");
    char choice[256];
    fflush(stdin);
    fgets(choice, sizeof(choice), stdin);

    choice[strcspn(choice, "\n")] = '\0';

    if (strlen(choice) == 0)
    {
        fprintf(stderr, "Invalid selection\n");
        return;
    }

    int selection = atoi(choice);
    if (selection > 0 && selection < entry_index)
    {
        int idx = 0;
        // Lấy entry thứ selection
        fatfs_entry_t *selected_entry = head;
        for (idx = 1; idx < selection; idx++)
        {
            selected_entry = selected_entry->next;
        }

        if (selected_entry->attributes & 0x10)
        {
            // Nếu là thư mục, mở thư mục con
            char new_path[MAX_PATH_LEN];
            snprintf(new_path, MAX_PATH_LEN, "%s/%s", current_directory, selected_entry->name);
            strncpy(current_directory, new_path, MAX_PATH_LEN);
            fatfs_list_directory(new_path);
        }
        else
        {
            // Nếu là file, hiển thị nội dung file
            fatfs_display_file(selected_entry->name);
        }
    }
    else if (strncmp(choice, "..", sizeof("..")) == 0)
    {
        // Điều hướng về thư mục cha
        if (strcmp(current_directory, "/") == 0)
        {
            fatfs_list_directory("/");
        }
        else
        {
            char *last_slash = strrchr(current_directory, '/');
            if (last_slash != NULL)
            {
                *last_slash = '\0'; // Xóa phần thư mục con cuối cùng
                if (strcmp(current_directory, "") == 0)
                {
                    strcpy(current_directory, "/"); // Nếu bị xóa hết, trở lại root
                }
            }
            fatfs_list_directory(current_directory);
        }
    }
    else
    {
        fprintf(stderr, "Invalid selection\n");
    }

    current_entry = head;
    while (current_entry != NULL)
    {
        fatfs_entry_t *temp = current_entry;
        current_entry = current_entry->next;
        free(temp);
    }
}

void fatfs_display_file(const char *filepath)
{
    uint8_t sector[SECTOR_SIZE];
    uint32_t root_dir_sectors = ((s_FAT12Info.root_entry_count * 32) + (SECTOR_SIZE - 1)) / SECTOR_SIZE;
    uint32_t root_dir_start = s_FAT12Info.reserved_sectors + (s_FAT12Info.fat_count * s_FAT12Info.fat_size_16);
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t l = 0;
    int k = 0;
    int file_found = 0;

    for (i = 0; i < root_dir_sectors; i++)
    {
        if (kmc_read_sector(root_dir_start + i, sector) != SECTOR_SIZE)
        {
            fprintf(stderr, "Failed to read root directory sector\n");
            return;
        }

        for (j = 0; j < SECTOR_SIZE / sizeof(fatfs_dir_entry_t); j++)
        {
            fatfs_dir_entry_t *entry = (fatfs_dir_entry_t *)(sector + j * sizeof(fatfs_dir_entry_t));

            if (entry->name[0] == 0x00)
            {
                return;
            }

            char name[12];
            memcpy(name, entry->name, 11);
            name[11] = '\0';

            for (k = 0; k < 11; k++)
            {
                if (name[k] == ' ')
                {
                    name[k] = '\0';
                    break;
                }
            }

            if (strcmp(name, filepath) == 0)
            {
                file_found = 1;
                uint32_t cluster = entry->first_cluster_low;
                uint32_t size = entry->file_size;
                uint8_t file_data[SECTOR_SIZE];

                while (size > 0)
                {
                    uint32_t sector_num = root_dir_start + (cluster - 2) * s_FAT12Info.sectors_per_cluster;

                    if (kmc_read_sector(sector_num, file_data) != SECTOR_SIZE)
                    {
                        fprintf(stderr, "Failed to read file sector\n");
                        return;
                    }

                    for (l = 0; l < SECTOR_SIZE && size > 0; l++, size--)
                    {
                        putchar(file_data[l]);
                    }

                    uint16_t fat_offset = cluster * 3 / 2;
                    uint16_t fat_entry = 0;
                    if (cluster & 0x0001)
                    {
                        fat_entry = (*(uint16_t *)&s_fat_table[fat_offset]) >> 4;
                    }
                    else
                    {
                        fat_entry = (*(uint16_t *)&s_fat_table[fat_offset]) & 0x0FFF;
                    }

                    cluster = fat_entry;
                    if (cluster >= 0xFF8)
                    {
                        break;
                    }
                }

                return;
            }
        }
    }

    if (!file_found)
    {
        fprintf(stderr, "File not found\n");
    }
}
 */
