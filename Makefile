program_name = spreadsheet
source_dir   = src
build_dir    = build

o = .o
d = .mk
e =

CPPDIRS = -Isrc
LDDIRS  =
LDLIBS  = -pthread -lm

OPTFLAGS          := $(CFLAGS)
override CPPFLAGS := -g -std=gnu11 $(CPPFLAGS) $(CPPDIRS)
override CFLAGS   := -Wall -Wextra -Wpedantic $(CFLAGS) #-fsanitize=address,undefined
override LDFLAGS  := $(LDFLAGS) $(LDDIRS)

sources      = $(wildcard $(source_dir)/*.c)
objects      = $(patsubst $(source_dir)/%.c,$(build_dir)/%$o,$(sources))
deps         = $(patsubst $(source_dir)/%.c,$(build_dir)/%$d,$(sources))
target       = $(build_dir)/$(program_name)$e

# The Target Build
all: $(target)

release: CFLAGS := -O2 -Wall -Wextra -DNDEBUG $(OPTFLAGS)
release: $(target)

-include $(deps)
$(deps): $(build_dir)/%$d: $(source_dir)/%.c | $(build_dir)
	@echo "  GEN   $@"
	@$(CPP) $(CPPFLAGS) $< -MM -MT $(@:$d=$o) >$@

$(target): $(objects) | $(build_dir)
	@echo "  CC    $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(objects): %$o:
	@echo "  CC    $@"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $< $(LDFLAGS) $(LDLIBS)

$(build_dir):
	@echo "  MKDIR $@"
	@mkdir -p $@

.PHONY: clean
clean:
	@echo "  RM    $(build_dir)"
	@rm -rf $(build_dir)
	@echo "  RM    tags"
	@rm -f tags

.PHONY: tags
tags:
	@echo "  CTAGS $(source_dir)"
	@ctags -R $(source_dir)
