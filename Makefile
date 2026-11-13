CC      = gcc
CFLAGS  = -Wall -Wextra -std=gnu11 -O2 -pthread -Iinclude
TARGET  = campuscompute
SRCS    = main.c src/jobs.c src/scheduling.c src/bankers.c src/worker.c src/member2.c
HDRS    = $(wildcard include/*.h)

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lrt

# Removes only files created by this program.
clean:
	rm -f $(TARGET) campuscompute.log
