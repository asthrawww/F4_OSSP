#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node
{
    char *command;
    struct Node *next;
};

int main()
{
    char *buffer;
    int size = 10;
    int length = 0;
    int ch;

    struct Node *head = NULL;
    struct Node *tail = NULL;

    buffer = (char *)malloc(size * sizeof(char));

    if (buffer == NULL)
    {
        printf("Memory allocation failed.\n");
        return 1;
    }

    printf("Enter a command: ");

    while ((ch = getchar()) != '\n')
    {
        if (length >= size - 1)
        {
            size *= 2;

            buffer = (char *)realloc(buffer, size * sizeof(char));

            if (buffer == NULL)
            {
                printf("Memory reallocation failed.\n");
                return 1;
            }
        }

        buffer[length++] = ch;
    }

    buffer[length] = '\0';

    struct Node *newNode =
        (struct Node *)malloc(sizeof(struct Node));

    if (newNode == NULL)
    {
        printf("Memory allocation failed.\n");
        free(buffer);
        return 1;
    }

    newNode->command = (char *)malloc((strlen(buffer) + 1) * sizeof(char));

    if (newNode->command == NULL)
    {
        printf("Memory allocation failed.\n");
        free(newNode);
        free(buffer);
        return 1;
    }

    strcpy(newNode->command, buffer);
    newNode->next = NULL;

    head = newNode;
    tail = newNode;

    printf("\nStored command: %s\n", head->command);

    free(head->command);
    free(head);
    free(buffer);

    printf("Memory released successfully.\n");

    return 0;
}
