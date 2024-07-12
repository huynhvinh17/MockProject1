#include "HAL.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    const char *image_path = "floppy.img";

    if (hal_init(image_path) != 0) {
        fprintf(stderr, "Failed to open image file\n");
        return 1;
    }

    uint8_t buffer[SECTOR_SIZE];
    int32_t bytes_read = kmc_read_sector(0, buffer);

    if (bytes_read != SECTOR_SIZE) {
        fprintf(stderr, "Failed to read sector\n");
    } else {
        printf("Sector read successfully\n");
    }

    uint8_t multi_buffer[SECTOR_SIZE * 4];
    bytes_read = kmc_read_multi_sector(0, 4, multi_buffer);

    if (bytes_read != SECTOR_SIZE * 4) {
        fprintf(stderr, "Failed to read multiple sectors\n");
    } else {
        printf("Multiple sectors read successfully\n");
    }

    hal_cleanup();


    // Mount the image or ensure it is accessible
    // For simplicity, assume floppy.img is a directory
    // If using a real disk image, you would use a library to mount it and access its contents

    char path[256] = ".";
    int choice;
    char filepath[256];

    while (1) {
        printf("\nOptions:\n");
        printf("1. List all files and folders in the root directory\n");
        printf("2. List files and folders in a subdirectory\n");
        printf("3. Display contents of a file\n");
        printf("4. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                list_directory(image_path);
                break;
            case 2:
                printf("Enter the subdirectory path: ");
                scanf("%s", path);
                list_directory(path);
                break;
            case 3:
                printf("Enter the file path: ");
                scanf("%s", filepath);
                display_file(filepath);
                break;
            case 4:
                exit(0);
            default:
                printf("Invalid choice. Please try again.\n");
        }
    }

    return 0;
}
