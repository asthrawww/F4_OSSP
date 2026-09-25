#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    int pid;

    pid = fork();

    if (pid < 0) {
        perror("Problem in creating child");
        exit(1);
    }
    else if (pid == 0) {
        printf("Child process is executing\n");
    }
    else {
        printf("Parent process executing\n");
    }

    return 0;
}
