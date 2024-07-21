/*******************************************************************************
 * Definitions
 ******************************************************************************/

#ifndef _FATFS_H_
#define _FATFS_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

#pragma pack(push, 1)
/**
 * @brief  Define the structure for the FAT filesystem boot sector
 */
typedef struct
{
    uint8_t jump_code[3];        /** Jump code */
    char oem_name[8];            /** OEM Name */
    uint16_t bytes_per_sector;   /** Bytes per sector */
    uint8_t sectors_per_cluster; /** Sectors per cluster */
    uint16_t reserved_sectors;   /** Number of reserved sectors */
    uint8_t fat_count;           /** Number of FATs */
    uint16_t root_entry_count;   /** Number of root directory entries */
    uint16_t total_sectors_16;   /** Total number of sectors (if zero, use total_sectors_32) */
    uint8_t media_descriptor;    /** Media descriptor */
    uint16_t fat_size_16;        /** Sectors per FAT */
    uint16_t sectors_per_track;  /** Sectors per track */
    uint16_t number_of_heads;    /** Number of heads */
    uint32_t hidden_sectors;     /** Number of hidden sectors */
    uint32_t total_sectors_32;   /** Total number of sectors (if total_sectors_16 is zero) */
    uint8_t drive_number;        /** Drive number */
    uint8_t reserved1;           /** Reserved */
    uint8_t boot_signature;      /** Boot signature */
    uint32_t volume_id;          /** Volume ID */
    char volume_label[11];       /** Volume label */
    char filesystem_type[8];     /** Filesystem type */
} fatfs_bootsector_struct_t;
#pragma pack(pop)

/**
 * @brief Define the structure for a directory entry in the FAT filesystem
 */
typedef struct
{
    uint8_t name[11];            /** Name of the file or directory */
    uint8_t attr;                /** Attribute byte */
    uint8_t nt_reserved;         /** Reserved for NT */
    uint8_t create_time_tenth;   /** Creation time in tenths of a second */
    uint16_t create_time;        /** Creation time */
    uint16_t create_date;        /** Creation date */
    uint16_t last_access_date;   /** Last access date */
    uint16_t first_cluster_high; /** High word of first cluster number */
    uint16_t write_time;         /** Last write time */
    uint16_t write_date;         /** Last write date */
    uint16_t first_cluster_low;  /** Low word of first cluster number */
    uint32_t file_size;          /** Size of the file */
} fatfs_dir_entry_t;

/**
 * @brief  Define the structure for a directory entry in the linked list
 */
typedef struct DirEntry
{
    char name[12];          /** 11 characters for name + null terminator */
    uint32_t size;          /** Size of the file */
    int is_dir;             /** Flag to indicate if entry is a directory */
    uint32_t first_cluster; /** First cluster number of the file/directory */
    struct DirEntry *next;  /** Pointer to the next directory entry in the lists */
} DirEntry;

/**
 * @brief  Define the structure for a stack of directories (for navigation)
 */
typedef struct DirectoryStack
{
    uint32_t cluster;            /** Cluster number */
    struct DirectoryStack *next; /** Pointer to the next item in the stack */
} DirectoryStack;

/**
 * @brief Initialize the filesystem and release resources
 *
 * @param image_path path of the file to init
 * @return int int Status code indicating success (0) or failure (-1)
 */
int fatfs_init(const char *image_path);

/**
 * @brief Deinitialize the filesystem and release resources
 *
 * This function closes the image file and cleans up any allocated resources.
 */
void fatfs_deinit(void);

/**
 * @brief Update the sector size for filesystem operations
 *
 * @param sectorSize New sector size to set
 * @return int Status code indicating success (0) or failure (-1)
 */
int kmc_update_sector_size(uint16_t sectorSize);

/**
 * @brief Display the directory entries
 *
 * @param head Pointer to the head of the linked list of directory entries
 */
void display_entries(DirEntry *head);

/**
 * @brief Get a directory entry by its index
 *
 * @param head Pointer to the head of the linked list of directory entries
 * @param index Index of the entry to retrieve
 * @return DirEntry* Pointer to the directory entry at the specified index
 */
DirEntry *get_entry_by_index(DirEntry *head, int index);

/**
 * @brief Free the memory allocated for directory entries
 *
 * @param head Pointer to the head of the linked list of directory entries
 */
void free_entries(DirEntry *head);

/**
 * @brief Read the contents of a directory starting from a specific cluster
 *
 * @param start_cluster Cluster number where the directory starts
 * @param head Pointer to a pointer to the head of the linked list of directory entries (will be updated)
 */
void fatfs_read_dir(uint32_t start_cluster, DirEntry **head);

/**
 * @brief Read a file from the filesystem
 *
 * @param filepath Path of the file to read
 * @param start_cluster Cluster number where the file starts
 */
void fatfs_read_file(const char *filepath, uint32_t start_cluster);

#endif  /** _FATFS_H_ */
