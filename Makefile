REVISION = $(shell git rev-list -n 1 --abbrev-commit HEAD)
REVISION += $(shell git diff-index --quiet HEAD -- || echo "+ uncommitted changes")

ifeq ($(origin CC), default)
  REAL_CC := $(CC)
  CC = rootsim-cc
endif

CFLAGS += -ggdb -g3
#CFLAGS += -O3
CFLAGS += -std=gnu11 -Wall -Wextra
CFLAGS += -Wwrite-strings -Winit-self -Wcast-align -Wcast-qual -Wpointer-arith
CFLAGS += -Wconversion -Wformat=2 -Wlogical-op -Wshadow
CFLAGS += -Wstrict-prototypes -Wmissing-declarations -Wmissing-prototypes
#CFLAGS += -Wno-unused-variable -Wno-unused-parameter -Wno-unused
CFLAGS += -Isrc -DREVISION="$(REVISION)"

# Json-c library
CFLAGS  += $(shell pkg-config --cflags json-c)
LDFLAGS += $(shell pkg-config --libs json-c)

SRC = $(shell find src -iname '*.c')
HEADERS = $(shell find src -iname '*.h')
OBJ = $(SRC:.c=.o)

RUN_PREFIX = time

PYTHON-VENV = .python-virtualenv
SPHINXBUILD = $(shell pwd)/.python-virtualenv/bin/sphinx-build

.PHONY: all asan tsan ubsan clean run gdb sequential record replay doc

all: application tags

# When linking, rootsim-cc runs "rm *.o" and also removes its input files if
# they match *.o which wreaks havoc in our build tree. So we run it in
# another directory with a copy of the object files.
application: $(OBJ)
	( \
	    mkdir -p rootsim-cc-temp; \
	    cp --parents $^ rootsim-cc-temp; \
	    cd rootsim-cc-temp; \
	    $(CC) $(CFLAGS) -o ../$@ $^ $(LDFLAGS); \
	)
	rm -rf rootsim-cc-temp

gprof: CFLAGS += -pg
gprof: LDFLAGS += -pg
gprof: clean all

asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer -O1 -fno-optimize-sibling-calls
asan: clean all

tsan: CFLAGS += -fsanitize=thread -fPIE -pie
tsan: clean all

ubsan: CFLAGS += -fsanitize=undefined
ubsan: clean all

-include $(OBJ:.o=.d)

%.o: %.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	$(REAL_CC) -MT $*.o -MM $(CFLAGS) $*.c > $*.d

tags: $(SRC) $(HEADERS)
	-which ctags && ctags --fields=+l --c-kinds=+p -f $@ $^

clean:
	-find src '(' -iname '*.o' -or -iname '*.d' ')' -exec rm -v '{}' ';'
	-rm -f application
	-rm -rf outputs/

run: application
	$(RUN_PREFIX) ./application $(RUN_ARGS) -- --np 1 $(ROOTSIM_ARGS)

gdb: RUN_PREFIX = gdb --args
gdb: run

sequential: ROOTSIM_ARGS += --sequential
sequential: run

valgrind: RUN_PREFIX = valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --
valgrind: run

valgrind-gdb: RUN_PREFIX = valgrind --vgdb-error=0 --leak-check=full --show-leak-kinds=all --track-origins=yes --
valgrind-gdb: run

callgrind: RUN_PREFIX = valgrind --tool=callgrind --
callgrind: run

record: ROOTSIM_ARGS += --sequential
record: RUN_PREFIX = rr record
record: run

replay:
	rr replay

$(PYTHON-VENV):
	virtualenv --python="$$(which python3)" "$@"

$(SPHINXBUILD): $(PYTHON-VENV)
	$(PYTHON-VENV)/bin/pip install Sphinx sphinxcontrib-cmtinc
	touch "$@"

doc: $(SPHINXBUILD)
	$(MAKE) -C doc SPHINXBUILD="$(SPHINXBUILD)" html
	@echo "Documentation available at file://$$(readlink -f doc/_build/html/index.html)"
