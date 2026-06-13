# ===================================================================
# Makefile for the Duris DB write unit test suite.
#
# Compiles test_db_write.c + test_main.c standalone — no MySQL or
# MUD dependencies needed.
#
# Usage:
#   make           — build test_db_write binary
#   make run       — build and run the full test suite
#   make test      — alias for 'make run'
#   make clean     — remove build artifacts
# ===================================================================

SHELL = /bin/sh
CC    = gcc
CFLAGS = -g -std=c11 -Wall -Wextra -pedantic \
         -I../../src \
         -DPRODUCTION_SOURCE_PATH=\"$(CURDIR)/../../src/sql_player.c\"

TARGET = test_db_write
SRCS   = test_db_write.c test_game_scenarios.c test_data_validation.c test_character_lifecycle.c test_crash_stress.c test_container_rescue.c test_transaction_rollback.c test_main.c
OBJS   = $(SRCS:.c=.o)

LIBS   =

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
	@echo "  Build complete: ./$@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	@echo "=== Running DB write unit tests ==="
	./$(TARGET)

test: run

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "  Clean complete"
