BASE_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

RUN_ARGS = --stop-after=0.2
ROOTSIM_ARGS = --np 1 --sequential
APPLICATION = $(BASE_DIR)bin/application
STATS = $(BASE_DIR)stats.pl

.PRECIOUS: outputs/%

.PHONY: run-all

outputs/%: config/%.json $(APPLICATION)
	mkdir -p outputs
	rm -rf $@
	mkdir "$@"
	$(APPLICATION) --config "$<" $(RUN_ARGS) --output-dir "$@" -- --output-dir "$@/outputs" $(ROOTSIM_ARGS) > "$@"/log 2>&1 || (r="$$?"; touch -t 01010000 "$@"; exit $$r)

outputs/%/stats.json: outputs/% $(STATS)
	$(STATS) "$<" > "$@"

ALL_CONFIGS = $(wildcard config/*.json)
ALL_OUTPUTS = $(subst .json,/stats.json,$(addprefix outputs/,$(notdir $(ALL_CONFIGS))))

run-all: $(ALL_OUTPUTS)
