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
    int sum = 0;
    float average;

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

    printf("Consumer Process\n");
    printf("----------------\n");
    printf("Student marks read from shared memory:\n");

    for (int i = 0; i < NUM_MARKS; i++) {
        printf("%d ", marks[i]);
        sum += marks[i];
    }

    printf("\n");

    average = (float)sum / NUM_MARKS;

    printf("Total Marks = %d\n", sum);
    printf("Average Marks = %.2f\n", average);

    if (shmdt(marks) == -1) {
        perror("shmdt");
        exit(1);
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(1);
    }

    return 0;
}
