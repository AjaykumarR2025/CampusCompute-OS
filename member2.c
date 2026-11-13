#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define QUEUE_CAPACITY 5
#define LOG_FILE "campuscompute.log"

typedef struct {
    int values[QUEUE_CAPACITY];
    int front, rear, count;
    int total_items;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} bounded_queue;

static bounded_queue queue_data;
static FILE *log_stream;

static void usage(const char *program) {
    printf("CampusCompute - Member 2 modules\n");
    printf("Usage: %s <command> [arguments]\n\n", program);
    printf("Commands:\n");
    printf("  logger [count]  Run synchronized producer-consumer logger (default 10)\n");
    printf("  ipc             Exchange a message using POSIX shared memory + named semaphore\n");
    printf("  limits          Demonstrate a safe RLIMIT_NOFILE process limit\n");
    printf("  all             Run logger, IPC, and resource-limit demonstrations\n");
    printf("  help            Display this help message\n");
}

static int parse_count(const char *text, int *value) {
    char *end = NULL;
    errno = 0;
    long n = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || n < 1 || n > 1000)
        return -1;
    *value = (int)n;
    return 0;
}

static void log_event(const char *role, int item) {
    printf("[%s] processed event %d\n", role, item);
    if (log_stream != NULL) {
        fprintf(log_stream, "[%s] processed event %d\n", role, item);
        fflush(log_stream);
    }
}

static void pause_ms(long milliseconds) {
    struct timespec delay;
    delay.tv_sec = milliseconds / 1000;
    delay.tv_nsec = (milliseconds % 1000) * 1000000L;
    nanosleep(&delay, NULL);
}

static void *producer_thread(void *arg) {
    int count = *(int *)arg;
    for (int item = 1; item <= count; ++item) {
        pthread_mutex_lock(&queue_data.mutex);
        while (queue_data.count == QUEUE_CAPACITY)
            pthread_cond_wait(&queue_data.not_full, &queue_data.mutex);

        queue_data.values[queue_data.rear] = item;
        queue_data.rear = (queue_data.rear + 1) % QUEUE_CAPACITY;
        queue_data.count++;
        printf("[Producer] added event %d (queue=%d)\n", item, queue_data.count);

        pthread_cond_signal(&queue_data.not_empty);
        pthread_mutex_unlock(&queue_data.mutex);
        pause_ms(30);
    }
    return NULL;
}

static void *consumer_thread(void *arg) {
    (void)arg;
    for (;;) {
        pthread_mutex_lock(&queue_data.mutex);
        while (queue_data.count == 0 && queue_data.total_items > 0)
            pthread_cond_wait(&queue_data.not_empty, &queue_data.mutex);

        if (queue_data.total_items == 0) {
            pthread_mutex_unlock(&queue_data.mutex);
            break;
        }

        int item = queue_data.values[queue_data.front];
        queue_data.front = (queue_data.front + 1) % QUEUE_CAPACITY;
        queue_data.count--;
        queue_data.total_items--;

        pthread_cond_signal(&queue_data.not_full);
        pthread_mutex_unlock(&queue_data.mutex);

        log_event("Consumer", item);
        pause_ms(50);
    }
    return NULL;
}

static int run_logger(int count) {
    pthread_t producer, consumer;
    memset(&queue_data, 0, sizeof(queue_data));
    queue_data.total_items = count;

    if (pthread_mutex_init(&queue_data.mutex, NULL) != 0 ||
        pthread_cond_init(&queue_data.not_empty, NULL) != 0 ||
        pthread_cond_init(&queue_data.not_full, NULL) != 0) {
        fprintf(stderr, "Error: could not initialize thread synchronization objects.\n");
        return 1;
    }

    log_stream = fopen(LOG_FILE, "a");
    if (!log_stream) {
        perror("fopen log file");
        pthread_cond_destroy(&queue_data.not_full);
        pthread_cond_destroy(&queue_data.not_empty);
        pthread_mutex_destroy(&queue_data.mutex);
        return 1;
    }

    fprintf(log_stream, "\n--- Logger session: %d events ---\n", count);
    printf("Starting producer-consumer logger with queue capacity %d...\n", QUEUE_CAPACITY);

    int p = pthread_create(&producer, NULL, producer_thread, &count);
    int c = pthread_create(&consumer, NULL, consumer_thread, NULL);
    if (p != 0 || c != 0) {
        fprintf(stderr, "Error: pthread_create failed.\n");
        /* Ensure a successfully created consumer can exit. */
        pthread_mutex_lock(&queue_data.mutex);
        queue_data.total_items = 0;
        pthread_cond_broadcast(&queue_data.not_empty);
        pthread_cond_broadcast(&queue_data.not_full);
        pthread_mutex_unlock(&queue_data.mutex);
        if (p == 0) pthread_join(producer, NULL);
        if (c == 0) pthread_join(consumer, NULL);
        fclose(log_stream);
        log_stream = NULL;
        pthread_cond_destroy(&queue_data.not_full);
        pthread_cond_destroy(&queue_data.not_empty);
        pthread_mutex_destroy(&queue_data.mutex);
        return 1;
    }

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);
    fclose(log_stream);
    log_stream = NULL;

    pthread_cond_destroy(&queue_data.not_full);
    pthread_cond_destroy(&queue_data.not_empty);
    pthread_mutex_destroy(&queue_data.mutex);
    printf("Logger finished. Events were appended to %s\n", LOG_FILE);
    return 0;
}

