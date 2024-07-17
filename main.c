#include "FATfs.h"
#include <stdio.h>
#include <stdlib.h>



int main()
{
    const char *image_path = "floppy.img";

    if (fatfs_init(image_path) != 0)
    {
        fprintf(stderr, "Failed to initialize FAT filesystem\n");
        return -1;
    }

    DirEntry *head = NULL;
    DirEntry *tail = NULL;

    read_dir(0, &head, &tail);
    display_entries(head);

    int index;
    printf("\nEnter the index of the directory or file to read (or -1 to exit): ");
    while (scanf("%d", &index) == 1 && index != -1)
    {
        DirEntry *entry = get_entry_by_index(head, index);
        if (!entry)
        {
            printf("Invalid index. Try again.\n");
        }
        else if (entry->is_dir)
        {
            free_entries(head);
            head = tail = NULL;
            read_dir(entry->first_cluster, &head, &tail);
            display_entries(head);
        }
        else
        {
            printf("\nReading file: %s\n", entry->name);
            fatfs_read_file(entry->name, entry->first_cluster);
        }
        printf("\nEnter the index of the directory or file to read (or -1 to exit): ");
    }

    free_entries(head);
    fatfs_deinit();
    return 0;
}
