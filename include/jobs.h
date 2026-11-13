#ifndef JOBS_H
#define JOBS_H

#define MAX_JOBS 50

typedef enum { JOB_WAITING, JOB_RUNNING, JOB_COMPLETED } JobStatus;

typedef struct {
    int id;
    int arrival;
    int burst;
    int priority;          /* lower number = higher priority */
    int start_time;        /* set when the job starts running */
    int completion_time;   /* set when the job completes */
    JobStatus status;
} Job;

const char *status_name(JobStatus s);
int  input_jobs(Job jobs[], int *n);       /* returns 0 on success, -1 on EOF/error */
void print_jobs(const Job jobs[], int n);  /* ID, Arrival, Burst, Priority, Status */
void print_job_results(const Job jobs[], int n);
void run_lifecycle(Job jobs[], int n);     /* FCFS run that updates job status */

#endif
