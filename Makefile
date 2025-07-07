CC = clang

# Common flags (shared)
COMMON_FLAGS = -Wall -Wextra -std=c17 -g \
               -D__BLOCKS__ -D__DARWIN_UNIX03 \
               -isysroot $(shell xcrun --sdk macosx --show-sdk-path)

# C-only flags
CFLAGS = $(COMMON_FLAGS)

# ObjC flags (enable blocks, use ObjC parser)
OBJCFLAGS = $(COMMON_FLAGS) -fblocks -x objective-c

# Include directories
INCLUDES = \
    -I. \
    -I./include \
    -I./include/sys \
    -I./include/IO \
    -I./sysheaders \
    -I./include/appleExcl \
    -I./include/appleExcl/Obj-C \
    -I./src

# Frameworks to link
LDFLAGS = -framework CoreFoundation -framework IOKit

# C source files
SRC_C = main.c                             \
        src/framebuffer.c                  \
        src/render.c                       \
        src/armOpCodes.c                   \
        src/sysprobe.c                     \
        src/macho_probe.c                  \
        src/window_probe.c                 \
        src/graphics_bruteforce.c

# Objective-C source files
SRC_OBJC = include/io/io.c \
           include/appleExcl/Obj-C/some_objc_header_user.c

# Object files
OBJ_C    := $(SRC_C:.c=.o)
OBJ_OBJC := $(SRC_OBJC:.c=.o)
OBJ      := $(OBJ_C) $(OBJ_OBJC)

TARGET = my_program

# Default build rule
all: $(TARGET)

# Rule for C sources
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Rule for known Objective-C sources (explicitly listed)
$(OBJ_OBJC): %.o: %.c
	$(CC) $(OBJCFLAGS) $(INCLUDES) -c $< -o $@

# Linking rule
$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
