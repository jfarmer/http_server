CC := gcc
# CFLAGS := -g -O0 -fsanitize=leak
BIN_DIR := ./bin
BUILD_DIR := ./bin
PROGRAMS := strtok_test echo_server_multi echo_server datetime_server showip hello

all: mk_build_dir http_server $(PROGRAMS)

http_server: http.o http_server.o
	gcc $(BUILD_DIR)/http.o $(BUILD_DIR)/http_server.o -o $(BIN_DIR)/http_server

http_server.o:
	gcc -c http_server.c -o $(BUILD_DIR)/http_server.o

http.o:
	gcc -c http.c -o $(BUILD_DIR)/http.o

mk_build_dir:
	mkdir -p $(BIN_DIR)
	mkdir -p $(BUILD_DIR)

$(PROGRAMS):
	gcc -o $(BIN_DIR)/$@ $@.c

clean:
	rm -f $(BUILD_DIR)/*
	rm -f $(BIN_DIR)/*
