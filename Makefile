TARGET=ipk-lookup

CC=gcc
CFLAGS=-Wall -lm

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c 

clean:
	rm -f *.o $(TARGET)
