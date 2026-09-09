#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

void handler(int signo)
{
    printf("Signal received\n");
    printf("Signal no: %d\n", signo);
    exit(1);
}

int main()
{
    signal(SIGINT, handler);

    while (1)
    {
        printf("Infinite no. of prints\n");
        break;
    }

    raise(SIGINT);

    return 0;
}
