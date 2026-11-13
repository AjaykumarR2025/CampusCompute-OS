CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -pthread
TARGET = campuscompute_member2
SOURCE = member2.c

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) -pthread -lrt

clean:
	rm -f $(TARGET) campuscompute.log
