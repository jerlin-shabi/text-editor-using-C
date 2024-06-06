#include <stdio.h>
#include <stdlib.h>

void openFile(char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Could not open file %s\n", filename);
        return;
    }
    char ch;
    while ((ch = fgetc(file)) != EOF) {
        putchar(ch);
    }
    fclose(file);
}

void writeFile(char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Could not open file %s\n", filename);
        return;
    }
    char ch;
    printf("Enter text (CTRL+D to save):\n");
    while ((ch = getchar()) != EOF) {
        fputc(ch, file);
    }
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <command> <filename>\n", argv[0]);
        printf("Commands:\n  open - open and read a file\n  write - write to a file\n");
        return 1;
    }

    char *command = argv[1];
    char *filename = argv[2];

    if (strcmp(command, "open") == 0) {
        openFile(filename);
    } else if (strcmp(command, "write") == 0) {
        writeFile(filename);
    } else {
        printf("Unknown command %s\n", command);
    }

    return 0;
}