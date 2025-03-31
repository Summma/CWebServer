CC = cc
CFLAGS = -Wall -Wextra -O2

SRC = src/main.c src/server.c

OUTPUT = server

all: $(OUTPUT)

$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUTPUT)

clean:
	rm -f $(OUTPUT)

rebuild: clean all

help:
	@echo "Makefile for building the web server."
	@echo "Usage:"
	@echo "  make       - Compile the server"
	@echo "  make clean - Remove compiled binary"
	@echo "  make rebuild - Clean and recompile"
	@echo "  make help  - Display this help message"
