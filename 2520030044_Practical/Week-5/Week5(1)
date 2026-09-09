#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int pid;
    int fd[2];
    char msg[] = "Hello World";
    char buffer[100];

    if(pipe(fd) == -1)
    {
        perror("pipe");
        return 1;
    }

    pid = fork();

    if(pid < 0)
    {
        perror("fork");
        return 1;
    }
    else if(pid == 0)
    {
        // Child process
        close(fd[1]);

        read(fd[0], buffer, strlen(msg) + 1);

        printf("Message from Parent: %s\n", buffer);
        printf("Child process\n");

        close(fd[0]);
    }
    else
    {
        // Parent process
        printf("Parent process\n");

        close(fd[0]);

        write(fd[1], msg, strlen(msg) + 1);

        close(fd[1]);
    }

    return 0;
}
