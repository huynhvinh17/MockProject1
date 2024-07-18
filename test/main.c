#include "HAL.h"
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

            fatfs_init(image_path);
            printf("\nPress Enter to continue...");
            while (getchar() != '\n');
            getchar();
        }
    }

    return 0;
}