typedef struct {
    char message[256];
    int ready;
} shared_data;

static int run_ipc(void) {
    char shm_name[80], sem_name[80];
    snprintf(shm_name, sizeof(shm_name), "/campuscompute_shm_%ld", (long)getpid());
    snprintf(sem_name, sizeof(sem_name), "/campuscompute_sem_%ld", (long)getpid());

    int fd = shm_open(shm_name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(fd, sizeof(shared_data)) == -1) {
        perror("ftruncate");
        close(fd);
        shm_unlink(shm_name);
        return 1;
    }

    shared_data *shared = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE,
                               MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        close(fd);
        shm_unlink(shm_name);
        return 1;
    }
    memset(shared, 0, sizeof(*shared));

    sem_t *sync_sem = sem_open(sem_name, O_CREAT | O_EXCL, 0600, 0);
    if (sync_sem == SEM_FAILED) {
        perror("sem_open");
        munmap(shared, sizeof(*shared));
        close(fd);
        shm_unlink(shm_name);
        return 1;
    }

    printf("IPC: parent created shared memory and named semaphore.\n");
    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        sem_close(sync_sem);
        sem_unlink(sem_name);
        munmap(shared, sizeof(*shared));
        close(fd);
        shm_unlink(shm_name);
        return 1;
    }

    if (child == 0) {
        snprintf(shared->message, sizeof(shared->message),
                 "Hello from child process %ld through POSIX shared memory.",
                 (long)getpid());
        shared->ready = 1;
        if (sem_post(sync_sem) == -1) {
            perror("child sem_post");
            _exit(2);
        }
        _exit(0);
    }

    if (sem_wait(sync_sem) == -1) {
        perror("sem_wait");
        kill(child, SIGTERM);
    } else if (shared->ready) {
        printf("Parent received: %s\n", shared->message);
    } else {
        fprintf(stderr, "Error: shared-memory message was not marked ready.\n");
    }

    int status = 0;
    if (waitpid(child, &status, 0) == -1) perror("waitpid");
    else if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        printf("Child process completed normally.\n");
    else
        printf("Child process did not complete normally.\n");

    munmap(shared, sizeof(*shared));
    close(fd);
    sem_close(sync_sem);
    shm_unlink(shm_name);
    sem_unlink(sem_name);
    printf("IPC cleanup: shared memory and named semaphore released.\n");
    return 0;
}

static int run_limits(void) {
    struct rlimit old_limit, new_limit;
    if (getrlimit(RLIMIT_NOFILE, &old_limit) == -1) {
        perror("getrlimit");
        return 1;
    }

    printf("RLIMIT_NOFILE before: soft=%ld, hard=%ld\n",
           (long)old_limit.rlim_cur, (long)old_limit.rlim_max);

    new_limit = old_limit;
    rlim_t target = 64;
    if (new_limit.rlim_max != RLIM_INFINITY && target > new_limit.rlim_max)
        target = new_limit.rlim_max;
    if (new_limit.rlim_cur > target)
        new_limit.rlim_cur = target;

    if (setrlimit(RLIMIT_NOFILE, &new_limit) == -1) {
        perror("setrlimit");
        return 1;
    }
    if (getrlimit(RLIMIT_NOFILE, &new_limit) == -1) {
        perror("getrlimit after setrlimit");
        return 1;
    }

    printf("RLIMIT_NOFILE after : soft=%ld, hard=%ld\n",
           (long)new_limit.rlim_cur, (long)new_limit.rlim_max);
    printf("Effect: this process can now keep at most %ld file descriptors open.\n",
           (long)new_limit.rlim_cur);
    printf("The limit applies to this process and its children; it does not change system-wide limits.\n");
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0) {
        usage(argv[0]);
        return argc < 2 ? 1 : 0;
    }

    if (strcmp(argv[1], "logger") == 0) {
        int count = 10;
        if (argc > 3 || (argc == 3 && parse_count(argv[2], &count) != 0)) {
            fprintf(stderr, "Error: logger count must be an integer from 1 to 1000.\n");
            usage(argv[0]);
            return 2;
        }
        return run_logger(count);
    }
    if (strcmp(argv[1], "ipc") == 0) {
        if (argc != 2) {
            fprintf(stderr, "Error: ipc takes no arguments.\n");
            usage(argv[0]);
            return 2;
        }
        return run_ipc();
    }
    if (strcmp(argv[1], "limits") == 0) {
        if (argc != 2) {
            fprintf(stderr, "Error: limits takes no arguments.\n");
            usage(argv[0]);
            return 2;
        }
        return run_limits();
    }
    if (strcmp(argv[1], "all") == 0) {
        if (argc != 2) {
            fprintf(stderr, "Error: all takes no arguments.\n");
            usage(argv[0]);
            return 2;
        }
        int result = run_logger(8);
        if (result == 0) result = run_ipc();
        if (result == 0) result = run_limits();
        return result;
    }

    fprintf(stderr, "Error: unknown command '%s'.\n", argv[1]);
    usage(argv[0]);
    return 2;
}
