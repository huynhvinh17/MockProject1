
#include "FATfs.h"
#include "HAL.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define DEFAULT_SECTOR_SIZE 512      /** Define size of sector by 512 byte */
#define FIRST_SECTOR_OF_ROOT_DIR 19; /** Define the sector number where the root directory start */
#define ROOT_DIR_SECTOR 14;          /** Define the number of sectors used by the root directory */

/*******************************************************************************
 * Variables
 ******************************************************************************/

static fatfs_bootsector_struct_t s_FAT12Info; /** FAT12 boot sector information */
static uint8_t *s_fat_table = NULL;           /** Pointer to the FAT table */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static int offsetCluster(uint32_t cluster); /** Calculate the offset for a given cluster in the file */

/*******************************************************************************
 * Code
 ******************************************************************************/

/**
 * @brief Initialize the filesystem and release resources
 *
 * @param image_path path of the file to init
 * @return int int Status code indicating success (0) or failure (-1)
 */
int fatfs_init(const char *image_path)
{
    FAT_status_t result = FAT_OK;            /** Variable to store the result of initialization */
    uint8_t bootSector[DEFAULT_SECTOR_SIZE]; /** Buffer to hold the boot sector data */

    /** Initialize the layer with the image path */
    if (kmc_init(image_path) != 0)
    {
        fprintf(stderr, "Failed to open image file\n");
        result = FAT_ERROR; /** Indicate failure to open the image file */
    }
    /** Read the boot sector from the image */
    else if ((kmc_read_sector(0, bootSector)) != DEFAULT_SECTOR_SIZE)
    {
        fprintf(stderr, "Error: Failed to read boot sector\n");
        result = FAT_ERROR; /** Indicate failure to read the boot sector */
    }
    else
    {
        memcpy(&s_FAT12Info, bootSector, sizeof(s_FAT12Info)); /** Copy the boot sector data into the global FAT12 info structure */

        if (kmc_update_sector_size(s_FAT12Info.bytes_per_sector) != 0) /** Update the sector size */
        {
            fprintf(stderr, "Error: Failed to update sector size\n");
            result = FAT_ERROR; /** Indicate failure to update sector size */
        }
        else
        {
            /** Allocate memory for the FAT table */
            s_fat_table = malloc(s_FAT12Info.fat_size_16 * s_FAT12Info.bytes_per_sector);
            if (!s_fat_table)
            {
                fprintf(stderr, "Error: Failed to allocate memory for FAT table\n");
                result = FAT_ERROR; /** Indicate failure to allocate memory */
            }
            else
            {
                /** Read the FAT table into memory */
                if (kmc_read_multi_sector(s_FAT12Info.reserved_sectors, s_FAT12Info.fat_size_16, s_fat_table) != (s_FAT12Info.fat_size_16 * s_FAT12Info.bytes_per_sector))
                {
                    fprintf(stderr, "Error: Failed to read FAT table\n");
                    free(s_fat_table);  /** Free the allocated memory on failure */
                    result = FAT_ERROR; /** Indicate failure to read the FAT table */
                }
            }
        }
    }

    return result; /** Return the result of initialization */
}

/**
 * @brief Get a directory entry by its index
 *
 * @param head Pointer to the head of the linked list of directory entries
 * @param index Index of the entry to retrieve
 * @return DirEntry* Pointer to the directory entry at the specified index
 */
DirEntry *get_index(DirEntry *DirEntryList, int index)
{
    int count = (int)FAT_INDEX;  /** Counter for the list */
    FAT_status_t found = FAT_OK; /** Flag to indicate if entry is found */
    DirEntry *result = NULL;     /** Pointer to store the result */

    while ((DirEntryList) && (found == FAT_OK))
    {
        if (count == index)
        {
            result = DirEntryList; /** Store the result */
            found = FAT_INDEX;     /** Set the flag to indicate the entry is found */
        }
        else
        {
            count++;                           /** Increment the count by 1 */
            DirEntryList = DirEntryList->next; /** Move the next entry in linked list */
        }
    }

    return result; /** Return the result (NULL if not found) */
}

/**
 * @brief Calculate the offset for a given cluster in the file
 *
 * @param cluster Cluster number
 * @return int Return cluster which the offset is to be calculate
 */
