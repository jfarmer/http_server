#include <stddef.h>
#include <stdio.h>
#include <string.h>

int main() {
    char str1[] = "hello\r\nworld"; // Fixed declaration to char array
    char str2[] = "waffle\n\rhouse"; // Fixed declaration to char array

    // Tokenize str1
    char* token = strtok(str1, "\r\n");
    while (token != NULL) {
        printf("%s\n", token);
        token = strtok(NULL, "\r\n");
    }

    // Tokenize str2
    token = strtok(str2, "\r\n");
    while (token != NULL) {
        printf("%s\n", token);
        token = strtok(NULL, "\r\n");
    }

    return 0;
}
