#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_PATH 256
#define CLEAR_SCREEN "\033[2J\033[H"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RED "\033[0;31m"
#define COLOR_RESET "\033[0m"

// Function to list files in current directory
void list_files() {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char path[MAX_PATH];
    
    printf("\n%sAvailable files in current directory:%s\n", COLOR_BLUE, COLOR_RESET);
    printf("----------------------------------------\n");
    
    dir = opendir(".");
    if (dir == NULL) {
        printf("%sError opening current directory%s\n", COLOR_RED, COLOR_RESET);
        return;
    }
    
    printf("%-40s %15s\n", "Filename", "Size");
    printf("----------------------------------------\n");
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {  // Only show regular files
            snprintf(path, sizeof(path), "%s", entry->d_name);
            if (stat(path, &file_stat) == 0) {
                // Convert size to appropriate unit
                double size = file_stat.st_size;
                const char* unit = "B";
                if (size > 1024*1024*1024) {
                    size /= 1024*1024*1024;
                    unit = "GB";
                } else if (size > 1024*1024) {
                    size /= 1024*1024;
                    unit = "MB";
                } else if (size > 1024) {
                    size /= 1024;
                    unit = "KB";
                }
                printf("%-40s %8.2f %s\n", entry->d_name, size, unit);
            }
        }
    }
    closedir(dir);
    printf("\n");
}

// Function to check if file exists
int file_exists(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Function to display the main menu
void display_menu() {
    printf("%s╔════════════════════════════════════╗%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s║      File Transfer System Menu     ║%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s╠════════════════════════════════════╣%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("%s║%s 1. List available files             %s║%s\n", COLOR_YELLOW, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("%s║%s 2. Send a file                      %s║%s\n", COLOR_YELLOW, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("%s║%s 3. Clear screen                     %s║%s\n", COLOR_YELLOW, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("%s║%s 4. Exit                            %s║%s\n", COLOR_YELLOW, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("%s╚════════════════════════════════════╝%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("\nEnter your choice (1-4): ");
}

int main() {
    char filename[MAX_PATH];
    char command[MAX_PATH * 2];
    int choice;
    
    // Print welcome message
    printf(CLEAR_SCREEN);
    printf("%s╔════════════════════════════════════════════╗%s\n", COLOR_GREEN, COLOR_RESET);
    printf("%s║  Welcome to the File Transfer System CLI!  ║%s\n", COLOR_GREEN, COLOR_RESET);
    printf("%s╚════════════════════════════════════════════╝%s\n\n", COLOR_GREEN, COLOR_RESET);
    
    while (1) {
        display_menu();
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // Clear input buffer
            printf("%sInvalid input. Please enter a number.%s\n", COLOR_RED, COLOR_RESET);
            continue;
        }
        while (getchar() != '\n'); // Clear input buffer
        
        switch (choice) {
            case 1:
                list_files();
                break;
                
            case 2:
                list_files();
                printf("\nEnter the filename to send: ");
                if (fgets(filename, sizeof(filename), stdin)) {
                    // Remove newline if present
                    filename[strcspn(filename, "\n")] = 0;
                    
                    if (file_exists(filename)) {
                        // Start server if not running
                        system("pkill -f \"./server\" 2>/dev/null"); // Kill any existing server
                        system("./server & sleep 1"); // Start server and wait a bit
                        
                        // Run client with the file
                        snprintf(command, sizeof(command), "./client \"%s\"", filename);
                        printf("%sStarting file transfer...%s\n", COLOR_BLUE, COLOR_RESET);
                        system(command);
                        
                        printf("\n%sFile transfer completed. Press Enter to continue...%s", COLOR_GREEN, COLOR_RESET);
                        getchar();
                        printf(CLEAR_SCREEN);
                    } else {
                        printf("%sError: File '%s' not found!%s\n", COLOR_RED, filename, COLOR_RESET);
                    }
                }
                break;
                
            case 3:
                printf(CLEAR_SCREEN);
                break;
                
            case 4:
                printf("\n%sThank you for using the File Transfer System!%s\n", COLOR_GREEN, COLOR_RESET);
                system("pkill -f \"./server\" 2>/dev/null"); // Clean up server process
                exit(0);
                
            default:
                printf("%sInvalid choice. Please enter a number between 1 and 4.%s\n", COLOR_RED, COLOR_RESET);
        }
    }
    
    return 0;
} 