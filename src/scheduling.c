#include <stdio.h>
#include "scheduling.h"

/*
 * Tie-breaking rules (printed in the output):
 *  FCFS     : earlier arrival first; same arrival -> lower Job ID.
 *  RR       : queue order is arrival, then lower Job ID; a job that arrives
 *             at the same moment a quantum ends is queued before the
 *             preempted job.
 *  Priority : lower priority number first; then earlier arrival; then lower Job ID.
 */

/* Sorts job indices by arrival time, then Job ID. */
static void sort_by_arrival(const Job jobs[], int n, int order[]) {
    for (int i = 0; i < n; i++) {
        int j = i;
        while (j > 0 &&
               (jobs[order[j - 1]].arrival > jobs[i].arrival ||
                (jobs[order[j - 1]].arrival == jobs[i].arrival &&
                 jobs[order[j - 1]].id > jobs[i].id))) {
            order[j] = order[j - 1];
            j--;
        }
        order[j] = i;
    }
}

/* Prints the CT/TAT/WT table (jobs in input order) and the averages. */
static void print_results(const Job jobs[], int n, const int ct[], int show_priority) {
    double total_wt = 0, total_tat = 0;

    printf("\nJob\t");
    if (show_priority) printf("Priority\t");
    printf("CT\tTAT\tWT\n");

    for (int i = 0; i < n; i++) {
        int tat = ct[i] - jobs[i].arrival;
        int wt  = tat - jobs[i].burst;
        total_tat += tat;
        total_wt  += wt;

        printf("J%d\t", jobs[i].id);
        if (show_priority) printf("%d\t\t", jobs[i].priority);
        printf("%d\t%d\t%d\n", ct[i], tat, wt);
    }
    printf("\nAverage Waiting Time: %.2f\n", total_wt / n);
    printf("Average Turnaround Time: %.2f\n", total_tat / n);
}

void fcfs_schedule(const Job jobs[], int n) {
    int order[MAX_JOBS], ct[MAX_JOBS], time = 0;

    if (n <= 0 || n > MAX_JOBS) {
        printf("FCFS: invalid number of jobs.\n");
        return;
    }
    sort_by_arrival(jobs, n, order);

    printf("\nFCFS Scheduling Results\n");
    printf("Tie-break: earlier arrival, then lower Job ID\n");
    printf("Execution Order: ");

    for (int k = 0; k < n; k++) {
        int i = order[k];
        if (time < jobs[i].arrival) time = jobs[i].arrival;   /* CPU idle */
        int start = time;
        time += jobs[i].burst;
        ct[i] = time;
        printf("J%d(%d-%d) ", jobs[i].id, start, time);
    }
    printf("\n");

    print_results(jobs, n, ct, 0);
}

void round_robin_schedule(const Job jobs[], int n, int quantum) {
    int order[MAX_JOBS], rem[MAX_JOBS], ct[MAX_JOBS];
    int queue[MAX_JOBS + 1], head = 0, tail = 0, count = 0;
    int next = 0, done = 0, time = 0, printed = 0;

    if (n <= 0 || n > MAX_JOBS) {
        printf("Round Robin: invalid number of jobs.\n");
        return;
    }
    if (quantum <= 0) {
        printf("Round Robin: time quantum must be greater than 0.\n");
        return;
    }
    sort_by_arrival(jobs, n, order);
    for (int i = 0; i < n; i++) rem[i] = jobs[i].burst;

    printf("\nRound Robin Scheduling\n");
    printf("Time Quantum: %d\n", quantum);
    printf("Tie-break: arrival order, then lower Job ID; new arrivals are queued before a preempted job\n");
    printf("Execution Order: ");

    while (done < n) {
        /* queue every job that has arrived by now */
        while (next < n && jobs[order[next]].arrival <= time) {
            queue[tail] = order[next++];
            tail = (tail + 1) % (MAX_JOBS + 1);
            count++;
        }
        if (count == 0) {                      /* CPU idle: jump to next arrival */
            time = jobs[order[next]].arrival;
            continue;
        }

        int i = queue[head];
        head = (head + 1) % (MAX_JOBS + 1);
        count--;

        int run = rem[i] < quantum ? rem[i] : quantum;
        printf("J%d(%d-%d) ", jobs[i].id, time, time + run);
        if (++printed % 6 == 0) printf("\n                 ");   /* wrap long lines */
        time += run;
        rem[i] -= run;

        /* jobs that arrived during this slice go in before the preempted job */
        while (next < n && jobs[order[next]].arrival <= time) {
            queue[tail] = order[next++];
            tail = (tail + 1) % (MAX_JOBS + 1);
            count++;
        }
        if (rem[i] > 0) {
            queue[tail] = i;
            tail = (tail + 1) % (MAX_JOBS + 1);
            count++;
        } else {
            ct[i] = time;
            done++;
        }
    }
    printf("\n");

    print_results(jobs, n, ct, 0);
}

void priority_schedule(const Job jobs[], int n) {
    int ct[MAX_JOBS], finished[MAX_JOBS] = {0};
    int done = 0, time = 0;

    if (n <= 0 || n > MAX_JOBS) {
        printf("Priority: invalid number of jobs.\n");
        return;
    }

    printf("\nNon-preemptive Priority Scheduling\n");
    printf("Tie-break: lower priority number first, then earlier arrival, then lower Job ID\n");
    printf("Execution Order: ");

    while (done < n) {
        int best = -1, earliest = -1;

        for (int i = 0; i < n; i++) {
            if (finished[i]) continue;
            if (earliest == -1 || jobs[i].arrival < jobs[earliest].arrival)
                earliest = i;
            if (jobs[i].arrival > time) continue;      /* not arrived yet */

            if (best == -1 ||
                jobs[i].priority < jobs[best].priority ||
                (jobs[i].priority == jobs[best].priority &&
                 (jobs[i].arrival < jobs[best].arrival ||
                  (jobs[i].arrival == jobs[best].arrival && jobs[i].id < jobs[best].id))))
                best = i;
        }

        if (best == -1) {                              /* CPU idle */
            time = jobs[earliest].arrival;
            continue;
        }

        printf("J%d(%d-%d) ", jobs[best].id, time, time + jobs[best].burst);
        time += jobs[best].burst;
        ct[best] = time;
        finished[best] = 1;
        done++;
    }
    printf("\n");

    print_results(jobs, n, ct, 1);
}
