CC = gcc
LIBS = -ladvapi32 -lpsapi
TARGET = enumerator.exe
SRC = src/main.c

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(LIBS)

clean:
	del $(TARGET)