#ifndef FATFS_H
#define FATFS_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t jump_code[3];            /** Jump code */
    char oem_name[8];                /** OEM Name */
    uint16_t bytes_per_sector;       /** Bytes per sector */
    uint8_t sectors_per_cluster;     /** Sectors per cluster */
    uint16_t reserved_sectors;       /** Number of reserved sectors */
    uint8_t fat_count;               /** Number of FATs */
    uint16_t root_entry_count;       /** Number of root directory entries */
    uint16_t total_sectors_16;       /** Total number of sectors (if zero, use total_sectors_32) */
    uint8_t media_descriptor;        /** Media descriptor */
    uint16_t fat_size_16;            /** Sectors per FAT */
    uint16_t sectors_per_track;      /** Sectors per track */
    uint16_t number_of_heads;        /** Number of heads */
    uint32_t hidden_sectors;         /** Number of hidden sectors */
    uint32_t total_sectors_32;       /** Total number of sectors (if total_sectors_16 is zero) */
    uint8_t drive_number;            /** Drive number */
    uint8_t reserved1;               /** Reserved */
    uint8_t boot_signature;          /** Boot signature */
    uint32_t volume_id;              /** Volume ID */
    char volume_label[11];           /** Volume label */
    char filesystem_type[8];         /** Filesystem type */
} fatfs_bootsector_struct_t;
#pragma pack(pop)

typedef struct
{
    uint8_t name[11];
    uint8_t attr;
    uint8_t nt_reserved;
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} fatfs_dir_entry_t;

typedef struct fatfs_entry {
    char name[12];
    uint8_t attributes;
    uint32_t first_cluster;
    struct fatfs_entry *next;
} fatfs_entry_t;

int fatfs_init(const char *image_path);
void fatfs_deinit(void);
int kmc_update_sector_size(uint16_t sectorSize);
void fatfs_list_directory(const char *path);
void fatfs_display_file(const char *filepath);

#endif

