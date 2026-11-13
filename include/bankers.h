#ifndef BANKERS_H
#define BANKERS_H

#define MAX_PROCESSES 10
#define MAX_RESOURCES 10

int bankers_safety(
    int n,
    int m,
    int available[MAX_RESOURCES],
    int allocation[MAX_PROCESSES][MAX_RESOURCES],
    int maximum[MAX_PROCESSES][MAX_RESOURCES]
);

#endif
