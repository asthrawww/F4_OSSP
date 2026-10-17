
#include <stdio.h>
#define MAX 10

int n, m, alloc[MAX][MAX], max[MAX][MAX];
int need[MAX][MAX], avail[MAX], work[MAX];
int finish[MAX], seq[MAX], count = 0;

void safeSequences(int depth) {
    if (depth == n) {
        count++;
        printf("Safe Sequence %d: ", count);
        for (int i = 0; i < n; i++)
            printf("P%d ", seq[i]);
        printf("\n");
        return;
    }

    for (int i = 0; i < n; i++) {
        if (!finish[i]) {
            int ok = 1;

            for (int j = 0; j < m; j++)
                if (need[i][j] > work[j])
                    ok = 0;

            if (ok) {
                for (int j = 0; j < m; j++)
                    work[j] += alloc[i][j];

                finish[i] = 1;
                seq[depth] = i;
                safeSequences(depth + 1);
                finish[i] = 0;

                for (int j = 0; j < m; j++)
                    work[j] -= alloc[i][j];
            }
        }
    }
}

int main(void) {
    printf("Enter number of processes and resources: ");
    scanf("%d %d", &n, &m);

    if (n < 1 || n > MAX || m < 1 || m > MAX) {
        printf("Invalid input.\n");
        return 1;
    }

    printf("Enter Allocation Matrix:\n");
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) {
            scanf("%d", &alloc[i][j]);
            if (alloc[i][j] < 0) {
                printf("Invalid allocation.\n");
                return 1;
            }
        }

    printf("Enter Maximum Matrix:\n");
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++) {
            scanf("%d", &max[i][j]);
            if (max[i][j] < alloc[i][j]) {
                printf("Maximum cannot be less than allocation.\n");
                return 1;
            }
            need[i][j] = max[i][j] - alloc[i][j];
        }

    printf("Enter Available Resources:\n");
    for (int j = 0; j < m; j++) {
        scanf("%d", &avail[j]);
        if (avail[j] < 0) {
            printf("Invalid available resources.\n");
            return 1;
        }
        work[j] = avail[j];
    }

    printf("\nNeed Matrix:\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < m; j++)
            printf("%d\t", need[i][j]);
        printf("\n");
    }

    for (int i = 0; i < n; i++)
        finish[i] = 0;

    printf("\nAll Possible Safe Sequences:\n");
    safeSequences(0);

    if (count > 0)
        printf("System is SAFE. Total sequences: %d\n", count);
    else
        printf("System is UNSAFE. No safe sequence exists.\n");

    return 0;
}
