#include <stdio.h>

int main() {
    FILE *fp = fopen("pic.jpg", "r");
    if (fp == NULL) {
        perror("Error opening file");
        return 1;
    }
    
    // Move file pointer to the end of the file
    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("Error seeking to end of file");
        fclose(fp);
        return 1;
    }
    
    // Get the position of the file pointer, which is the file size
    long file_size = ftell(fp);
    if (file_size == -1) {
        perror("Error getting file size");
        fclose(fp);
        return 1;
    }
    
    fclose(fp);
    
    printf("File size: %ld bytes\n", file_size);
    
    return 0;
}
