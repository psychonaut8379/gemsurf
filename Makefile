# --- Configuration ---
# PREFIX determines where the program is installed	
# Users can override this like: $ make install PREFIX=/usr

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DATADIR = $(PREFIX)/share/gemsurf

# --- Compiler and Flags ---
CFLAGS = -Wall -Wextra
LIBS = -ls2n -lncurses

# --- Files ---
TARGET = gemsurf
SRC = main.c network.c parser.c utils.c ui.c
OBJ = $(SRC:.c=.o)

# --- Rules ---
all: $(TARGET)

$(TARGET): $(OBJ)
	cc $(CFLAGS) -o $(TARGET) $(OBJ) $(CFLAGS) $(LIBS)

%.o: %.c
	cc $(CFLAGS) -c $< -o $@

run:
	./gemsurf

.PHONY: all run