static int offsetCluster(uint32_t cluster)
{
    uint16_t fat_entry = (uint16_t)FAT_OK; /** Used to store the FAT entry value, init to FAT_OK status  */
    uint8_t high_byte = (uint8_t)FAT_OK;   /** Used to store the high byte of a FAT entry, init to FAT_OK status */
    uint8_t low_byte = (uint8_t)FAT_OK;    /** Used to store the low byte of a FAT entry, init to FAT_OK status */

    fat_entry = (cluster * 3) / 2;         /** Find the byte offset */
    high_byte = s_fat_table[fat_entry];    /** Read the high byte from FAT */
    low_byte = s_fat_table[fat_entry + 1]; /** Read the low byte from FAT */

    /** Check if the cluster number is even or odd */
    if (0 == (cluster % 2))
    { /** Taking the lower 4 bits of the hige byte and combining them with the low byte*/
        cluster = (high_byte & 0x0F) << 8 | low_byte;
    }
    else
    { /** Taking the upper 4 bits of highbyte, and combining them with the low byte*/
        cluster = (high_byte >> 4) | ((low_byte & 0xFF) << 8);
    }

    return cluster;
}

/**
 * @brief Read the contents of a directory starting from a specific cluster
 *
 * @param start_cluster Cluster number where the directory starts
 * @param head Pointer to a pointer to the head of the linked list of directory entries (will be updated)
 */
void fatfs_read_dir(uint32_t start_cluster, DirEntry **DirEntryList)
{
    uint32_t cluster_physical = 0;                                /** Assign cluster physical to 0 */
    uint8_t sector[DEFAULT_SECTOR_SIZE];                          /** Buffer to hold sector data */
    uint32_t root_dir_sector = ROOT_DIR_SECTOR;                   /** The number of sectors used by the root directory */
    uint32_t first_sector_of_root_dir = FIRST_SECTOR_OF_ROOT_DIR; /** The sector number where the root directory start */

    bool varReturn = true;         /** indicating the success status of operation */
    uint32_t i = (uint32_t)FAT_OK; /** Used as an index of operation */
    uint32_t j = (uint32_t)FAT_OK; /** Used as an index of operation */

    /** Read root directory if start cluster is 0 */
    if (0 == start_cluster)
    {
        for (i = 0; i < root_dir_sector; i++)
        {
            /** Read each sector of the root directory */
            if (kmc_read_multi_sector(first_sector_of_root_dir + i, s_FAT12Info.sectors_per_cluster, sector) != DEFAULT_SECTOR_SIZE)
            {
                fprintf(stderr, "Error: Failed to read root directory sector %d\n", i);
            }
            else
            {
                fatfs_dir_entry_t *dir = (fatfs_dir_entry_t *)sector; /** Pointer to directory entries in the sector */

                for (j = 0; (j < DEFAULT_SECTOR_SIZE / sizeof(fatfs_dir_entry_t)) && (varReturn); ++j)
                {
                    /** Check if the directory entry is empty */
                    if (dir[j].name[0] == 0x00)
                    {
                        varReturn = false;
                    }
                    /** Check if the entry is deleted or a long file name */
                    else if ((dir[j].name[0] == 0xE5) || ((dir[j].attr & 0x0F) == 0x0F))
                    {
                        /** do noting */
                    }
                    else
                    {
                        DirEntry *entry = (DirEntry *)malloc(sizeof(DirEntry)); /** Allocate and initialize a new DirEntry */
                        memcpy(entry->name, dir[j].name, 11);                   /** Copy the name */
                        entry->name[11] = '\0';                                 /** Null-terminate the name */
                        entry->size = dir[j].file_size;                         /** File size */
                        (entry->is_dir = (dir[j].attr & 0x10)) != 0;            /** Check if it's a directory */
                        entry->first_cluster = dir[j].first_cluster_low;        /** First cluster of the file or directory */
                        entry->modified_time = dir[j].write_time;               /** Last write time of the file or directory */
                        entry->modified_date = dir[j].write_date;               /** Last write date of the file or directory */
                        entry->next = NULL;                                     /** Initialize next pointer to NULL */

                        /** Add the new entry to the linked list */
                        if (*DirEntryList == NULL)
                        {
                            *DirEntryList = entry; /** If the head is NULL, assign new entry to head */
                        }
                        else
                        {
                            DirEntry *tail = *DirEntryList; /** If the linked list is not empty, find the last element of the list */
                            while (tail->next != NULL)
                            {
                                tail = tail->next; /** Move the tail pointer to the next element */
                            }
                            tail->next = entry; /** Assign the new entry to the next pointer of the last element */
                        }
                    }
                }
            }
        }
    }
    else
    {
        varReturn = true; /** Set return status to indicate true */

        while (start_cluster < 0xFF8)
        {
            cluster_physical = first_sector_of_root_dir + root_dir_sector + (start_cluster - 2) * s_FAT12Info.sectors_per_cluster; /** The cluster numbering starting */

            /** Read sector of the current cluster */
            if (kmc_read_multi_sector(cluster_physical, s_FAT12Info.sectors_per_cluster, sector) != DEFAULT_SECTOR_SIZE)
            {
                fprintf(stderr, "Error: Failed to read sector %d of subdirectory\n", cluster_physical);
            }
            else
            {

                /** Pointer to directory entries in the sector */
                fatfs_dir_entry_t *dir = (fatfs_dir_entry_t *)sector;

                for (j = 0; j < (DEFAULT_SECTOR_SIZE / sizeof(fatfs_dir_entry_t)) && (varReturn); ++j)
                {
                    /** Check if the directory entry is empty */
                    if (dir[j].name[0] == 0x00)
                    {
                        varReturn = false; /** Set return status to indicate false */
                    }
                    else if ((dir[j].name[0] == 0xE5) || ((dir[j].attr & 0x0F) == 0x0F))
                    {
                        /**do nothing */
                    }
                    else
                    {
                        DirEntry *entry = (DirEntry *)malloc(sizeof(DirEntry)); /** Allocate and initialize a new DirEntry */
                        memcpy(entry->name, dir[j].name, 11);                   /** Copy the name */
                        entry->name[11] = '\0';                                 /** Null-terminate the name */
                        entry->size = dir[j].file_size;                         /** File size */
                        entry->is_dir = (dir[j].attr & 0x10) != 0;              /** Check if it's a directory */
                        entry->first_cluster = dir[j].first_cluster_low;        /** First cluster of the file or directory */
                        entry->modified_time = dir[j].write_time;               /** Last write time of the file or directory */
                        entry->modified_date = dir[j].write_date;               /** Last write time of the file or directory */
                        entry->next = NULL;                                     /** Initialize next pointer to NULL */

                        /** Add the new entry to the linked list */
                        if (*DirEntryList == NULL)
                        {
                            *DirEntryList = entry; /** If the linked list is empty, assign the new entry to head*/
                        }
                        else
                        {
                            DirEntry *tail = *DirEntryList; /** If the linked list is not empty, find the last element of the list */
                            while (tail->next != NULL)
                            {
                                tail = tail->next; /** Move the tail pointer to the next element */
                            }
                            tail->next = entry; /** Assign the new entry to the next pointer of the last element */
                        }
                    }
                }
            }

            start_cluster = offsetCluster(start_cluster); /** Assign start_cluster for return the offsetCluster of start_cluster*/
        }
    }
}

