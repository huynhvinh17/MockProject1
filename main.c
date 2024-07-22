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
 * Prototype
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/**
 * @brief Formats a 16-bit date value into a string in "dd-mm-yyyy" format.
 *
 * @param date A 16-bit date value to be formatted.
 * @param buffer A pointer to a character array will be stored
 */
void format_date(uint16_t date, char *buffer)
{
    int year = ((date >> 9) & 0x7F) + 1980;               /** Extract year and adjust to full year */
    int month = (date >> 5) & 0x0F;                       /** Extract month */
    int day = date & 0x1F;                                /** Extract day */
    sprintf(buffer, " %02d-%02d-%04d", day, month, year); /** Format and store the date string */
}

/**
 * @brief Formats a 16-nit time value into a string in "hh:mm" format
 *
 * @param time A 16-bit time value to be formatted
 * @param buffer A pointer to a character array will be stored
 */
void format_time(uint16_t time, char *buffer)
{
    int hour = (time >> 11) & 0x1F;             /** Extract hour */
    int minute = (time >> 5) & 0x3F;            /** Extract minute */
    sprintf(buffer, "%02d:%02d", hour, minute); /** Format and store time string */
}

/**
 * @brief Displays the entries of a directory in a formatted table
 *
 * @param head A pointer to the first 'DirEntry' in the linked list
 */
void display_entries(DirEntry *head)
{
    DirEntry *current = head; /** Pointer to the list */
    int index = 1;            /** Index for displaying entries */
    char modified_time[7];    /** Buffer to store formatted time */
    char modified_date[11];   /** Buffer to store formatted date */

    printf("Index   Name            Size    Type    Modified\n");

    while (current)
    {
        /** Format the date and time of the entry */
        format_date(current->modified_date, modified_date);
        format_time(current->modified_time, modified_time);

        /** Print the entry details */
        printf("%-7d %-15s %-7u %-7s %-12s\n", index++, current->name, current->size, current->is_dir ? "DIR" : "FILE", strcat(modified_time, modified_date));

        current = current->next; /** Move to the next entry */
    }
}

/**
 * @brief Display the option for user's choice
 */
void printOption(void)
{
    printf("\nOptions:\n");
    printf("1. Open a file or directory\n");
    printf("2. Go back to root directory\n");
    printf("3. Exit\n");
    printf("Enter your choice: ");
}

int main(void)
{
    const char *image_path = "floppy.img"; /** Path to the FAT filesystem image */
    DirEntry *head = NULL;                 /** Head of the linked list of directory entries */
    uint32_t currentCluster = 0;           /** Current cliuster for directory */

    int choice = 0;
    int index = 0;                              /** Variable to store user's menu choice */
    bool checkChoice = true;                    /** Control variable for the main loop */

    char currentPath[MAX_PATH_LENGTH] = "/";    /** Curren path string */
    char rootPath[MAX_PATH_LENGTH] = "/";       /** Root path string for navigation */

    /** Initialize the FAT filesystem with the provided image path */
    if (fatfs_init(image_path) != 0)
    {
        fprintf(stderr, "Failed to initialize FAT filesystem\n");
    }
    else
    {
        fatfs_read_dir(currentCluster, &head); /** Reads thte contents of a directory from the FAT filesystem */

        while (checkChoice)
        {
            system("cls"); /** clear the screen on windows */
            printf("\nCurrent Directory: %s\n", currentPath);
            display_entries(head); /** Display the entries in the current directory */

            printOption();  /** Display the option for user's choice */

            scanf("%d", &choice); /** Read user's choice */

            if (1 == choice)
            {
                printf("Enter the index of the file or directory to open: ");
                scanf("%d", &index); /**Read input until newline character */

                DirEntry *entry = get_entry_by_index(head, index); /** Get the directory entry by index */

                if (entry)
                {
                    printf("Entry %d \n", entry->first_cluster);

                    if (entry->is_dir)
                    {
                        currentCluster = entry->first_cluster; /** Update the current cluster to the new directory */
                        strcat(currentPath, "/");              /** Update the current path with / */
                        strcat(currentPath, entry->name);      /** Update the current path */
                        free_entries(head);                    /** Free the old directory entries */
                        head = NULL;                           /** Reset */
                        fatfs_read_dir(currentCluster, &head); /** Read the new directory */
                    }
                    else
                    {
                        printf("Reading file %s:\n", entry->name);          /** If the entry is a file, read and display its contents */
                        fatfs_read_file(entry->name, entry->first_cluster); /** Reads the content of a file from the FAT filesystem. */

                        printf("\nPress Enter to continue...");
                        getchar(); /** Read a character from the input buffer */
                        getchar(); /** waits for the user to press Enter */
                    }
                }
                else
                {
                    printf("Invalid index or entry type.\n");
                    printf("\nPress Enter to continue...");

                    getchar(); /** Read a character from the input buffer */
                    getchar(); /** waits for the user to press Enter */
                }
            }
            else if (2 == choice)
            {
                currentCluster = 0;                    /** Set the current cluster to root */
                free_entries(head);                    /** Free the old directory entries */
                head = NULL;                           /** Reset */
                strcpy(currentPath, rootPath);         /** Reset the current path to root */
                fatfs_read_dir(currentCluster, &head); /** Read the root directory */
            }
            else if (3 == choice)
            {
                checkChoice = false; /** Exit the program */
            }
            else
            {
                printf("Invalid choice. Please try again.\n");
                printf("\nPress Enter to continue...");
                getchar(); /** Read a character from the input buffer */
                getchar(); /** waits for the user to press Enter */
            }
        }

        /** Free allocated resources and deinitialize the FAT filesystem */
        free_entries(head);
        fatfs_deinit();
    }

    return 0;
}
