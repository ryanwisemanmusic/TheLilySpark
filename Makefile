CC = clang
CFLAGS = -Wall -Wextra -std=c17 -g -I./include \
         -I./sysheaders -I./src 

SRC = main.c src/framebuffer.c src/render.c src/armOpCodes.c
OBJ = $(SRC:.c=.o)
TARGET = my_program

all: $(TARGET)

$(OBJ): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
