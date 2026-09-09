#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    char input[100];

    while (1)
    {
        printf("myshell> ");
        fgets(input, sizeof(input), stdin);

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0)
        {
            printf("Exiting shell...\n");
            break;
        }

        if (strlen(input) == 0)
        {
            continue;
        }

        printf("You entered: %s\n", input);
    }

    return 0;
}
