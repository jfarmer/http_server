BUILD_DIR := ./bin
PROGRAMS := echo_server_multi echo_server datetime_server showip hello

all: mk_build_dir $(PROGRAMS)

mk_build_dir:
	mkdir -p $(BUILD_DIR)

$(PROGRAMS):
	gcc -o $(BUILD_DIR)/$@ $@.c

clean:
	rm -f $(BUILD_DIR)/*
