# Root Makefile for Petri Net project
# Authors: Lukáš, Jan, Matěj

.PHONY: all clean run test pack doxygen

all:
	$(MAKE) -C src all

clean:
	$(MAKE) -C src clean
	rm -rf doc/html doc/latex

run:
	$(MAKE) -C src run

test:
	$(MAKE) -C src test

pack: clean
	zip -r xname01-xname02-xname03.zip src examples doc README.txt Makefile zadani.txt implementacni_plan.md Lukas-plan.md

doxygen:
	doxygen Doxyfile