# Compiler and flags
CC       := gcc
CFLAGS   := -Wall -Wextra -O2 -pthread
LDFLAGS  := -pthread -lrt

# Source files
CLIENT_SRCS := client.c
SERVER_SRCS := server.c

# Headers
INCLUDES := -I.

# Targets
CLIENT_BIN := client
SERVER_BIN := server

.PHONY: all clean

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRCS) client_server.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_SRCS) client_server.h
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)

clean:
	@echo "🧹 Cleaning build artifacts…"
	rm -f $(CLIENT_BIN) $(SERVER_BIN)