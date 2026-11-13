#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "jobs.h"

const char *status_name(JobStatus s) {
    switch (s) {
        case JOB_WAITING:   return "Waiting";
        case JOB_RUNNING:   return "Running";
        case JOB_COMPLETED: return "Completed";
    }
    return "Unknown";
}

/* Reads one integer >= min. Re-prompts on bad input. Returns -1 on EOF. */
static int ask_int(const char *prompt, int min, int *out) {
    char buf[64], *end;
    for (;;) {
        printf("%s", prompt);
        if (!fgets(buf, sizeof buf, stdin)) return -1;
        errno = 0;
        long v = strtol(buf, &end, 10);
        /* reject empty input, trailing junk, overflow, and values below min */
        if (end == buf || (*end != '\n' && *end != '\0') || errno != 0 || v < min || v > 1000000) {
            printf("Invalid input: enter a whole number >= %d.\n", min);
            continue;
        }
        *out = (int)v;
        return 0;
    }
}

static int id_exists(const Job jobs[], int count, int id) {
    for (int i = 0; i < count; i++)
        if (jobs[i].id == id) return 1;
    return 0;
}

int input_jobs(Job jobs[], int *n) {
    int count;
    char prompt[64];

    if (ask_int("Enter number of jobs: ", 1, &count) < 0) return -1;
    if (count > MAX_JOBS) {
        printf("Too many jobs (maximum %d).\n", MAX_JOBS);
        return -1;
    }

    for (int i = 0; i < count; i++) {
        printf("\nEnter details for Job %d\n", i + 1);

        for (;;) {   /* job IDs must be positive and unique */
            if (ask_int("Job ID: ", 1, &jobs[i].id) < 0) return -1;
            if (!id_exists(jobs, i, jobs[i].id)) break;
            printf("Invalid input: Job ID %d already exists.\n", jobs[i].id);
        }
        if (ask_int("Arrival Time: ", 0, &jobs[i].arrival)  < 0) return -1;
        if (ask_int("Burst Time: ",   1, &jobs[i].burst)    < 0) return -1;
        snprintf(prompt, sizeof prompt, "Priority: ");
        if (ask_int(prompt,           1, &jobs[i].priority) < 0) return -1;

        jobs[i].start_time = -1;
        jobs[i].completion_time = -1;
        jobs[i].status = JOB_WAITING;   /* every new job starts as Waiting */
    }
    *n = count;
    return 0;
}

void print_jobs(const Job jobs[], int n) {
    printf("\nJob Details\n");
    printf("----------------------------------------------\n");
    printf("%-7s%-8s%-8s%-10s%s\n", "ID", "Arrival", "Burst", "Priority", "Status");
    for (int i = 0; i < n; i++)
        printf("%-7d%-8d%-8d%-10d%s\n", jobs[i].id, jobs[i].arrival,
               jobs[i].burst, jobs[i].priority, status_name(jobs[i].status));
}

void print_job_results(const Job jobs[], int n) {
    printf("\nFinal Job Table\n");
    printf("----------------------------------------------------------\n");
    printf("%-6s%-9s%-7s%-7s%-8s%-6s%s\n", "ID", "Arrival", "Burst", "Prio", "Start", "End", "Status");
    for (int i = 0; i < n; i++)
        printf("%-6d%-9d%-7d%-7d%-8d%-6d%s\n", jobs[i].id, jobs[i].arrival, jobs[i].burst,
               jobs[i].priority, jobs[i].start_time, jobs[i].completion_time,
               status_name(jobs[i].status));
}

/* Runs jobs in arrival order (FCFS) and records each status change. */
void run_lifecycle(Job jobs[], int n) {
    int order[MAX_JOBS], time = 0;

    /* stable insertion sort of indices by arrival time (input order breaks ties) */
    for (int i = 0; i < n; i++) {
        int j = i;
        while (j > 0 && jobs[order[j - 1]].arrival > jobs[i].arrival) {
            order[j] = order[j - 1];
            j--;
        }
        order[j] = i;
    }

    printf("\nLifecycle Trace\n");
    for (int k = 0; k < n; k++) {
        Job *j = &jobs[order[k]];
        if (time < j->arrival) time = j->arrival;   /* CPU idle until arrival */

        j->status = JOB_RUNNING;
        j->start_time = time;
        printf("Time %2d: J%d  Waiting -> Running\n", time, j->id);

        time += j->burst;
        j->completion_time = time;
        j->status = JOB_COMPLETED;
        printf("Time %2d: J%d  Running -> Completed\n", time, j->id);
    }
}
