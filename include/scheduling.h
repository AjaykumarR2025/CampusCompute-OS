#ifndef SCHEDULING_H
#define SCHEDULING_H

#include "jobs.h"

void fcfs_schedule(const Job jobs[], int n);
void round_robin_schedule(const Job jobs[], int n, int quantum);
void priority_schedule(const Job jobs[], int n);

#endif
