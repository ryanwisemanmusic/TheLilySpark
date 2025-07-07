CC = clang
CFLAGS = -Wall -Wextra -std=gnu99 -g -D__BLOCKS__=1 -D__ASSUME_PTR_ABI_SINGLE_BEGIN= -D__ASSUME_PTR_ABI_SINGLE_END= \
         -I. -I./include -I./include/sys -I./include/IO -I./sysheaders -I./src

# Add framework linking for CoreFoundation and IOKit
LDFLAGS = -framework CoreFoundation -framework IOKit

SRC = main.c src/framebuffer.c src/render.c src/armOpCodes.c src/sysprobe.c src/macho_probe.c src/window_probe.c src/graphics_bruteforce.c
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