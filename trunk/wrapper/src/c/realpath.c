#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {
    
    char* resolved;
    
    if(argc != 2) {
        fprintf(stderr, "Usage: realpath path\n");
        return 1;
    }

    resolved = malloc(PATH_MAX * sizeof(char));
    
    if(realpath(argv[1], resolved) == NULL) {
        fprintf(stderr, "Could not resolve %s: %s\n", argv[1], (char *)strerror(errno));
        return 1;
    }
    
    printf(resolved);
    
    free(resolved);

    return 0;
}

		

