#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SM_NAME "shmfile"
#define NUM_MARKS 5

int main() {
    key_t key;
    int shmid;
    int *marks;

    key = ftok(SM_NAME, 65);

    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    shmid = shmget(key, NUM_MARKS * sizeof(int), IPC_CREAT | 0666);

    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }

    marks = (int *)shmat(shmid, NULL, 0);

    if (marks == (int *)-1) {
        perror("shmat");
        exit(1);
    }

    printf("Producer Process\n");
    printf("----------------\n");
    printf("Enter marks for %d subjects:\n", NUM_MARKS);

    for (int i = 0; i < NUM_MARKS; i++) {
        printf("Enter mark %d: ", i + 1);
        scanf("%d", &marks[i]);
    }

    printf("\nMarks successfully written to shared memory.\n");

    if (shmdt(marks) == -1) {
        perror("shmdt");
        exit(1);
    }

    return 0;
}
