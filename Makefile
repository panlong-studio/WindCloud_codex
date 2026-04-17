CC := gcc
CFLAGS := -Wall -Wextra -g -Iinclude

COMMON_SRCS := \
	src/common/config.c \
	src/common/log.c \
	src/common/protocol.c \
	src/common/utils.c

SERVER_SRCS := \
	src/server/main.c \
	src/server/socket_init.c \
	src/server/queue.c \
	src/server/thread_pool.c \
	src/server/worker.c \
	src/server/session.c \
	src/server/handle.c \
	src/server/file_transfer.c

CLIENT_SRCS := \
	src/client/main.c \
	src/client/client.c \
	src/client/command.c \
	src/client/file_transfer.c

COMMON_OBJS := $(patsubst src/%.c,build/%.o,$(COMMON_SRCS))
SERVER_OBJS := $(patsubst src/%.c,build/%.o,$(SERVER_SRCS))
CLIENT_OBJS := $(patsubst src/%.c,build/%.o,$(CLIENT_SRCS))

.PHONY: all server client clean rebuild

all: server client

server: bin/mycloud_server

client: bin/mycloud_client

bin/mycloud_server: $(COMMON_OBJS) $(SERVER_OBJS)
	mkdir -p bin
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

bin/mycloud_client: $(COMMON_OBJS) $(CLIENT_OBJS)
	mkdir -p bin
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

build/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
	rm -f bin/mycloud_server bin/mycloud_client

rebuild: clean all
