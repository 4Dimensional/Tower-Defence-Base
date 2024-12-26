# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -g

# SDL2 Flags
SDL2_CFLAGS = $(shell sdl2-config --cflags)
SDL2_LDFLAGS = $(shell sdl2-config --libs)

# SDL2_image Flags
SDL2_IMAGE_CFLAGS = -I/usr/include/SDL2  # Adjust this path if SDL_image's headers are elsewhere
SDL2_IMAGE_LDFLAGS = -lSDL2_image        # Link SDL2_image library

# Target executable
TARGET = main

# Source files
SRC = main.c

# Object files
OBJ = $(SRC:.c=.o)

# Build target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(SDL2_CFLAGS) $(SDL2_LDFLAGS) $(SDL2_IMAGE_LDFLAGS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) $(SDL2_IMAGE_CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJ) $(TARGET)
