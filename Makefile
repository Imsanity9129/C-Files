CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Iinclude
TARGET = c-files
SOURCES = src/main.c src/file_ops.c src/nav.c src/search.c src/sort.c
OBJECTS = src/main.o src/file_ops.o src/nav.o src/search.o src/sort.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

src/main.o: src/main.c include/models.h include/file_ops.h include/search.h include/sort.h
	$(CC) $(CFLAGS) -c src/main.c -o src/main.o

src/file_ops.o: src/file_ops.c include/file_ops.h include/models.h
	$(CC) $(CFLAGS) -c src/file_ops.c -o src/file_ops.o

src/nav.o: src/nav.c include/nav.h include/models.h
	$(CC) $(CFLAGS) -c src/nav.c -o src/nav.o

src/search.o: src/search.c include/search.h
	$(CC) $(CFLAGS) -c src/search.c -o src/search.o

src/sort.o: src/sort.c include/sort.h include/models.h
	$(CC) $(CFLAGS) -c src/sort.c -o src/sort.o

clean:
	rm -f $(TARGET) $(OBJECTS)
