#include <stdio.h>
#include "../include/bankers.h"

int bankers_safety(
    int n,
    int m,
    int available[MAX_RESOURCES],
    int allocation[MAX_PROCESSES][MAX_RESOURCES],
    int maximum[MAX_PROCESSES][MAX_RESOURCES])
{
    int need[MAX_PROCESSES][MAX_RESOURCES];
    int work[MAX_RESOURCES];
    int finish[MAX_PROCESSES] = {0};
    int safe_sequence[MAX_PROCESSES];

    int count = 0;

    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < m; j++)
        {
            need[i][j] = maximum[i][j] - allocation[i][j];
        }
    }

    printf("\nNeed Matrix\n");
    printf("Process\t");

    for (int j = 0; j < m; j++)
        printf("%c\t", 'A' + j);

    printf("\n");

    for (int i = 0; i < n; i++)
    {
        printf("P%d\t", i);

        for (int j = 0; j < m; j++)
            printf("%d\t", need[i][j]);

        printf("\n");
    }

    for (int j = 0; j < m; j++)
        work[j] = available[j];

    while (count < n)
    {
        int found = 0;

        for (int i = 0; i < n; i++)
        {
            if (finish[i] == 0)
            {
                int possible = 1;

                for (int j = 0; j < m; j++)
                {
                    if (need[i][j] > work[j])
                    {
                        possible = 0;
                        break;
                    }
                }

                if (possible)
                {
                    for (int j = 0; j < m; j++)
                        work[j] += allocation[i][j];

                    safe_sequence[count] = i;
                    count++;

                    finish[i] = 1;
                    found = 1;
                }
            }
        }

        if (found == 0)
        {
            printf("\nSystem is in an UNSAFE state.\n");
            return 0;
        }
    }

    printf("\nSystem is in a SAFE state.\n");

    printf("Safe Sequence: ");

    for (int i = 0; i < n; i++)
    {
        printf("P%d", safe_sequence[i]);

        if (i < n - 1)
            printf(" -> ");
    }

    printf("\n");

    return 1;
}

