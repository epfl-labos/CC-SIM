MAKEFLAGS += --no-builtin-rules
.SUFFIXES:
.PHONY: clean outputs

all: outputs

include ../common.mk

clean:
	rm -rf config
	rm -rf outputs
	#rm -f errors.json
	mkdir config

CONFIGS = $(wildcard config/*.json)
OUTPUTS = $(patsubst config/%.json,outputs/%/stats.json,$(CONFIGS))

errors.json: compute_errors.pl $(OUTPUTS)
	./compute_errors.pl outputs/
