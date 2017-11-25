MAKEFLAGS += --no-builtin-rules
.SUFFIXES:
.PHONY: clean outputs

all: outputs

include ../common.mk

clean:
	rm -rf config
	rm -rf outputs

config: config.json mkconfig.pl
	rm -rf config
	mkdir config
	./mkconfig.pl config.json ./config/

outputs: config
	mkdir -p outputs
	#$(MAKE) $(patsubst config/%.json,outputs/%.gr/stats.json,$(wildcard config/*.json)) \
	#        $(patsubst config/%.json,outputs/%.grv/stats.json,$(wildcard config/*.json))
	$(MAKE) $(patsubst config/%.json,outputs/%.grv/stats.json,$(wildcard config/*.json))
