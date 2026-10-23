
CC = gcc
CFLAGS = -D_GNU_SOURCE -Wall -Wextra -std=c11
LIBS = -pthread -lsqlite3
TARGET = simulator
SOURCE = server.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET) simulator.db
