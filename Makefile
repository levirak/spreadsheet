program_name = spreadsheet
build_dir    = build
source_dir   = src
test_dir     = tests
main         = main.c

CPPDIRS = -Isrc
LDDIRS  =
LDLIBS  =

OPTFLAGS := $(CFLAGS)
override CPPFLAGS := $(CPPFLAGS) $(CPPDIRS)
override CFLAGS   := -g -Wall -Wextra $(CFLAGS) # -fsanitize=address,undefined
override LDFLAGS  := $(LDFLAGS) $(LDDIRS)

sources      = $(wildcard $(source_dir)/**/*.c $(source_dir)/*.c)
objects      = $(sources:.c=.o)
test_sources = $(wildcard $(test_dir)/*_tests.c)
tests        = $(test_sources:.c=)
deps         = $(sources:.c=.mk)
test_deps    = $(test_sources:.c=.mk)
target       = $(build_dir)/$(program_name)


# The Target Build
all: $(target) tests

release: CFLAGS := -O2 -Wall -Wextra -Isrc -DNDEBUG $(OPTFLAGS)
release: $(target)

-include $(deps) $(test_deps)
$(deps): %.mk: %.c
	$(CPP) $(CPPFLAGS) $< -MM -MT $(@:.mk=.o) >$@
$(test_deps): %.mk: %.c
	$(CPP) $(CPPFLAGS) $< -MM -MT $(@:.mk=) >$@

$(target): $(objects) | $(build_dir)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(build_dir):
	@mkdir -p $@

.PHONY: tests
$(tests): %: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

tests: CFLAGS += $(CPPFLAGS)
tests: $(tests)
	@sh ./$(test_dir)/runtests.sh

.PHONY: clean
clean:
	rm -rf $(build_dir) $(objects) $(deps) $(tests)

.PHONY: tags
tags:
	ctags -R $(source_dir)
