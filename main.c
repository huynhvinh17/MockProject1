#include "FATfs.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    const char *image_path = "floppy.img";

    if (fatfs_init(image_path) != 0)
    {
        fprintf(stderr, "Failed to initialize FAT filesystem\n");
    }
    else
    {
        char path[256] = ".";
        int choice;
        char filepath[256];

        while (1)
        {
#ifdef _WIN32
            system("cls");
#else
            system("clear");
#endif

            printf("\n+---------------------------------------------+\n");
            printf("|                  OPTIONS                    |\n");
            printf("+---------------------------------------------+\n");
            printf("| 1. List all files and folders in the root   |\n");
            printf("|    directory                                |\n");
            printf("| 2. List files and folders in a subdirectory |\n");
            printf("| 3. Display contents of a file               |\n");
            printf("| 4. Exit                                     |\n");
            printf("+---------------------------------------------+\n");

            fatfs_init(image_path);

            printf("Please enter your choice: ");
            while (scanf("%d", &choice) != 1)
            {
                while (getchar() != '\n');
            }

            switch (choice)
            {
            case 1:
                printf("\nListing root directory:\n");
                fatfs_list_directory(path);
                break;
            case 2:
                printf("Please enter the subdirectory path: ");
                scanf("%s", path);
                fatfs_list_directory(path);
                break;
            case 3:
                printf("Please enter the file path: ");
                scanf("%s", filepath);
                fatfs_display_file(filepath);
                break;
            case 4:
                fatfs_deinit();
                hal_cleanup();
                exit(0);
            default:
                printf("Invalid choice!\n");
                break;
            }

            printf("\nPress Enter to continue...");
            while (getchar() != '\n');
            getchar();
        }
    }

    return 0;
}
