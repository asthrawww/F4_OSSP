#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int count = 0;

int main() {
    int pid;

    pid = fork();

    if (pid < 0) {
        perror("Problem in creating child");
        exit(1);
    }
    else if (pid == 0) {
        printf("Child process is executing\n");

        for (int i = 0; i < 5; i++) {
            count++;
            printf("Child count = %d\n", count);
        }
    }
    else {
        wait(NULL);

        printf("Parent process is executing\n");

        for (int i = 0; i < 5; i++) {
            count++;
            printf("Parent count = %d\n", count);
        }
    }

    return 0;
}
