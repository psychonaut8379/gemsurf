# --- Configuration ---
DEBUG ?= 0

# --- Paths ---
HOMEDIR = $(HOME)

ifeq ($(DEBUG), 1) 
	PREFIX = $(shell echo $$PWD)
endif

PREFIX ?= $(HOMEDIR)/.local
DATADIR = $(PREFIX)/share/gemsurf
BINDIR = $(PREFIX)/bin

# --- Compiler Flags and Libraries
CC = gcc
CFLAGS = -Wall -Wextra
CFLAGS += -DDATADIR=\"$(DATADIR)\"
LIBS = -lssl -lcrypto -lncurses

ifeq ($(DEBUG), 1)
	CFLAGS += -g -fsanitize=address
endif

TARGET = gemsurf
SRCS = main.c network.c parser.c ui.c utils.c
OBJS = $(SRCS:.c=.o)

# --- Rules ---

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)

install: all
	mkdir -p $(DATADIR)
	mkdir -p $(BINDIR)
	cp $(TARGET) $(BINDIR)

uninstall: 
	rm -rf $(DATADIR)
	rm -f $(BINDIR)/$(TARGET)

run: install
	$(BINDIR)/gemsurf

.PHONY: all clean install uninstall run