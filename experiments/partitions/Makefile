MAKEFLAGS += --no-builtin-rules
.SUFFIXES:
.PHONY: clean

all: plots.pdf

include ../common.mk

clean:
	rm -rf config outputs *.dat *.aux *.log plots_fragment.tex plots.pdf

config: config.json mkconfig.pl
	rm -rf config
	mkdir config
	./mkconfig.pl config.json ./config/

outputs: config
	mkdir -p outputs
	$(MAKE) $(patsubst ./config/%.json,./outputs/%/stats.json,$(wildcard ./config/*.json))
	touch $@

# This rule also generates the .dat files
plots_fragment.tex: outputs make_dat_and_plots.pl
	./make_dat_and_plots.pl $<

plots.pdf: plots.tex plots_fragment.tex
	xelatex -halt-on-error -interaction=nonstopmode -shell-escape $<
	rm -f plots.aux plots.log plots_fragment.tex.aux
