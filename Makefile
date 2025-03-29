CC = clang
CFLAGS = -Wall -Wextra -std=c17 -g -I./include \
         -I./sysheaders -I./src 

SRC = main.c
OBJ = $(SRC:.c=.o)
TARGET = my_program

all: $(TARGET)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
