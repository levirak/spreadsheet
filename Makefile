PROG_NAME=      spreadsheet
PROG_DIR=       build

MAIN=           src/main.c

CFLAGS =        -g -Wall -Wextra -Isrc $(OPTFLAGS) # -fsanitize=address,undefined
LDFLAGS=        
LDLIBS=         $(OBJECTS)

SOURCES=        $(filter-out $(MAIN),$(wildcard src/**/*.c src/*.c))
OBJECTS=        $(patsubst %.c,%.o,$(SOURCES))

TEST_SRC=       $(wildcard tests/*_tests.c)
TESTS=          $(patsubst %.c,%,$(TEST_SRC))

TARGET=         $(PROG_DIR)/$(PROG_NAME)


# The Target Build
all: $(TARGET) tests

release: CFLAGS= -O2 -Wall -Wextra -Isrc -DNDEBUG $(OPTFLAGS)
release: all

$(TARGET): $(MAIN) $(OBJECTS) $(PROG_DIR)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

$(PROG_DIR):
	@mkdir -p $(PROG_DIR)
	@mkdir -p bin

.PHONY: tests
tests: $(TESTS)
	@sh ./tests/runtests.sh

.PHONY: clean
clean:
	rm -rf $(PROG_DIR) $(OBJECTS) $(TESTS)

.PHONY: tags
tags:
	ctags -R src
