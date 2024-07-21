/*******************************************************************************
 * Definitions
 ******************************************************************************/

#include "FATfs.h"
#include "HAL.h"

#define DEFAULT_SECTOR_SIZE 512 /** Define size of sector by 512 byte */

/*******************************************************************************
 * Variables
 ******************************************************************************/

static fatfs_bootsector_struct_t s_FAT12Info; /** FAT12 boot sector information */
static uint8_t *s_fat_table = NULL;           /** Pointer to the FAT table */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/**
 * @brief Push a new cluster onto the directory stack
 *
 * @param stack Pointer to the pointer of the stack
 * @param cluster Cluster number to push onto the stack
 */
void push(DirectoryStack **stack, uint32_t cluster)
{
    /** Allocate memory for a new stack node */
    DirectoryStack *newNode = (DirectoryStack *)malloc(sizeof(DirectoryStack));
    newNode->cluster = cluster;
    newNode->next = *stack; /** Pointer to the previous top of the stack */
    *stack = newNode;       /** Update the stack to point to the new node */
}

/**
 * @brief Pop a cluster from the directory stack
 *
 * @param stack Pointer to the pointer of the stack
 * @return uint32_t Cluster number that was removed from the stack
 */
uint32_t pop(DirectoryStack **stack)
{
    uint32_t cluster = 0; /** Initialize cluster to a default value */

    if (*stack != NULL)
    {
        DirectoryStack *temp = *stack;
        cluster = temp->cluster; /** Store the cluster value */
        *stack = (*stack)->next; /** Move the stack pointer to the next node */
        free(temp);              /** Free the memory of the old top node */
    }

    return cluster; /** Return the cluster value, or 0 if the stack was empty */
}

int fatfs_init(const char *image_path)
{
    int result = FAT_OK;                  /** Variable to store the result of initialization */
    uint8_t bootSector[DEFAULT_SECTOR_SIZE]; /** Buffer to hold the boot sector data */

    /** Initialize the layer with the image path */
    if (kmc_init(image_path) != 0)
    {
        fprintf(stderr, "Failed to open image file\n");
        result = FAT_ERROR; /** Indicate failure to open the image file */
    }
    /** Read the boot sector from the image */
    else if (kmc_read_sector(0, bootSector) != DEFAULT_SECTOR_SIZE)
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
                    free(s_fat_table); /** Free the allocated memory on failure */
                    result = FAT_ERROR;       /** Indicate failure to read the FAT table */
                }
            }
        }
    }

    return result; /** Return the result of initialization */
}

void fatfs_deinit(void)
{
    if (s_fat_table)
    {
        free(s_fat_table);  /** Free the allocated memory for the FAT table */
        s_fat_table = NULL; /** Set the pointer to NULL */
    }
    kmc_deinit();
}

void display_entries(DirEntry *head)
{
    DirEntry *current = head; /** Pointer to the list */
    int index = 0;            /** Index for displaying entries */

    printf("Index   Name            Size    Type\n");
    while (current)
    {
        printf("%-7d %-15s %-7u %s\n", index++, current->name, current->size, current->is_dir ? "DIR" : "FILE");
        current = current->next; /** Move to the next entry */
    }
}

DirEntry *get_entry_by_index(DirEntry *head, int index)
{
    int count = 0;            /** Counter for the list */
    DirEntry *current = head; /** Pointer to the list */
    DirEntry *result = NULL;  /** Pointer to store the result */
    int found = 0;            /** Flag to indicate if entry is found */

    while ((current) && (!found))
    {
        if (count == index)
        {
            result = current; /** Store the result */
            found = 1;        /** Set the flag to indicate the entry is found */
        }
        count++;
        current = current->next;
    }

    return result; /** Return the result (NULL if not found) */
}

void free_entries(DirEntry *head)
{
    DirEntry *current = head; /** Pointer to the list */
    while (current)
    {
        DirEntry *next = current->next; /** Store the next entry */
        free(current);                  /** Free the current entry */
        current = next;                 /** Move to the next entry */
    }
}

