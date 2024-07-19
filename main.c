#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FATfs.h"

#define MAX_PATH_LENGTH 255

int main(void)
{
    const char *image_path = "floppy.img";

    if (fatfs_init(image_path) != 0)
    {
        fprintf(stderr, "Failed to initialize FAT filesystem\n");
        return 1;
    }
    DirEntry *head = NULL;
    DirEntry *tail = NULL;
    DirectoryStack *dirStack = NULL;

    uint32_t currentCluster = 0;
    fatfs_read_dir(currentCluster, &head, &tail);

    int choice;
    char currentPath[MAX_PATH_LENGTH] = "/";
    char rootPath[MAX_PATH_LENGTH] = "/";

    while (1)
    {
#ifdef _WIN32
        system("cls");
#else
        system("clear");
#endif
        printf("\033[2J\033[H"); // Clear the screen
        printf("\nCurrent Directory: %s\n", currentPath);
        display_entries(head);

        printf("\nOptions:\n");
        printf("1. Open a file or directory\n");
        printf("2. Go back to parent directory\n");
        printf("3. Go back to root directory\n");
        printf("4. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        if (choice == 1)
        {
            int index;
            printf("Enter the index of the file or directory to open: ");
            scanf("%d", &index);

            DirEntry *entry = get_entry_by_index(head, index);
            if (entry)
            {
                if (entry->is_dir)
                {
                    push(&dirStack, currentCluster);
                    currentCluster = entry->first_cluster;
                    strcat(currentPath, "/");
                    strcat(currentPath, entry->name);
                    free_entries(head);
                    head = tail = NULL;
                    fatfs_read_dir(currentCluster, &head, &tail);
                }
                else
                {
                    printf("Reading file %s:\n", entry->name);
                    fatfs_read_file(entry->name, entry->first_cluster);
                    printf("\nPress Enter to continue...");
                    getchar();
                    getchar();
                }
            }
            else
            {
                printf("Invalid index or entry type.\n");
                printf("Press Enter to continue...");
                getchar();
                getchar();
            }
        }
        else if (choice == 2)
        {
            if (dirStack != NULL)
            {
                currentCluster = pop(&dirStack);
                free_entries(head);
                head = tail = NULL;
                strcpy(currentPath, rootPath);
                fatfs_read_dir(currentCluster, &head, &tail);
            }
            else
            {
                printf("Already at the root directory.\n");
                printf("Press Enter to continue...");
                getchar();
                getchar();
            }
        }
        else if (choice == 3)
        {
            currentCluster = 0;
            free_entries(head);
            head = tail = NULL;
            strcpy(currentPath, rootPath);
            fatfs_read_dir(currentCluster, &head, &tail);
        }
        else if (choice == 4)
        {
            break;
        }
        else
        {
            printf("Invalid choice. Please try again.\n");
            printf("Press Enter to continue...");
            getchar();
            getchar();
        }
    }

    free_entries(head);
    fatfs_deinit();
    return 0;
}