/**
 * @brief Read a file from the filesystem
 *
 * @param filepath Path of the file to read
 * @param start_cluster Cluster number where the file starts
 */
void fatfs_read_file(const char *filepath, uint32_t start_cluster)
{
    uint8_t sector[DEFAULT_SECTOR_SIZE];                          /** Buffer to hold sector data */
    uint32_t root_dir_sector = ROOT_DIR_SECTOR;                   /** The number of sectors used by the root directory */
    uint32_t first_sector_of_root_dir = FIRST_SECTOR_OF_ROOT_DIR; /** The sector number where the root directory start */

    bool readSuccess = true;       /** Flag to track read success */
    uint32_t cluster_physical = 0; /** Assign cluster physical to 0 */

    while ((start_cluster < 0xFF8) && (readSuccess))
    {
        cluster_physical = first_sector_of_root_dir + root_dir_sector + (start_cluster - 2) * s_FAT12Info.sectors_per_cluster; /** The cluster numbering starting */

        /** Read sector of current cluster */
        if (kmc_read_multi_sector(cluster_physical, s_FAT12Info.sectors_per_cluster, sector) != DEFAULT_SECTOR_SIZE)
        {
            fprintf(stderr, "Error: Failed to read sector %d of file\n", cluster_physical);

            readSuccess = false; /** Flag to track read fault */
        }
        else
        {
            fwrite(sector, 1, DEFAULT_SECTOR_SIZE, stdout); /** Output the sector data to stdout. */
            start_cluster = offsetCluster(start_cluster);   /** Assign start_cluster for return the offsetCluster of start_cluster*/
        }
    }
}

/**
 * @brief Free the memory allocated for directory entries
 *
 * @param head Pointer to the head of the linked list of directory entries
 */
void free_entries(DirEntry *DirEntryList)
{
    while (DirEntryList)
    {
        DirEntry *next = DirEntryList->next; /** Store the next entry */
        free(DirEntryList);                  /** Free the DirEntryList entry */
        DirEntryList = next;                 /** Move to the next entry */
    }
}

/**
 * @brief Deinitialize the filesystem and release resources
 *
 * This function closes the image file and cleans up any allocated resources.
 */
void fatfs_deinit(void)
{
    if (s_fat_table)
    {
        free(s_fat_table);  /** Free the allocated memory for the FAT table */
        s_fat_table = NULL; /** Set the pointer to NULL */
    }
    kmc_deinit();
}