void fatfs_read_dir(uint32_t start_cluster, DirEntry **head)
{
    uint8_t sector[DEFAULT_SECTOR_SIZE]; /** Buffer to hold sector data */
    uint32_t root_dir_sector = s_FAT12Info.reserved_sectors + (s_FAT12Info.fat_count * s_FAT12Info.fat_size_16);
    uint32_t root_dir_size = (s_FAT12Info.root_entry_count * sizeof(fatfs_dir_entry_t)) / DEFAULT_SECTOR_SIZE;
    uint32_t i, j;
    bool varReturn = true;

    /** Read root directory if start cluster is 0 */
    if (start_cluster == 0)
    {
        for (i = 0; (i < root_dir_size) && (varReturn); i++)
        {
            /** Read each sector of the root directory */
            if (kmc_read_sector(root_dir_sector + i, sector) != DEFAULT_SECTOR_SIZE)
            {
                fprintf(stderr, "Error: Failed to read root directory sector %d\n", i);
                varReturn = false;
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
                    else if (dir[j].name[0] == 0xE5 || ((dir[j].attr & 0x0F) == 0x0F))
                    {
                        /** do noting */
                    }
                    else
                    {
                        DirEntry *entry = (DirEntry *)malloc(sizeof(DirEntry)); /** Allocate and initialize a new DirEntry */
                        memcpy(entry->name, dir[j].name, 11);                   /** Copy the name */
                        entry->name[11] = '\0';                                 /** Null-terminate the name */
                        entry->size = dir[j].file_size;                         /** File size */
                        entry->is_dir = (dir[j].attr & 0x10) != 0;              /** Check if it's a directory */
                        entry->first_cluster = dir[j].first_cluster_low;        /** First cluster of the file or directory */
                        entry->next = NULL;                                     /** Initialize next pointer to NULL */

                        /** Add the new entry to the linked list */
                        if (*head == NULL)
                        {
                            *head = entry;
                        }
                        else
                        {
                            DirEntry *tail = *head;
                            while (tail->next != NULL)
                            {
                                tail = tail->next;
                            }
                            tail->next = entry;
                        }
                    }
                }
            }
        }
    }
    else
    {
        uint32_t cluster = start_cluster; /** Start reading from the given cluster */
        uint16_t fat_entry = 0;
        uint8_t high_byte = 0;
        uint8_t low_byte = 0;

        while (cluster < 0xFF8)
        {
            /** Calculate the starting sector of the current cluster */
            uint32_t cluster_sector = cluster + 31;

            /** Read each sector of the current cluster */
            if (kmc_read_sector(cluster_sector, sector) != DEFAULT_SECTOR_SIZE)
            {
                fprintf(stderr, "Error: Failed to read sector %d of subdirectory\n", cluster_sector);
            }
            else
            {
                varReturn = true;
                /** Pointer to directory entries in the sector */
                fatfs_dir_entry_t *dir = (fatfs_dir_entry_t *)sector;
                for (j = 0; j < (DEFAULT_SECTOR_SIZE / sizeof(fatfs_dir_entry_t)) && (varReturn); ++j)
                {
                    /** Check if the directory entry is empty */
                    if (dir[j].name[0] == 0x00)
                    {
                        /**do nothing */
                        varReturn = false;
                    }
                    else if (dir[j].name[0] == 0xE5 || ((dir[j].attr & 0x0F) == 0x0F))
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
                        entry->next = NULL;                                     /** Initialize next pointer to NULL */

                        /** Add the new entry to the linked list */
                        if (*head == NULL)
                        {
                            *head = entry; /** If the linked list is empty, assign the new entry to head*/
                        }
                        else
                        {
                            DirEntry *tail = *head; /** If the linked list is not empty, find the last element of the list */
                            while (tail->next != NULL)
                            {
                                tail = tail->next; /** Move the tail pointer to the next element */
                            }
                            tail->next = entry; /** Assign the new entry to the next pointer of the last element */
                        }
                    }
                }
            }

            fat_entry = (cluster * 3) / 2;        /** Find the byte offset */
            high_byte = s_fat_table[fat_entry];    /** Read the high byte from FAT */
            low_byte = s_fat_table[fat_entry + 1]; /** Read the low byte from FAT */

            /** Check if the cluster number is even or odd */
            if (cluster % 2 == 0)
            { /** Taking the lower 4 bits of the hige byte and combining them with the low byte*/
                cluster = (high_byte & 0x0F) << 8 | low_byte;
            }
            else
            { /** Taking the upper 4 bits of highbyte, and combining them with the low byte*/
                cluster = (high_byte >> 4) | ((low_byte & 0xFF) << 8);
            }
        }
    }
}

void fatfs_read_file(const char *filepath, uint32_t start_cluster)
{
    uint32_t cluster = start_cluster; /** Current cluster being read */
    uint8_t sector[DEFAULT_SECTOR_SIZE];      /** Buffer to hold data read from a sector */
    bool readSuccess = true;          /** Flag to track read success */
    uint16_t fat_entry = 0;
    uint8_t high_byte = 0;
    uint8_t low_byte = 0;

    while ((cluster < 0xFF8) && (readSuccess))
    {
        /** Calculate the sector number for the current cluster. */
        uint32_t cluster_sector = cluster + 31;

        if (kmc_read_sector(cluster_sector, sector) != DEFAULT_SECTOR_SIZE)
        {
            fprintf(stderr, "Error: Failed to read sector %d of file\n", cluster_sector);
            readSuccess = false; /** Set flag to false if reading fails */
        }
        else
        {
            fwrite(sector, 1, DEFAULT_SECTOR_SIZE, stdout); /** Output the sector data to stdout. */
        }

        /** Caculate the position in the FAT table */
        if (readSuccess)
        {
            fat_entry = (cluster * 3) / 2;         /** Find the byte offset */
            high_byte = s_fat_table[fat_entry];    /** Read the high byte from FAT */
            low_byte = s_fat_table[fat_entry + 1]; /** Read the low byte from FAT */

            /** Check if the cluster number is even or odd */
            if (cluster % 2 == 0)
            { /** Taking the lower 4 bits of the hige byte and combining them with the low byte*/
                cluster = (high_byte & 0x0F) << 8 | low_byte;
            }
            else
            { /** Taking the upper 4 bits of highbyte, and combining them with the low byte*/
                cluster = (high_byte >> 4) | ((low_byte & 0xFF) << 8);
            }
        }
    }
}
