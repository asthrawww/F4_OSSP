#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

 int main()
{
    int fd;

    
    char *fifo_path = "/tmp/Srilaxmi_pipe";
    mkfifo(fifo_path, 0666);

    char message[] = "Hello from the independent writer process!";

    printf("Writer: Waiting for a reader to open the pipe...\n");

    
    
    fd = open(fifo_path, O_WRONLY);

    printf("Writer: Reader connected! Sending message...\n");

    write(fd, message, strlen(message) + 1);

    close(fd);

    printf("Writer: Message sent and pipe closed.\n");

    return 0;
}
