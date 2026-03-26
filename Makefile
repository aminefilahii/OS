CC = gcc
CFLAGS = -Wall
LDFLAGS = -lreadline
TARGET = biceps
SRC = biceps.c
all: $(TARGET)
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
clean:
	rm -f $(TARGET)
