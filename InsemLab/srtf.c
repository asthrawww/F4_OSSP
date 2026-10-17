
#include <stdio.h>
#define MAX 20

int main(void) {
    int n, at[MAX], bt[MAX], rt[MAX];
    int ct[MAX], tat[MAX], wt[MAX], response[MAX];
    int first[MAX], time = 0, done = 0;
    int prev = -1, switches = 0, segments = 0;
    int pid[MAX * 1000], start[MAX * 1000], end[MAX * 1000];
    float avgTAT = 0, avgWT = 0, avgRT = 0;

    printf("Enter number of processes: ");
    scanf("%d", &n);

    if (n < 1 || n > MAX) {
        printf("Invalid number of processes.\n");
        return 1;
    }

    for (int i = 0; i < n; i++) {
        printf("Enter arrival and burst time of P%d: ", i);
        scanf("%d %d", &at[i], &bt[i]);

        if (at[i] < 0 || bt[i] <= 0) {
            printf("Invalid input.\n");
            return 1;
        }

        rt[i] = bt[i];
        first[i] = -1;
    }

    while (done < n) {
        int idx = -1;

        for (int i = 0; i < n; i++) {
            if (at[i] <= time && rt[i] > 0) {
                if (idx == -1 || rt[i] < rt[idx])
                    idx = i;
            }
        }

        int current = (idx == -1) ? -1 : idx;

        if (current != prev && prev != -1 &&
            current != -1)
            switches++;

        if (segments == 0 || pid[segments - 1] != current) {
            pid[segments] = current;
            start[segments] = time;
            end[segments] = time + 1;
            segments++;
        } else {
            end[segments - 1] = time + 1;
        }

        if (idx == -1) {
            time++;
            prev = -1;
            continue;
        }

        if (first[idx] == -1)
            first[idx] = time;

        rt[idx]--;
        time++;

        if (rt[idx] == 0) {
            ct[idx] = time;
            tat[idx] = ct[idx] - at[idx];
            wt[idx] = tat[idx] - bt[idx];
            response[idx] = first[idx] - at[idx];
            done++;
        }

        prev = current;
    }

    printf("\nGantt Chart:\n");
    for (int i = 0; i < segments; i++) {
        if (pid[i] == -1)
            printf("| IDLE ");
        else
            printf("| P%d ", pid[i]);
    }
    printf("|\n");

    printf("Timeline:\n");
    for (int i = 0; i < segments; i++)
        printf("%d\t", start[i]);
    printf("%d\n", end[segments - 1]);

    printf("\nPID\tAT\tBT\tCT\tTAT\tWT\tRT\n");

    for (int i = 0; i < n; i++) {
        printf("P%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
               i, at[i], bt[i], ct[i],
               tat[i], wt[i], response[i]);

        avgTAT += tat[i];
        avgWT += wt[i];
        avgRT += response[i];
    }

    printf("\nAverage TAT = %.2f\n", avgTAT / n);
    printf("Average WT  = %.2f\n", avgWT / n);
    printf("Average RT  = %.2f\n", avgRT / n);
    printf("Context switches = %d\n", switches);

    return 0;
}
