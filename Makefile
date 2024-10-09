# Variables
CC = clang                         # The compiler to use
CFLAGS = -Wall -Wextra -g          # Compiler flags: warnings and debugging
FRAMEWORKS = -F/Library/Frameworks # Frameworks path for macOS
LDFLAGS = -framework SDL2 -framework SDL2_ttf -Wl,-rpath,/Library/Frameworks  # Linker flags for SDL2 and SDL2_ttf frameworks

# The target executable name
TARGET = pong

# The source files
SRC = pong.c

# The object files generated from the source files
OBJ = $(SRC:.c=.o)

# Default target to build the pong game
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(FRAMEWORKS) $(LDFLAGS)

# Compile the source file into object file
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(FRAMEWORKS)

# Run the game
run: $(TARGET)
	./$(TARGET)

# Clean up build files
clean:
	rm -f $(OBJ) $(TARGET)

# Convenience target for rebuilding the project from scratch
rebuild: clean all