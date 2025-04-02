CC = clang
CFLAGS = -Wall -Wextra -std=c17 -g \
         -I. \
         -I./include \
         -I./include/sys \
         -I./include/IO \
         -I./sysheaders \
         -I./src

# Add framework linking for CoreFoundation and IOKit
LDFLAGS = -framework CoreFoundation -framework IOKit

SRC = main.c src/framebuffer.c src/render.c src/armOpCodes.c include/io/io.c 
OBJ = $(SRC:.c=.o)
TARGET = my_program

all: $(TARGET)

$(OBJ): %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
