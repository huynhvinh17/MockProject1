#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SECTOR_SIZE 512
#define MAX_PATH_LEN 256

typedef struct {
    char name[11];        // File name (8 characters + 3 extension)
    uint8_t attr;         // File attributes (e.g., read-only, hidden, directory, etc.)
    uint8_t reserved[10]; // Reserved bytes
    uint16_t first_cluster_high; // High 16 bits of the cluster number (for FAT12/16)
    uint16_t first_cluster_low;  // Low 16 bits of the cluster number (for FAT12/16)
    uint32_t file_size;   // File size in bytes
} fatfs_dir_entry_t;

// Structure for FAT directory entry
typedef struct fatfs_entry {
    char name[12];
    uint8_t attributes;
    uint32_t first_cluster;
    struct fatfs_entry *next; // Pointer to the next entry in the linked list
} fatfs_entry_t;

// Function to read a sector from the floppy image
int read_sector(FILE *file, uint32_t sector_num, uint8_t *buffer) {
    fseek(file, sector_num * SECTOR_SIZE, SEEK_SET);
    return fread(buffer, 1, SECTOR_SIZE, file);
}

void fatfs_list_directory(FILE *file, uint32_t start_sector, const char *current_directory) {
    uint8_t sector[SECTOR_SIZE];
    fatfs_entry_t *head = NULL;
    fatfs_entry_t *current_entry = NULL;
    int j =0;
    // Read the sector containing directory entries
    if (read_sector(file, start_sector, sector) != SECTOR_SIZE) {
        fprintf(stderr, "Failed to read directory sector\n");
        return;
    }

    // Iterate through directory entries
    for (j  = 0; j < SECTOR_SIZE / sizeof(fatfs_dir_entry_t); j++) {
        fatfs_dir_entry_t *entry = (fatfs_dir_entry_t *)(sector + j * sizeof(fatfs_dir_entry_t));

        if (entry->name[0] == 0x00) {
            break; // No more entries
        }

        if ((entry->name[0] != 0xE5) && ((entry->attr & 0x0F) != 0x0F)) {
            char name[12];
            memcpy(name, entry->name, 11);
            name[11] = '\0';
            int k = 0;
            // Trim trailing spaces
            for (k = 0; k < 11; k++) {
                if (name[k] == ' ') {
                    name[k] = '\0';
                    break;
                }
            }

            // Allocate memory for new entry
            fatfs_entry_t *new_entry = (fatfs_entry_t *)malloc(sizeof(fatfs_entry_t));
            if (new_entry == NULL) {
                fprintf(stderr, "Error: Memory allocation failed\n");
                return;
            }
            strncpy(new_entry->name, name, 12);
            new_entry->attributes = entry->attr;
            new_entry->first_cluster = entry->first_cluster_low; // Assign the correct cluster number
            new_entry->next = NULL;

            // Insert new entry into linked list
            if (head == NULL) {
                head = new_entry;
                current_entry = head;
            } else {
                current_entry->next = new_entry;
                current_entry = new_entry;
            }
        }
    }

    // Display directory entries and handle user selection (similar to previous code)
    // ...

    // Free allocated memory
    current_entry = head;
    while (current_entry != NULL) {
        fatfs_entry_t *temp = current_entry;
        current_entry = current_entry->next;
        free(temp);
    }
}

int main() {
    FILE *file = fopen("floppy.img", "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open floppy image file.\n");
        return 1;
    }

    // Start reading from the root directory
    uint32_t root_dir_start_sector = 19; // Adjust this according to your floppy image's FAT structure
    char current_directory[MAX_PATH_LEN] = "/"; // Initialize with root directory

    printf("Listing root directory:\n");
    fatfs_list_directory(file, root_dir_start_sector, current_directory);

    fclose(file);
    return 0;
}
