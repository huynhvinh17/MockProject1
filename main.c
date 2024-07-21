#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "FATfs.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define MAX_PATH_LENGTH 255 /** Define maximum length for path strings */

/*******************************************************************************
 * Code
 ******************************************************************************/

void format_date(uint16_t date, char *buffer)
{
    int year = ((date >> 9) & 0x7F) + 1980;
    int month = (date >> 5) & 0x0F;
    int day = date & 0x1F;
    sprintf(buffer, " %02d-%02d-%04d", day, month, year);
}

void format_time(uint16_t time, char *buffer)
{
    int hour = (time >> 11) & 0x1F;
    int minute = (time >> 5) & 0x3F;
    sprintf(buffer, "%02d:%02d", hour, minute);
}

void display_entries(DirEntry *head)
{
    DirEntry *current = head; /** Pointer to the list */
    int index = 0;            /** Index for displaying entries */
    char modified_time[7];
    char modified_date[11];
    printf("Index   Name            Size    Type    Modified\n");
    while (current)
    {
        format_date(current->modified_date, modified_date);
        format_time(current->modified_time, modified_time);
        printf("%-7d %-15s %-7u %-7s %-12s\n", index++, current->name, current->size, current->is_dir ? "DIR" : "FILE", strcat(modified_time, modified_date));
        current = current->next; /** Move to the next entry */
    }
}

void printOption(void)
{
    /** Display the entries in the current directory */
    printf("\nOptions:\n");
    printf("1. Open a file or directory\n");
    printf("2. Go back to previous directory\n");
    printf("3. Go back to root directory\n");
    printf("4. Exit\n");
    printf("Enter your choice: ");
}

int main(void)
{
    const char *image_path = "floppy.img"; /** Path to the FAT filesystem image */
    DirEntry *head = NULL;                 /** Head of the linked list of directory entries */
    DirectoryStack *dirStack = NULL;       /** Stack for keeping of directory cluster */
    uint32_t currentCluster = 0;           /** Current cliuster for directory */
    int choice = 0;
    int index = 0;                           /** Variable to store user's menu choice */
    bool checkChoice = true;                 /** Control variable for the main loop */
    char currentPath[MAX_PATH_LENGTH] = "/"; /** Curren path string */
    char rootPath[MAX_PATH_LENGTH] = "/";    /** Root path string for navigation */

    /** Initialize the FAT filesystem with the provided image path */
    if (fatfs_init(image_path) != 0)
    {
        fprintf(stderr, "Failed to initialize FAT filesystem\n");
    }
    else
    {
        fatfs_read_dir(currentCluster, &head);

        while (checkChoice)
        {
            system("cls"); /** clear the screen on windows */
            printf("\nCurrent Directory: %s\n", currentPath);
            display_entries(head); /** Display the entries in the current directory */

            printOption();

            scanf("%d", &choice); /** Read user's choice */

            if (choice == 1)
            {
                printf("Enter the index of the file or directory to open: ");
                scanf("%d", &index); /**Read input until newline character */

                DirEntry *entry = get_entry_by_index(head, index); /** Get the directory entry by index */
                if (entry)
                {
                    printf("Entry %d \n", entry->first_cluster);
                    if (entry->is_dir)
                    {
                        push(&dirStack, currentCluster);       /** If the entry is a directory, push the current cluster onto the stack */
                        currentCluster = entry->first_cluster; /** Update the current cluster to the new directory */
                        strcat(currentPath, "/");
                        strcat(currentPath, entry->name); /** Update the current path */
                        free_entries(head);               /** Free the old directory entries */
                        head = NULL;
                        fatfs_read_dir(currentCluster, &head); /** Read the new directory */
                    }
                    else
                    {
                        printf("Reading file %s:\n", entry->name); /** If the entry is a file, read and display its contents */
                        fatfs_read_file(entry->name, entry->first_cluster);
                        printf("\nPress Enter to continue...");
                        getchar();
                        getchar();
                    }
                }
                else
                {
                    printf("Invalid index or entry type.\n");
                    printf("\nPress Enter to continue...");
                    getchar();
                    getchar();
                }
            }
            else if (choice == 2)
            {
                /** Go back to the parent directory */
                if (dirStack != NULL)
                {
                    currentCluster = pop(&dirStack); /** Pop the cluster from the stack */
                    free_entries(head);              /** Free the old directory entries */
                    head = NULL;
                    strcpy(currentPath, rootPath);         /** Reset the current path to root */
                    fatfs_read_dir(currentCluster, &head); /** Read the parent directory */
                }
                else
                {
                    printf("Already at the root directory.\n");
                    printf("\nPress Enter to continue...");
                    getchar();
                    getchar();
                }
            }
            else if (choice == 3)
            {
                currentCluster = 0; /** Set the current cluster to root */
                free_entries(head); /** Free the old directory entries */
                head = NULL;
                strcpy(currentPath, rootPath);         /** Reset the current path to root */
                fatfs_read_dir(currentCluster, &head); /** Read the root directory */
            }
            else if (choice == 4)
            {
                checkChoice = false; /** Exit the program */
            }
            else
            {
                printf("Invalid choice. Please try again.\n");
                printf("\nPress Enter to continue...");
                getchar();
                getchar();
            }
        }

        /** Free allocated resources and deinitialize the FAT filesystem */
        free_entries(head);
        fatfs_deinit();
    }

    return 0;
}
