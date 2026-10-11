PYTHON ?= python3
SOURCES := $(wildcard src/*.ASC)

.PHONY: programs
programs:
	$(PYTHON) tools/ascii85p.py $(SOURCES) --output-dir programs
